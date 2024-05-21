#include "llvm/Transforms/Instrumentation/FilPizlonator.h"

#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/TypedPointerType.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>
#include <llvm/TargetParser/Triple.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace llvm;

// This is some crazy code. I believe that it's sound (assuming assertions are on LMFAO). I did
// it the right way for getting an experiment running quickly, but definitely the wrong way for
// long-term maintenance.
//
// Some shit to look out for, I'm not even kidding:
//
// - I have no idea how to use IRBuilder correctly and I don't even care, so get used to direct
//   Instruction construction.
//
// - I'm RAUW'ing to a different type, so I had to comment out asserts throughout the rest of
//   llvm to make this pass work. I think that llvm wants me to do this by creating new IR based
//   on the existing IR, but I ain't got time for that shit.
//
// - This pass is meant to issue compiler errors, like clang would. There are probably smart ways
//   to do that! This doesn't do any of them! Make sure to compile llvm with asserts enabled
//   (i.e. -DLLVM_ENABLE_ASSERTIONS=ON) or this pass won't actually catch any errors.
//
// - Look out for the use of dummy instructions to perform RAUW hacks, those are the best.
//
// - Bunch of other stuff. Look, I know how to architect compilers, and I have exquisite tastes
//   when I put that hat on. I did not put that hat on when writing this pass. I wrote this pass
//   to check if my programming language design - a memory safe C, no less! - actually works.
//
// Think of this as prototype code. If you put that hat on, you'll get it, and you'll be hella
// amused, I promise.

namespace {

static cl::opt<bool> verbose(
  "filc-verbose", cl::desc("Make FilC verbose"),
  cl::Hidden, cl::init(false));
static cl::opt<bool> ultraVerbose(
  "filc-ultra-verbose", cl::desc("Make FilC ultra verbose"),
  cl::Hidden, cl::init(false));
static cl::opt<bool> logAllocations(
  "filc-log-allocations", cl::desc("Make FilC emit code to log every allocation"),
  cl::Hidden, cl::init(false));

static constexpr size_t MinAlign = 16;
static constexpr size_t WordSize = 16;

// This has to match the FilC runtime.
typedef uint8_t WordType;

static constexpr WordType WordTypeUnset = 0;
static constexpr WordType WordTypeInt = 1;
static constexpr WordType WordTypePtr = 2;
static constexpr WordType WordTypeFunction = 4;

static constexpr uint16_t ObjectFlagReturnBuffer = 2;
static constexpr uint16_t ObjectFlagSpecial = 4;
static constexpr uint16_t ObjectFlagGlobal = 8;
static constexpr uint16_t ObjectFlagReadonly = 32;

enum class AccessKind {
  Read,
  Write
};

enum class ConstantKind {
  Global,
  Expr
};

struct ConstantTarget {
  ConstantTarget() { }

  ConstantTarget(ConstantKind Kind, GlobalValue* Target)
    : Kind(Kind)
    , Target(Target) {
  }

  explicit operator bool() const { return !!Target; }
  
  ConstantKind Kind { ConstantKind::Global };
  GlobalValue* Target { nullptr }; // Getter for globals (including functions), constexpr node for constexprs
};

struct ConstantRelocation {
  ConstantRelocation() { }

  ConstantRelocation(size_t Offset, ConstantKind Kind, GlobalValue* Target)
    : Offset(Offset)
    , Kind(Kind)
    , Target(Target) {
  }
  
  size_t Offset { 0 };
  ConstantKind Kind { ConstantKind::Global };
  GlobalValue* Target { nullptr }; // Getter for globals (including functions), constexpr node for constexprs
};

enum class ConstexprOpcode {
  AddPtrImmediate
};

enum class StoreKind {
  Unbarriered,
  Barriered
};

struct ValuePtr {
  ValuePtr() { }

  ValuePtr(Value* V, size_t PtrIndex)
    : V(V), PtrIndex(PtrIndex) {
  }

  Value* V { nullptr };
  size_t PtrIndex { 0 };

  bool operator==(const ValuePtr& Other) const {
    return V == Other.V && PtrIndex == Other.PtrIndex;
  }

  size_t hash() const {
    return std::hash<Value*>()(V) + PtrIndex;
  }
};

} // anonymous namespace

template<> struct std::hash<ValuePtr> {
  size_t operator()(const ValuePtr& Key) const {
    return Key.hash();
  }
};

namespace {

static constexpr size_t ScratchObjectIndex = 0;
static constexpr size_t MyArgsObjectIndex = 1;
static constexpr size_t NumSpecialFrameObjects = 2;

class Pizlonator {
  static constexpr unsigned TargetAS = 0;
  
  LLVMContext& C;
  Module &M;
  DataLayout& DL;

  unsigned PtrBits;
  Type* VoidTy;
  Type* Int1Ty;
  Type* Int8Ty;
  Type* Int16Ty;
  Type* Int32Ty;
  Type* IntPtrTy;
  Type* Int128Ty;
  PointerType* LowRawPtrTy;
  StructType* LowWidePtrTy;
  StructType* OriginTy;
  StructType* ObjectTy;
  StructType* FrameTy;
  StructType* ThreadTy;
  StructType* ConstantRelocationTy;
  StructType* ConstexprNodeTy;
  FunctionType* PizlonatedFuncTy;
  FunctionType* GlobalGetterTy;
  FunctionType* CtorDtorTy;
  Constant* LowRawNull;
  Constant* LowWideNull;
  BitCastInst* Dummy;

  // Low-level functions used by codegen.
  FunctionCallee Pollcheck;
  FunctionCallee StoreBarrier;
  FunctionCallee GetNextBytesForVAArg;
  FunctionCallee AllocateFunction;
  FunctionCallee Allocate;
  FunctionCallee AllocateWithAlignment;
  FunctionCallee CheckReadInt;
  FunctionCallee CheckReadPtr;
  FunctionCallee CheckWriteInt;
  FunctionCallee CheckWritePtr;
  FunctionCallee CheckFunctionCall;
  FunctionCallee Memset;
  FunctionCallee Memmove;
  FunctionCallee GlobalInitializationContextCreate;
  FunctionCallee GlobalInitializationContextAdd;
  FunctionCallee GlobalInitializationContextDestroy;
  FunctionCallee ExecuteConstantRelocations;
  FunctionCallee DeferOrRunGlobalCtor;
  FunctionCallee RunGlobalDtor;
  FunctionCallee Error;
  FunctionCallee RealMemset;

  std::unordered_map<std::string, GlobalVariable*> Strings;
  std::unordered_map<DILocation*, GlobalVariable*> Origins;
  std::unordered_map<Function*, GlobalVariable*> OriginsForFunctions;

  std::vector<GlobalVariable*> Globals;
  std::vector<Function*> Functions;
  std::vector<GlobalAlias*> Aliases;
  std::unordered_map<GlobalValue*, Type*> GlobalLowTypes;
  std::unordered_map<GlobalValue*, Type*> GlobalHighTypes;

  std::unordered_map<Type*, Type*> LoweredTypes;

  std::unordered_map<GlobalValue*, Function*> GlobalToGetter;
  std::unordered_map<GlobalValue*, GlobalVariable*> GlobalToGlobal;
  std::unordered_set<Value*> Getters;
  std::unordered_map<Function*, Function*> FunctionToHiddenFunction;

  std::string FunctionName;
  Function* OldF;
  Function* NewF;

  std::unordered_map<Instruction*, Type*> InstLowTypes;
  std::unordered_map<Instruction*, std::vector<Type*>> InstLowTypeVectors;
  
  BasicBlock* FirstRealBlock;

  BitCastInst* FutureReturnBuffer;

  BasicBlock* ReturnB;
  PHINode* ReturnPhi;

  size_t ReturnBufferSize;
  size_t ReturnBufferAlignment;

  Value* ArgBufferPtr; /* This is left in a state where it's advanced past the last declared
                          arg. */
  std::vector<Value*> Args;

  Value* MyThread;

  std::unordered_map<ValuePtr, size_t> FrameIndexMap;
  size_t FrameSize;
  Value* Frame;

  BitCastInst* makeDummy(Type* T) {
    return new BitCastInst(UndefValue::get(T), T, "dummy");
  }

  GlobalVariable* getString(StringRef Str) {
    auto iter = Strings.find(Str.str());
    if (iter != Strings.end())
      return iter->second;

    Constant* C = ConstantDataArray::getString(this->C, Str);
    GlobalVariable* Result = new GlobalVariable(
      M, C->getType(), true, GlobalVariable::PrivateLinkage, C, "filc_string");
    Strings[Str.str()] = Result;
    return Result;
  }

  std::string getFunctionName(Function *F) {
    std::string FunctionName;
    if (char* DemangledName = itaniumDemangle(F->getName())) {
      FunctionName = DemangledName;
      free(DemangledName);
    } else
      FunctionName = F->getName();
    return FunctionName;
  }

  Value* getOriginForFunction(Function* F) {
    auto iter = OriginsForFunctions.find(F);
    if (iter != OriginsForFunctions.end())
      return iter->second;

    Constant* C = ConstantStruct::get(
      OriginTy,
      { getString(getFunctionName(F)), getString(F->getSubprogram()->getFilename()),
        ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 0) });
    GlobalVariable* Result = new GlobalVariable(
      M, OriginTy, true, GlobalVariable::PrivateLinkage, C, "filc_function_origin");
    OriginsForFunctions[F] = Result;
    return Result;
  }

  Value* getOrigin(DebugLoc Loc) {
    if (!Loc)
      return LowRawNull;
    
    DILocation* Impl = Loc.get();
    auto iter = Origins.find(Impl);
    if (iter != Origins.end())
      return iter->second;

    Constant* C = ConstantStruct::get(
      OriginTy,
      { getString(FunctionName), getString(cast<DIScope>(Loc.getScope())->getFilename()),
        ConstantInt::get(Int32Ty, Loc.getLine()), ConstantInt::get(Int32Ty, Loc.getCol()) });
    GlobalVariable* Result = new GlobalVariable(
      M, OriginTy, true, GlobalVariable::PrivateLinkage, C, "filc_origin");
    Origins[Impl] = Result;
    return Result;
  }

  Type* lowerTypeImpl(Type* T) {
    assert(T != LowWidePtrTy);
    
    if (FunctionType* FT = dyn_cast<FunctionType>(T))
      return PizlonatedFuncTy;

    if (isa<TypedPointerType>(T)) {
      llvm_unreachable("Shouldn't ever see typed pointers");
      return nullptr;
    }

    if (isa<PointerType>(T)) {
      if (T->getPointerAddressSpace() == TargetAS) {
        assert(T == LowRawPtrTy);
        return LowWidePtrTy;
      }
      return T;
    }

    if (StructType* ST = dyn_cast<StructType>(T)) {
      if (ST->isOpaque())
        return ST;
      std::vector<Type*> Elements;
      for (Type* InnerT : ST->elements())
        Elements.push_back(lowerType(InnerT));
      if (ST->isLiteral())
        return StructType::get(C, Elements, ST->isPacked());
      std::string NewName = ("pizlonated_" + ST->getName()).str();
      return StructType::create(C, Elements, NewName, ST->isPacked());
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(T))
      return ArrayType::get(lowerType(AT->getElementType()), AT->getNumElements());
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(T))
      return FixedVectorType::get(lowerType(VT->getElementType()), VT->getElementCount().getFixedValue());

    if (ScalableVectorType* VT = dyn_cast<ScalableVectorType>(T)) {
      llvm_unreachable("Shouldn't see scalable vector types");
      return nullptr;
    }
    
    return T;
  }

  Type* lowerType(Type* T) {
    auto iter = LoweredTypes.find(T);
    if (iter != LoweredTypes.end())
      return iter->second;

    Type* LowT = lowerTypeImpl(T);
    assert(T->isSized() == LowT->isSized());
    if (T->isSized()) {
      if (DL.getTypeStoreSizeBeforeFilC(T) != DL.getTypeStoreSize(LowT)) {
        errs() << "Error lowering type: " << *T << "\n"
               << "Type after lowering: " << *LowT << "\n"
               << "Predicted lowered size: " << DL.getTypeStoreSizeBeforeFilC(T) << "\n"
               << "Actual lowered size: " << DL.getTypeStoreSize(LowT) << "\n";
      }
      assert(DL.getTypeStoreSizeBeforeFilC(T) == DL.getTypeStoreSize(LowT));
    }
    LoweredTypes[T] = LowT;
    return LowT;
  }

  void checkReadInt(Value *P, unsigned Size, Instruction *InsertBefore) {
    CallInst::Create(
      CheckReadInt,
      { P, ConstantInt::get(IntPtrTy, Size), getOrigin(InsertBefore->getDebugLoc()) },
      "", InsertBefore)
      ->setDebugLoc(InsertBefore->getDebugLoc());
  }

  void checkWriteInt(Value *P, unsigned Size, Instruction *InsertBefore) {
    CallInst::Create(
      CheckWriteInt,
      { P, ConstantInt::get(IntPtrTy, Size), getOrigin(InsertBefore->getDebugLoc()) },
      "", InsertBefore)
      ->setDebugLoc(InsertBefore->getDebugLoc());
  }

  void checkInt(Value* P, unsigned Size, AccessKind AK, Instruction *InsertBefore) {
    switch (AK) {
    case AccessKind::Read:
      checkReadInt(P, Size, InsertBefore);
      return;
    case AccessKind::Write:
      checkWriteInt(P, Size, InsertBefore);
      return;
    }
    llvm_unreachable("Bad access kind");
  }

  void checkReadPtr(Value *P, Instruction *InsertBefore) {
    if (verbose)
      errs() << "Inserting call to " << *CheckReadPtr.getFunctionType() << "\n";
    CallInst::Create(
      CheckReadPtr, { P, getOrigin(InsertBefore->getDebugLoc()) }, "", InsertBefore)
      ->setDebugLoc(InsertBefore->getDebugLoc());
  }

  void checkWritePtr(Value *P, Instruction *InsertBefore) {
    if (verbose)
      errs() << "Inserting call to " << *CheckWritePtr.getFunctionType() << "\n";
    CallInst::Create(
      CheckWritePtr, { P, getOrigin(InsertBefore->getDebugLoc()) }, "", InsertBefore)
      ->setDebugLoc(InsertBefore->getDebugLoc());
  }

  void checkPtr(Value *P, AccessKind AK, Instruction *InsertBefore) {
    switch (AK) {
    case AccessKind::Read:
      checkReadPtr(P, InsertBefore);
      return;
    case AccessKind::Write:
      checkWritePtr(P, InsertBefore);
      return;
    }
    llvm_unreachable("Bad access kind");
  }

  Value* ptrWord(Value* P, Instruction* InsertBefore) {
    if (isa<ConstantAggregateZero>(P))
      return ConstantInt::get(Int128Ty, 0);
    Instruction* Result = ExtractValueInst::Create(Int128Ty, P, { 0 }, "filc_ptr_word", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  Value* ptrPtr(Value* P, Instruction* InsertBefore) {
    Instruction* IntResult = new TruncInst(ptrWord(P, InsertBefore), IntPtrTy, "filc_ptr_ptr_as_int",
                                           InsertBefore);
    IntResult->setDebugLoc(InsertBefore->getDebugLoc());
    Instruction* Result = new IntToPtrInst(IntResult, LowRawPtrTy, "filc_ptr_ptr", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  Value* ptrObject(Value* P, Instruction* InsertBefore) {
    Instruction* Shifted = BinaryOperator::Create(
      Instruction::LShr, ptrWord(P, InsertBefore), ConstantInt::get(Int128Ty, 64), "filc_ptr_object_shift",
      InsertBefore);
    Shifted->setDebugLoc(InsertBefore->getDebugLoc());
    Instruction* IntResult = new TruncInst(Shifted, IntPtrTy, "filc_ptr_object_trunc_as_int", InsertBefore);
    IntResult->setDebugLoc(InsertBefore->getDebugLoc());
    Instruction* Result = new IntToPtrInst(IntResult, LowRawPtrTy, "filc_ptr_ptr", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  Value* wordToPtr(Value* Word, Instruction* InsertBefore) {
    Instruction* Result = InsertValueInst::Create(
      UndefValue::get(LowWidePtrTy), Word, { 0 }, "filc_word_to_ptr", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  Value* createPtr(Value* Object, Value* Ptr, Instruction* InsertBefore) {
    Instruction* PtrAsInt = new PtrToIntInst(Ptr, IntPtrTy, "filc_ptr_as_int", InsertBefore);
    PtrAsInt->setDebugLoc(InsertBefore->getDebugLoc());
    Instruction* ObjectAsInt = new PtrToIntInst(Object, IntPtrTy, "filc_object_as_int", InsertBefore);
    ObjectAsInt->setDebugLoc(InsertBefore->getDebugLoc());
    Instruction* CastedPtr = new ZExtInst(PtrAsInt, Int128Ty, "filc_ptr_zext", InsertBefore);
    CastedPtr->setDebugLoc(InsertBefore->getDebugLoc());
    Instruction* CastedObject = new ZExtInst(ObjectAsInt, Int128Ty, "filc_object_zext", InsertBefore);
    CastedObject->setDebugLoc(InsertBefore->getDebugLoc());
    Instruction* ShiftedObject = BinaryOperator::Create(
      Instruction::Shl, CastedObject, ConstantInt::get(Int128Ty, 64), "filc_object_shifted", InsertBefore);
    ShiftedObject->setDebugLoc(InsertBefore->getDebugLoc());
    Instruction* Word = BinaryOperator::Create(
      Instruction::Or, CastedPtr, ShiftedObject, "filc_ptr_word", InsertBefore);
    Word->setDebugLoc(InsertBefore->getDebugLoc());
    return wordToPtr(Word, InsertBefore);
  }

  Value* ptrWithPtr(Value* LowWidePtr, Value* NewLowRawPtr, Instruction* InsertBefore) {
    return createPtr(ptrObject(LowWidePtr, InsertBefore), NewLowRawPtr, InsertBefore);
  }

  Value* objectLowerPtr(Value* Object, Instruction* InsertBefore) {
    Instruction* LowerPtr = GetElementPtrInst::Create(
      ObjectTy, Object, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 0) },
      "filc_object_lower_ptr", InsertBefore);
    LowerPtr->setDebugLoc(InsertBefore->getDebugLoc());
    return LowerPtr;
  }

  Value* objectUpperPtr(Value* Object, Instruction* InsertBefore) {
    Instruction* LowerPtr = GetElementPtrInst::Create(
      ObjectTy, Object, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) },
      "filc_object_upper_ptr", InsertBefore);
    LowerPtr->setDebugLoc(InsertBefore->getDebugLoc());
    return LowerPtr;
  }

  Value* objectFlagsPtr(Value* Object, Instruction* InsertBefore) {
    Instruction* LowerPtr = GetElementPtrInst::Create(
      ObjectTy, Object, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 2) },
      "filc_object_flags_ptr", InsertBefore);
    LowerPtr->setDebugLoc(InsertBefore->getDebugLoc());
    return LowerPtr;
  }

  Value* objectLower(Value* Object, Instruction* InsertBefore) {
    Instruction* Lower = new LoadInst(
      LowRawPtrTy, objectLowerPtr(Object, InsertBefore), "filc_object_lower_load", InsertBefore);
    Lower->setDebugLoc(InsertBefore->getDebugLoc());
    return Lower;
  }

  Value* ptrWithObject(Value* Object, Instruction* InsertBefore) {
    return createPtr(Object, objectLower(Object, InsertBefore), InsertBefore);
  }

  Value* badPtr(Value* P, Instruction* InsertBefore) {
    return createPtr(LowRawNull, P, InsertBefore);
  }

  Value* loadPtr(
    Value* P, bool isVolatile, Align A, AtomicOrdering AO, Instruction* InsertBefore) {
    // FIXME: I probably only need Unordered, not Monotonic.
    Instruction* Result = new LoadInst(
      Int128Ty, P, "filc_load_ptr", isVolatile,
      std::max(A, Align(WordSize)), getMergedAtomicOrdering(AtomicOrdering::Monotonic, AO),
      SyncScope::System, InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return wordToPtr(Result, InsertBefore);
  }

  Value* loadPtr(Value* P, Instruction* InsertBefore) {
    return loadPtr(P, false, Align(WordSize), AtomicOrdering::Monotonic, InsertBefore);
  }
  
  void storePtr(
    Value* V, Value* P, bool isVolatile, Align A, AtomicOrdering AO, StoreKind SK,
    Instruction* InsertBefore) {
    if (SK == StoreKind::Barriered) {
      assert(MyThread);
      CallInst::Create(StoreBarrier, { MyThread, ptrObject(V, InsertBefore) }, "", InsertBefore)
        ->setDebugLoc(InsertBefore->getDebugLoc());
    }
    (new StoreInst(
      ptrWord(V, InsertBefore), P, isVolatile, std::max(A, Align(WordSize)),
      getMergedAtomicOrdering(AtomicOrdering::Monotonic, AO), SyncScope::System, InsertBefore))
      ->setDebugLoc(InsertBefore->getDebugLoc());
  }

  void storePtr(Value* V, Value* P, StoreKind SK, Instruction* InsertBefore) {
    storePtr(V, P, false, Align(WordSize), AtomicOrdering::Monotonic, SK, InsertBefore);
  }

  // This happens to work just as well for high types and low types, and that's important.
  bool hasPtrsForCheck(Type *T) {
    if (FunctionType* FT = dyn_cast<FunctionType>(T)) {
      llvm_unreachable("shouldn't see function types in hasPtrsForCheck");
      return false;
    }

    if (isa<TypedPointerType>(T)) {
      llvm_unreachable("Shouldn't ever see typed pointers");
      return false;
    }

    if (isa<PointerType>(T)) {
      assert (!T->getPointerAddressSpace());
      return true;
    }

    if (T == LowWidePtrTy)
      return true;

    if (StructType* ST = dyn_cast<StructType>(T)) {
      for (unsigned Index = ST->getNumElements(); Index--;) {
        Type* InnerT = ST->getElementType(Index);
        if (hasPtrsForCheck(InnerT))
          return true;
      }
      return false;
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(T))
      return hasPtrsForCheck(AT->getElementType());

    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(T))
      return hasPtrsForCheck(VT->getElementType());

    if (ScalableVectorType* VT = dyn_cast<ScalableVectorType>(T)) {
      llvm_unreachable("Shouldn't ever see scalable vectors in hasPtrsForCheck");
      return false;
    }
    
    return false;
  }

  void checkRecurse(Type *LowT, Value* HighP, Value *P, AccessKind AK, Instruction *InsertBefore) {
    if (!hasPtrsForCheck(LowT)) {
      checkInt(ptrWithPtr(HighP, P, InsertBefore), DL.getTypeStoreSize(LowT), AK, InsertBefore);
      return;
    }
    
    if (FunctionType* FT = dyn_cast<FunctionType>(LowT)) {
      llvm_unreachable("shouldn't see function types in checkRecurse");
      return;
    }

    if (isa<TypedPointerType>(LowT)) {
      llvm_unreachable("Shouldn't ever see typed pointers");
      return;
    }

    assert(LowT != LowRawPtrTy);

    if (LowT == LowWidePtrTy) {
      checkPtr(ptrWithPtr(HighP, P, InsertBefore), AK, InsertBefore);
      return;
    }

    assert(!isa<PointerType>(LowT));

    if (StructType* ST = dyn_cast<StructType>(LowT)) {
      for (unsigned Index = ST->getNumElements(); Index--;) {
        Type* InnerT = ST->getElementType(Index);
        Value *InnerP = GetElementPtrInst::Create(
          ST, P, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, Index) },
          "filc_InnerP_struct", InsertBefore);
        checkRecurse(InnerT, HighP, InnerP, AK, InsertBefore);
      }
      return;
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(LowT)) {
      for (uint64_t Index = AT->getNumElements(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          AT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "filc_InnerP_array", InsertBefore);
        checkRecurse(AT->getElementType(), HighP, InnerP, AK, InsertBefore);
      }
      return;
    }
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(LowT)) {
      for (unsigned Index = VT->getElementCount().getFixedValue(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          VT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "filc_InnerP_vector", InsertBefore);
        checkRecurse(VT->getElementType(), HighP, InnerP, AK, InsertBefore);
      }
      return;
    }

    if (ScalableVectorType* VT = dyn_cast<ScalableVectorType>(LowT)) {
      llvm_unreachable("Shouldn't see scalable vector types in checkRecurse");
      return;
    }

    llvm_unreachable("Should not get here.");
  }

  // Insert whatever checks are needed to perform the access and then return the lowered pointer to
  // access.
  Value* prepareForAccess(Type *LowT, Value *HighP, AccessKind AK, Instruction *InsertBefore) {
    Value* LowP = ptrPtr(HighP, InsertBefore);
    checkRecurse(LowT, HighP, LowP, AK, InsertBefore);
    return LowP;
  }

  Value* loadValueRecurseAfterCheck(
    Type* LowT, Value* HighP, Value* P,
    bool isVolatile, Align A, AtomicOrdering AO, SyncScope::ID SS,
    Instruction* InsertBefore) {
    A = std::min(DL.getABITypeAlign(LowT), A);
    
    if (!hasPtrsForCheck(LowT))
      return new LoadInst(LowT, P, "filc_load", isVolatile, A, AO, SS, InsertBefore);
    
    if (FunctionType* FT = dyn_cast<FunctionType>(LowT)) {
      llvm_unreachable("shouldn't see function types in checkRecurse");
      return nullptr;
    }

    if (isa<TypedPointerType>(LowT)) {
      llvm_unreachable("Shouldn't ever see typed pointers");
      return nullptr;
    }

    assert(LowT != LowRawPtrTy);

    if (LowT == LowWidePtrTy)
      return loadPtr(P, isVolatile, A, AO, InsertBefore);

    assert(!isa<PointerType>(LowT));

    if (StructType* ST = dyn_cast<StructType>(LowT)) {
      Value* Result = UndefValue::get(ST);
      for (unsigned Index = ST->getNumElements(); Index--;) {
        Type* InnerT = ST->getElementType(Index);
        Value *InnerP = GetElementPtrInst::Create(
          ST, P, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, Index) },
          "filc_InnerP_struct", InsertBefore);
        Value* V = loadValueRecurse(InnerT, HighP, InnerP, isVolatile, A, AO, SS, InsertBefore);
        Result = InsertValueInst::Create(Result, V, Index, "filc_insert_struct", InsertBefore);
      }
      return Result;
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(LowT)) {
      Value* Result = UndefValue::get(AT);
      assert(static_cast<unsigned>(AT->getNumElements()) == AT->getNumElements());
      for (unsigned Index = static_cast<unsigned>(AT->getNumElements()); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          AT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "filc_InnerP_array", InsertBefore);
        Value* V = loadValueRecurse(
          AT->getElementType(), HighP, InnerP, isVolatile, A, AO, SS, InsertBefore);
        Result = InsertValueInst::Create(Result, V, Index, "filc_insert_array", InsertBefore);
      }
      return Result;
    }
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(LowT)) {
      Value* Result = UndefValue::get(VT);
      for (unsigned Index = VT->getElementCount().getFixedValue(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          VT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "filc_InnerP_vector", InsertBefore);
        Value* V = loadValueRecurse(
          VT->getElementType(), HighP, InnerP, isVolatile, A, AO, SS, InsertBefore);
        Result = InsertElementInst::Create(
          Result, V, ConstantInt::get(IntPtrTy, Index), "filc_insert_vector", InsertBefore);
      }
      return Result;
    }

    if (ScalableVectorType* VT = dyn_cast<ScalableVectorType>(LowT)) {
      llvm_unreachable("Shouldn't see scalable vector types in checkRecurse");
      return nullptr;
    }

    llvm_unreachable("Should not get here.");
    return nullptr;
  }

  Value* loadValueRecurse(Type* LowT, Value* HighP, Value* P,
                          bool isVolatile, Align A, AtomicOrdering AO, SyncScope::ID SS,
                          Instruction* InsertBefore) {
    // In the future, checks will exit or pollcheck, so we need to ensure that we do all of them before
    // we do the actual load.
    checkRecurse(LowT, HighP, P, AccessKind::Read, InsertBefore);
    return loadValueRecurseAfterCheck(LowT, HighP, P, isVolatile, A, AO, SS, InsertBefore);
  }

  void storeValueRecurseAfterCheck(
    Type* LowT, Value* HighP, Value* V, Value* P,
    bool isVolatile, Align A, AtomicOrdering AO, SyncScope::ID SS, StoreKind SK,
    Instruction* InsertBefore) {
    A = std::min(DL.getABITypeAlign(LowT), A);
    
    if (!hasPtrsForCheck(LowT)) {
      new StoreInst(V, P, isVolatile, A, AO, SS, InsertBefore);
      return;
    }
    
    if (FunctionType* FT = dyn_cast<FunctionType>(LowT)) {
      llvm_unreachable("shouldn't see function types in checkRecurse");
      return;
    }

    if (isa<TypedPointerType>(LowT)) {
      llvm_unreachable("Shouldn't ever see typed pointers");
      return;
    }

    assert(LowT != LowRawPtrTy);

    if (LowT == LowWidePtrTy) {
      storePtr(V, P, isVolatile, A, AO, SK, InsertBefore);
      return;
    }

    assert(!isa<PointerType>(LowT));

    if (StructType* ST = dyn_cast<StructType>(LowT)) {
      for (unsigned Index = ST->getNumElements(); Index--;) {
        Type* InnerT = ST->getElementType(Index);
        Value *InnerP = GetElementPtrInst::Create(
          ST, P, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, Index) },
          "filc_InnerP_struct", InsertBefore);
        Value* InnerV = ExtractValueInst::Create(
          InnerT, V, { Index }, "filc_extract_struct", InsertBefore);
        storeValueRecurse(InnerT, HighP, InnerV, InnerP, isVolatile, A, AO, SS, SK, InsertBefore);
      }
      return;
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(LowT)) {
      assert(static_cast<unsigned>(AT->getNumElements()) == AT->getNumElements());
      for (unsigned Index = static_cast<unsigned>(AT->getNumElements()); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          AT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "filc_InnerP_array", InsertBefore);
        Value* InnerV = ExtractValueInst::Create(
          AT->getElementType(), V, { Index }, "filc_extract_array", InsertBefore);
        storeValueRecurse(AT->getElementType(), HighP, InnerV, InnerP, isVolatile, A, AO, SS, SK,
                          InsertBefore);
      }
      return;
    }
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(LowT)) {
      for (unsigned Index = VT->getElementCount().getFixedValue(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          VT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "filc_InnerP_vector", InsertBefore);
        Value* InnerV = ExtractElementInst::Create(
          V, ConstantInt::get(IntPtrTy, Index), "filc_extract_vector", InsertBefore);
        storeValueRecurse(VT->getElementType(), HighP, InnerV, InnerP, isVolatile, A, AO, SS, SK,
                          InsertBefore);
      }
      return;
    }

    if (ScalableVectorType* VT = dyn_cast<ScalableVectorType>(LowT)) {
      llvm_unreachable("Shouldn't see scalable vector types in checkRecurse");
      return;
    }

    llvm_unreachable("Should not get here.");
  }

  void storeValueRecurse(Type* LowT, Value* HighP, Value* V, Value* P,
                         bool isVolatile, Align A, AtomicOrdering AO, SyncScope::ID SS, StoreKind SK,
                         Instruction* InsertBefore) {
    checkRecurse(LowT, HighP, P, AccessKind::Write, InsertBefore);
    storeValueRecurseAfterCheck(LowT, HighP, V, P, isVolatile, A, AO, SS, SK, InsertBefore);
  }

  Value* allocateObject(Value* Size, size_t Alignment, Instruction* InsertBefore) {
    Instruction* Result;
    if (Alignment <= MinAlign) {
      Result = CallInst::Create(
        Allocate, { MyThread, Size }, "filc_allocate", InsertBefore);
    } else {
      Result = CallInst::Create(
        AllocateWithAlignment,
        { MyThread, Size, ConstantInt::get(IntPtrTy, Alignment) },
        "filc_allocate", InsertBefore);
    }
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  Value* allocate(Value* Size, size_t Alignment, Instruction* InsertBefore) {
    return ptrWithObject(allocateObject(Size, Alignment, InsertBefore), InsertBefore);
  }

  size_t countPtrs(Type* T) {
    assert(!isa<FunctionType>(T));
    assert(!isa<TypedPointerType>(T));
    assert(T != LowWidePtrTy);
    assert(!isa<ScalableVectorType>(T));

    if (isa<PointerType>(T))
      return 1;

    if (StructType* ST = dyn_cast<StructType>(T)) {
      size_t Result = 0;
      for (unsigned Index = ST->getNumElements(); Index--;)
        Result += countPtrs(ST->getElementType(Index));
      return Result;
    }

    if (ArrayType* AT = dyn_cast<ArrayType>(T))
      return AT->getNumElements() * countPtrs(AT->getElementType());

    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(T))
      return VT->getElementCount().getFixedValue() * countPtrs(VT->getElementType());

    assert(!hasPtrsForCheck(T));
    return 0;
  }

  void computeFrameIndexMap(std::vector<BasicBlock*> Blocks) {
    FrameIndexMap.clear();
    FrameSize = NumSpecialFrameObjects;

    assert(!Blocks.empty());

    auto LiveCast = [&] (Value* V) -> Value* {
      if (isa<Instruction>(V))
        return V;
      if (isa<Argument>(V))
        return V;
      if (!isa<Constant>(V) && !isa<MetadataAsValue>(V) && !isa<InlineAsm>(V) && !isa<BasicBlock>(V)) {
        errs() << "V = " << *V << "\n";
        assert(isa<Constant>(V) || isa<MetadataAsValue>(V) || isa<InlineAsm>(V) || isa<BasicBlock>(V));
      }
      return nullptr;
    };

    std::unordered_map<BasicBlock*, std::unordered_set<Value*>> LiveAtTail;
    bool Changed = true;
    while (Changed) {
      Changed = false;

      for (size_t BlockIndex = Blocks.size(); BlockIndex--;) {
        BasicBlock* BB = Blocks[BlockIndex];
        std::unordered_set<Value*> Live = LiveAtTail[BB];

        for (auto It = BB->rbegin(); It != BB->rend(); ++It) {
          Instruction* I = &*It;
          if (verbose)
            errs() << "Liveness dealing with: " << *I << "\n";

          Live.erase(I);
          
          if (PHINode* Phi = dyn_cast<PHINode>(I)) {
            for (unsigned Index = Phi->getNumIncomingValues(); Index--;) {
              Value* V = Phi->getIncomingValue(Index);
              if (Value* LV = LiveCast(V)) {
                BasicBlock* PBB = Phi->getIncomingBlock(Index);
                Changed |= LiveAtTail[PBB].insert(LV).second;
              }
            }
            continue;
          }

          for (Value* V : I->operand_values()) {
            if (Value* LV = LiveCast(V))
              Live.insert(LV);
          }
        }

        // NOTE: We might be given IR with unreachable blocks. Those will have live-at-head. Like, whatever.
        // But if it's the root block and it has live-at-head then what the fugc.
        if (!BlockIndex) {
          for (Value* V : Live)
            assert(isa<Argument>(V));
        }

        for (BasicBlock* PBB : predecessors(BB)) {
          for (Value* LV : Live)
            Changed |= LiveAtTail[PBB].insert(LV).second;
        }
      }
    }

    std::unordered_map<ValuePtr, std::unordered_set<ValuePtr>> Interference;

    for (size_t BlockIndex = Blocks.size(); BlockIndex--;) {
      BasicBlock* BB = Blocks[BlockIndex];
      std::unordered_set<Value*> Live = LiveAtTail[BB];
      
      for (auto It = BB->rbegin(); It != BB->rend(); ++It) {
        Instruction* I = &*It;
        
        Live.erase(I);

        size_t NumIPtrs = countPtrs(I->getType());
        if (NumIPtrs) {
          for (Value* LV : Live) {
            size_t NumVIPtrs = countPtrs(LV->getType());
            for (size_t LVPtrIndex = NumVIPtrs; LVPtrIndex--;) {
              for (size_t IPtrIndex = NumIPtrs; IPtrIndex--;) {
                Interference[ValuePtr(I, IPtrIndex)].insert(ValuePtr(LV, LVPtrIndex));
                Interference[ValuePtr(LV, LVPtrIndex)].insert(ValuePtr(I, IPtrIndex));
              }
            }
          }
        }

        for (Value* V : I->operand_values()) {
          if (Value* LV = LiveCast(V))
            Live.insert(LV);
        }
      }
    }

    // The arguments interfere with one another.
    for (Argument& A1 : OldF->args()) {
      size_t NumA1Ptrs = countPtrs(A1.getType());
      if (!NumA1Ptrs)
        continue;
      for (Argument& A2 : OldF->args()) {
        size_t NumA2Ptrs = countPtrs(A2.getType());
        for (size_t A2PtrIndex = NumA2Ptrs; A2PtrIndex--;) {
          for (size_t A1PtrIndex = NumA1Ptrs; A1PtrIndex--;)
            Interference[ValuePtr(&A1, A1PtrIndex)].insert(ValuePtr(&A2, A2PtrIndex));
        }
      }
    }

    // Make this deterministic by having a known order in which we process stuff.
    std::vector<ValuePtr> Order;
    for (Argument& A : OldF->args()) {
      for (size_t PtrIndex = countPtrs(A.getType()); PtrIndex--;)
        Order.push_back(ValuePtr(&A, PtrIndex));
    }
    for (BasicBlock* BB : Blocks) {
      for (Instruction& I : *BB) {
        for (size_t PtrIndex = countPtrs(I.getType()); PtrIndex--;)
          Order.push_back(ValuePtr(&I, PtrIndex));
      }
    }

    for (ValuePtr IP : Order) {
      const std::unordered_set<ValuePtr>& Adjacency = Interference[IP];
      for (size_t FrameIndex = NumSpecialFrameObjects; ; FrameIndex++) {
        bool Ok = true;
        for (ValuePtr AIP : Adjacency) {
          if (FrameIndexMap.count(AIP) && FrameIndexMap[AIP] == FrameIndex) {
            Ok = false;
            break;
          }
        }
        if (Ok) {
          FrameIndexMap[IP] = FrameIndex;
          FrameSize = std::max(FrameSize, FrameIndex + 1);
          break;
        }
      }
    }
  }

  void recordObjectAtIndex(Value* Object, size_t FrameIndex, Instruction* InsertBefore) {
    assert(FrameIndex < FrameSize);
    Instruction* ObjectsPtr = GetElementPtrInst::Create(
      FrameTy, Frame, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 3) },
      "filc_frame_objects", InsertBefore);
    ObjectsPtr->setDebugLoc(InsertBefore->getDebugLoc());
    Instruction* ObjectPtr = GetElementPtrInst::Create(
      LowRawPtrTy, ObjectsPtr, { ConstantInt::get(IntPtrTy, FrameIndex) }, "filc_frame_object",
      InsertBefore);
    ObjectPtr->setDebugLoc(InsertBefore->getDebugLoc());
    (new StoreInst(Object, ObjectPtr, InsertBefore))->setDebugLoc(InsertBefore->getDebugLoc());
  }

  void recordObjectsRecurse(
    Value* ValueKey, Type* LowT, Value* V, size_t& PtrIndex, Instruction* InsertBefore) {
    assert(!isa<FunctionType>(LowT));
    assert(!isa<TypedPointerType>(LowT));
    assert(!isa<ScalableVectorType>(LowT));
    assert(!isa<PointerType>(LowT));

    if (LowT == LowWidePtrTy) {
      assert(FrameIndexMap.count(ValuePtr(ValueKey, PtrIndex)));
      size_t FrameIndex = FrameIndexMap[ValuePtr(ValueKey, PtrIndex)];
      assert(FrameIndex >= NumSpecialFrameObjects);
      recordObjectAtIndex(ptrObject(V, InsertBefore), FrameIndex, InsertBefore);
      PtrIndex++;
      return;
    }

    if (StructType* ST = dyn_cast<StructType>(LowT)) {
      for (unsigned Index = ST->getNumElements(); Index--;) {
        Type* InnerT = ST->getElementType(Index);
        Value* InnerV = ExtractValueInst::Create(
          InnerT, V, { Index }, "filc_extract_struct", InsertBefore);
        recordObjectsRecurse(ValueKey, InnerT, InnerV, PtrIndex, InsertBefore);
      }
      return;
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(LowT)) {
      assert(static_cast<unsigned>(AT->getNumElements()) == AT->getNumElements());
      for (unsigned Index = static_cast<unsigned>(AT->getNumElements()); Index--;) {
        Value* InnerV = ExtractValueInst::Create(
          AT->getElementType(), V, { Index }, "filc_extract_array", InsertBefore);
        recordObjectsRecurse(ValueKey, AT->getElementType(), InnerV, PtrIndex, InsertBefore);
      }
      return;
    }
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(LowT)) {
      for (unsigned Index = VT->getElementCount().getFixedValue(); Index--;) {
        Value* InnerV = ExtractElementInst::Create(
          V, ConstantInt::get(IntPtrTy, Index), "filc_extract_vector", InsertBefore);
        recordObjectsRecurse(ValueKey, VT->getElementType(), InnerV, PtrIndex, InsertBefore);
      }
      return;
    }

    assert(!hasPtrsForCheck(LowT));
  }

  void recordObjects(Value* ValueKey, Type* LowT, Value* V, Instruction* InsertBefore) {
    if (verbose)
      errs() << "Recording objects for " << *ValueKey << ", LowT = " << *LowT << ", V = " << *V << "\n";
    size_t PtrIndex = 0;
    recordObjectsRecurse(ValueKey, LowT, V, PtrIndex, InsertBefore);
    assert(PtrIndex == countPtrs(ValueKey->getType()));
  }

  void recordObjects(Value* V, Instruction* InsertBefore) {
    recordObjects(V, lowerType(V->getType()), V, InsertBefore);
  }

  void buildWordTypesRecurse(
    Type* OuterLowT, Type* LowT, size_t& Size, std::vector<WordType>& WordTypes) {
    if (verbose)
      errs() << "Dealing with LowT = " << *LowT << ", Size = " << Size << "\n";
    
    assert((Size + WordSize - 1) / WordSize == WordTypes.size());
    if (Size % WordSize) {
      assert(!WordTypes.empty());
      assert(WordTypes.back() == WordTypeInt);
    }

    auto Fill = [&] () {
      while ((Size + WordSize - 1) / WordSize > WordTypes.size())
        WordTypes.push_back(WordTypeInt);
    };

    assert(LowT != LowRawPtrTy);
    
    if (LowT == LowWidePtrTy) {
      if ((Size % WordSize)) {
        errs() << "Trouble with offset of pointer in " << *OuterLowT
               << ", current LowT = " << *LowT << ", Size = " << Size << "\n";
      }
      assert(!(Size % WordSize));
      Size += WordSize;
      WordTypes.push_back(WordTypePtr);
      return;
    }

    if (StructType* ST = dyn_cast<StructType>(LowT)) {
      size_t SizeBefore = Size;
      const StructLayout* SL = DL.getStructLayout(ST);
      for (unsigned Index = 0; Index < ST->getNumElements(); ++Index) {
        Type* InnerT = ST->getElementType(Index);
        size_t ProposedSize = SizeBefore + SL->getElementOffset(Index);
        assert(ProposedSize >= Size);
        Size = ProposedSize;
        Fill();
        buildWordTypesRecurse(OuterLowT, InnerT, Size, WordTypes);
      }
      size_t ProposedSize = SizeBefore + SL->getSizeInBytes();
      assert(ProposedSize >= Size);
      Size = ProposedSize;
      Fill();
      return;
    }
    
    if (ArrayType* AT = dyn_cast<ArrayType>(LowT)) {
      for (uint64_t Index = AT->getNumElements(); Index--;)
        buildWordTypesRecurse(OuterLowT, AT->getElementType(), Size, WordTypes);
      return;
    }
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(LowT)) {
      for (unsigned Index = VT->getElementCount().getFixedValue(); Index--;)
        buildWordTypesRecurse(OuterLowT, VT->getElementType(), Size, WordTypes);
      return;
    }

    if (ScalableVectorType* VT = dyn_cast<ScalableVectorType>(LowT)) {
      llvm_unreachable("Shouldn't see scalable vector types in checkRecurse");
      return;
    }

    Size += DL.getTypeStoreSize(LowT);
    while ((Size + WordSize - 1) / WordSize > WordTypes.size())
      WordTypes.push_back(WordTypeInt);
  }

  Constant* objectConstant(Constant* Lower, Constant* Upper, uint16_t ObjectFlags,
                           const std::vector<WordType>& WordTypes) {
    StructType* ObjectTy = StructType::get(
      C, { LowRawPtrTy, LowRawPtrTy, Int16Ty, ArrayType::get(Int8Ty, WordTypes.size()) });
    return ConstantStruct::get(
      ObjectTy,
      { Lower, Upper, ConstantInt::get(Int16Ty, ObjectFlags), ConstantDataArray::get(C, WordTypes) });
  }

  bool isConstantZero(Constant* C) {
    if (isa<ConstantAggregateZero>(C))
      return true;
    if (ConstantInt* CI = dyn_cast<ConstantInt>(C))
      return CI->isZero();
    if (ConstantFP* CF = dyn_cast<ConstantFP>(C))
      return CF->isZero();
    if (ConstantAggregate* CA = dyn_cast<ConstantAggregate>(C)) {
      for (size_t Index = CA->getNumOperands(); Index--;) {
        if (!isConstantZero(CA->getOperand(Index)))
          return false;
      }
      return true;
    }
    return false;
  }

  GlobalVariable* objectGlobalForGlobal(GlobalValue* G, GlobalValue* OriginalG) {
    if (verbose)
      errs() << "Creating object constant for global = " << *G << "\n";
    std::vector<WordType> WordTypes;
    size_t ObjectSize;
    int16_t ObjectFlags = ObjectFlagGlobal;
    bool ObjectIsConstant = true;
    if (isa<Function>(G)) {
      WordTypes.push_back(WordTypeFunction);
      ObjectSize = WordSize;
      ObjectFlags |= ObjectFlagSpecial;
    } else {
      size_t Size = 0;
      Type* LowT = G->getValueType();
      buildWordTypesRecurse(LowT, LowT, Size, WordTypes);
      assert(Size == DL.getTypeStoreSize(LowT));
      assert(!(Size % WordSize));
      ObjectSize = Size;
      GlobalVariable* OriginalGV = cast<GlobalVariable>(OriginalG);
      if (OriginalGV->isConstant())
        ObjectFlags |= ObjectFlagReadonly;
      else if (isConstantZero(OriginalGV->getInitializer())) {
        // Read-write globals that have a zero initializer get to have unset type. This allows for
        // the C++ idiom where you allocate storage of char type to hold room for a class in order
        // to avoid the class's constructor or destructor from running.
        ObjectIsConstant = false;
        for (size_t Index = WordTypes.size(); Index--;)
          WordTypes[Index] = WordTypeUnset;
      }
    }
    assert(!(ObjectSize % WordSize));
    Constant* NewObjectC = objectConstant(
      G, ConstantExpr::getGetElementPtr(Int8Ty, G, ConstantInt::get(IntPtrTy, ObjectSize)),
      ObjectFlags, WordTypes);
    return new GlobalVariable(
      M, NewObjectC->getType(), ObjectIsConstant, GlobalValue::InternalLinkage, NewObjectC,
      "Jo_" + OriginalG->getName());
  }

  Constant* paddedConstant(Constant* C) {
    Type* LowT = C->getType();
    size_t TypeSize = DL.getTypeStoreSize(LowT);
    if (!(TypeSize % WordSize))
      return C;
    size_t PaddingSize = WordSize - (TypeSize % WordSize);
    if (verbose)
      errs() << "TypeSize = " << TypeSize << ", PaddingSize = " << PaddingSize << "\n";
    ArrayType* PaddingTy = ArrayType::get(Int8Ty, PaddingSize);
    StructType* PaddedTy = StructType::get(this->C, { LowT, PaddingTy });
    return ConstantStruct::get(PaddedTy, { C, ConstantAggregateZero::get(PaddingTy) });
  }

  enum class ResultMode {
    NeedFullConstant,
    NeedConstantWithPtrPlaceholders
  };
  Constant* tryLowerConstantToConstant(Constant* C, ResultMode RM = ResultMode::NeedFullConstant) {
    assert(C->getType() != LowWidePtrTy);
    
    if (isa<UndefValue>(C)) {
      if (isa<IntegerType>(C->getType()))
        return ConstantInt::get(C->getType(), 0);
      if (C->getType()->isFloatingPointTy())
        return ConstantFP::get(C->getType(), 0.);
      if (C->getType() == LowRawPtrTy)
        return LowWideNull;
      return ConstantAggregateZero::get(C->getType());
    }
    
    if (isa<ConstantPointerNull>(C))
      return LowWideNull;

    if (isa<ConstantAggregateZero>(C))
      return ConstantAggregateZero::get(lowerType(C->getType()));

    if (GlobalValue* G = dyn_cast<GlobalValue>(C)) {
      assert(!shouldPassThrough(G));
      switch (RM) {
      case ResultMode::NeedFullConstant:
        return nullptr;
      case ResultMode::NeedConstantWithPtrPlaceholders:
        return LowWideNull;
      }
      llvm_unreachable("should not get here");
    }

    if (isa<ConstantData>(C))
      return C;

    if (ConstantArray* CA = dyn_cast<ConstantArray>(C)) {
      std::vector<Constant*> Args;
      for (size_t Index = 0; Index < CA->getNumOperands(); ++Index) {
        Constant* LowC = tryLowerConstantToConstant(CA->getOperand(Index), RM);
        if (!LowC)
          return nullptr;
        Args.push_back(LowC);
      }
      return ConstantArray::get(cast<ArrayType>(lowerType(CA->getType())), Args);
    }
    if (ConstantStruct* CS = dyn_cast<ConstantStruct>(C)) {
      if (verbose)
        errs() << "Dealing with CS = " << *CS << "\n";
      std::vector<Constant*> Args;
      for (size_t Index = 0; Index < CS->getNumOperands(); ++Index) {
        Constant* LowC = tryLowerConstantToConstant(CS->getOperand(Index), RM);
        if (!LowC)
          return nullptr;
        if (verbose)
          errs() << "Index = " << Index << ", LowC = " << *LowC << "\n";
        Args.push_back(LowC);
      }
      return ConstantStruct::get(cast<StructType>(lowerType(CS->getType())), Args);
    }
    if (ConstantVector* CV = dyn_cast<ConstantVector>(C)) {
      std::vector<Constant*> Args;
      for (size_t Index = 0; Index < CV->getNumOperands(); ++Index) {
        Constant* LowC = tryLowerConstantToConstant(CV->getOperand(Index), RM);
        if (!LowC)
          return nullptr;
        Args.push_back(LowC);
      }
      return ConstantVector::get(Args);
    }

    assert(isa<ConstantExpr>(C));
    ConstantExpr* CE = cast<ConstantExpr>(C);

    if (verbose)
      errs() << "Lowering CE = " << *CE << "\n";
    switch (RM) {
    case ResultMode::NeedFullConstant:
      return nullptr;
    case ResultMode::NeedConstantWithPtrPlaceholders:
      if (isa<IntegerType>(CE->getType()))
        return ConstantInt::get(CE->getType(), 0);
      if (CE->getType() == LowRawPtrTy)
        return LowWideNull;
      
      llvm_unreachable("wtf kind of CE is that");
      return nullptr;
    }
    llvm_unreachable("bad RM");
  }

  ConstantTarget constexprRecurse(Constant* C) {
    assert(C->getType() != LowWidePtrTy);
    
    if (GlobalValue* G = dyn_cast<GlobalValue>(C)) {
      assert(!shouldPassThrough(G));
      assert(!Getters.count(G));
      assert(GlobalToGetter.count(G));
      Function* Getter = GlobalToGetter[G];
      return ConstantTarget(ConstantKind::Global, Getter);
    }

    if (ConstantExpr* CE = dyn_cast<ConstantExpr>(C)) {
      switch (CE->getOpcode()) {
      case Instruction::GetElementPtr: {
        ConstantTarget Target = constexprRecurse(CE->getOperand(0));
        APInt OffsetAP(64, 0, false);
        GetElementPtrInst* GEP = cast<GetElementPtrInst>(CE->getAsInstruction());
        bool result = GEP->accumulateConstantOffset(DL, OffsetAP);
        delete GEP;
        if (!result)
          return ConstantTarget();
        uint64_t Offset = OffsetAP.getZExtValue();
        Constant* CS = ConstantStruct::get(
          ConstexprNodeTy,
          { ConstantInt::get(Int32Ty, static_cast<unsigned>(ConstexprOpcode::AddPtrImmediate)),
            ConstantInt::get(Int32Ty, static_cast<unsigned>(Target.Kind)),
            Target.Target,
            ConstantInt::get(IntPtrTy, Offset) });
        GlobalVariable* ExprG = new GlobalVariable(
          M, ConstexprNodeTy, true, GlobalVariable::PrivateLinkage, CS, "filc_constexpr_gep_node");
        return ConstantTarget(ConstantKind::Expr, ExprG);
      }
      default:
        return ConstantTarget();
      }
    }

    return ConstantTarget();
  }

  bool computeConstantRelocations(Constant* C, std::vector<ConstantRelocation>& Result, size_t Offset = 0) {
    assert(C->getType() != LowWidePtrTy);

    if (ConstantTarget CT = constexprRecurse(C)) {
      assert(!(Offset % WordSize));
      Result.push_back(ConstantRelocation(Offset, CT.Kind, CT.Target));
      return true;
    }
    
    assert(!isa<GlobalValue>(C)); // Should have been caught by constexprRecurse.

    if (isa<UndefValue>(C))
      return true;
    
    if (isa<ConstantPointerNull>(C))
      return true;

    if (isa<ConstantAggregateZero>(C))
      return true;

    if (isa<ConstantData>(C))
      return true;

    if (ConstantArray* CA = dyn_cast<ConstantArray>(C)) {
      size_t ElementSize = DL.getTypeStoreSize(cast<ArrayType>(lowerType(CA->getType()))->getElementType());
      for (size_t Index = 0; Index < CA->getNumOperands(); ++Index) {
        if (!computeConstantRelocations(CA->getOperand(Index), Result, Offset + Index * ElementSize))
          return false;
      }
      return true;
    }
    if (ConstantStruct* CS = dyn_cast<ConstantStruct>(C)) {
      if (verbose)
        errs() << "Dealing with CS = " << *CS << "\n";
      const StructLayout* SL = DL.getStructLayout(cast<StructType>(lowerType(CS->getType())));
      for (size_t Index = 0; Index < CS->getNumOperands(); ++Index) {
        if (!computeConstantRelocations(
              CS->getOperand(Index), Result, Offset + SL->getElementOffset(Index)))
          return false;
      }
      return true;
    }
    if (ConstantVector* CV = dyn_cast<ConstantVector>(C)) {
      size_t ElementSize = DL.getTypeStoreSize(
        cast<VectorType>(lowerType(CV->getType()))->getElementType());
      for (size_t Index = 0; Index < CV->getNumOperands(); ++Index) {
        if (!computeConstantRelocations(CV->getOperand(Index), Result, Offset + Index * ElementSize))
          return false;
      }
      return true;
    }

    assert(isa<ConstantExpr>(C));
    if (verbose)
      errs() << "Failing to handle CE: " << *C << "\n";
    return false;
  }

  Value* lowerConstant(Constant* C, Instruction* InsertBefore, Value* InitializationContext) {
    if (ultraVerbose)
      errs() << "lowerConstant(" << *C << ", ..., " << *InitializationContext << ")\n";
    assert(C->getType() != LowWidePtrTy);

    if (Constant* LowC = tryLowerConstantToConstant(C))
      return LowC;
    
    if (isa<UndefValue>(C)) {
      if (isa<IntegerType>(C->getType()))
        return ConstantInt::get(C->getType(), 0);
      if (C->getType()->isFloatingPointTy())
        return ConstantFP::get(C->getType(), 0.);
      if (C->getType() == LowRawPtrTy)
        return LowWideNull;
      return ConstantAggregateZero::get(C->getType());
    }
    
    if (isa<ConstantPointerNull>(C))
      return LowWideNull;

    if (isa<ConstantAggregateZero>(C))
      return ConstantAggregateZero::get(lowerType(C->getType()));

    if (GlobalValue* G = dyn_cast<GlobalValue>(C)) {
      assert(!shouldPassThrough(G));
      assert(!GlobalToGetter.count(nullptr));
      assert(!Getters.count(nullptr));
      assert(!Getters.count(G));
      assert(GlobalToGetter.count(G));
      Function* Getter = GlobalToGetter[G];
      assert(Getter);
      Instruction* Result = CallInst::Create(
        GlobalGetterTy, Getter, { InitializationContext }, "filc_call_getter", InsertBefore);
      Result->setDebugLoc(InsertBefore->getDebugLoc());
      return Result;
    }

    if (isa<ConstantData>(C))
      return C;

    if (ConstantArray* CA = dyn_cast<ConstantArray>(C)) {
      Value* Result = UndefValue::get(lowerType(CA->getType()));
      for (size_t Index = 0; Index < CA->getNumOperands(); ++Index) {
        Instruction* Insert = InsertValueInst::Create(
          Result, lowerConstant(CA->getOperand(Index), InsertBefore, InitializationContext),
          static_cast<unsigned>(Index), "filc_insert_array", InsertBefore);
        Insert->setDebugLoc(InsertBefore->getDebugLoc());
        Result = Insert;
      }
      return Result;
    }
    if (ConstantStruct* CS = dyn_cast<ConstantStruct>(C)) {
      if (verbose)
        errs() << "Dealing with CS = " << *CS << "\n";
      Value* Result = UndefValue::get(lowerType(CS->getType()));
      for (size_t Index = 0; Index < CS->getNumOperands(); ++Index) {
        Value* LowC = lowerConstant(CS->getOperand(Index), InsertBefore, InitializationContext);
        if (verbose)
          errs() << "Index = " << Index << ", LowC = " << *LowC << "\n";
        Instruction* Insert = InsertValueInst::Create(
          Result, LowC, static_cast<unsigned>(Index), "filc_insert_struct", InsertBefore);
        Insert->setDebugLoc(InsertBefore->getDebugLoc());
        Result = Insert;
      }
      return Result;
    }
    if (ConstantVector* CV = dyn_cast<ConstantVector>(C)) {
      Value* Result = UndefValue::get(lowerType(CV->getType()));
      for (size_t Index = 0; Index < CV->getNumOperands(); ++Index) {
        Instruction* Insert = InsertElementInst::Create(
          Result, lowerConstant(CV->getOperand(Index), InsertBefore, InitializationContext),
          ConstantInt::get(IntPtrTy, Index), "filc_insert_vector", InsertBefore);
        Insert->setDebugLoc(InsertBefore->getDebugLoc());
        Result = Insert;
      }
      return Result;
    }

    assert(isa<ConstantExpr>(C));
    ConstantExpr* CE = cast<ConstantExpr>(C);

    if (verbose)
      errs() << "Lowering CE = " << *CE << "\n";

    Instruction* CEInst = CE->getAsInstruction(InsertBefore);
    CEInst->setDebugLoc(InsertBefore->getDebugLoc());

    // I am the worst compiler programmer.
    StoreInst* DummyUser = new StoreInst(
      CEInst, LowRawNull, false, Align(), AtomicOrdering::NotAtomic, SyncScope::System);
    lowerInstruction(CEInst, InitializationContext);
    Value* Result = DummyUser->getOperand(0);
    delete DummyUser;
    return Result;
  }

  Value* castInt(Value* V, Type* T, Instruction *InsertionPoint) {
    if (V->getType() == T)
      return V;
    Instruction* Result =
      CastInst::CreateIntegerCast(V, T, false, "filc_makeintptr", InsertionPoint);
    Result->setDebugLoc(InsertionPoint->getDebugLoc());
    return Result;
  }

  Value* makeIntPtr(Value* V, Instruction *InsertionPoint) {
    return castInt(V, IntPtrTy, InsertionPoint);
  }

  template<typename Func>
  void hackRAUW(Value* V, const Func& GetNewValue) {
    assert(!Dummy->getNumUses());
    V->replaceAllUsesWith(Dummy);
    Dummy->replaceAllUsesWith(GetNewValue());
  }

  void captureTypesIfNecessary(Instruction* I) {
    if (StoreInst* SI = dyn_cast<StoreInst>(I)) {
      InstLowTypes[I] = lowerType(SI->getValueOperand()->getType());
      return;
    }

    if (AtomicCmpXchgInst* AI = dyn_cast<AtomicCmpXchgInst>(I)) {
      InstLowTypes[I] = lowerType(AI->getNewValOperand()->getType());
      return;
    }

    if (AtomicRMWInst* AI = dyn_cast<AtomicRMWInst>(I)) {
      InstLowTypes[I] = lowerType(AI->getValOperand()->getType());
      return;
    }

    if (CallInst* CI = dyn_cast<CallInst>(I)) {
      std::vector<Type*> Types;
      for (size_t Index = 0; Index < CI->arg_size(); ++Index)
        Types.push_back(lowerType(CI->getArgOperand(Index)->getType()));
      InstLowTypeVectors[I] = std::move(Types);
      return;
    }
  }

  void lowerConstantOperand(Use& U, Instruction* I, Value* InitializationContext) {
    assert(!isa<PHINode>(I));
    if (Constant* C = dyn_cast<Constant>(U)) {
      if (ultraVerbose)
        errs() << "Got U = " << *U << ", C = " << *C << "\n";
      Value* NewC = lowerConstant(C, I, InitializationContext);
      if (ultraVerbose)
        errs() << "Got NewC = " << *NewC <<"\n";
      U = NewC;
    } else if (Argument* A = dyn_cast<Argument>(U)) {
      if (ultraVerbose) {
        errs() << "A = " << *A << "\n";
        errs() << "A->getArgNo() == " << A->getArgNo() << "\n";
        errs() << "Args[A->getArgNo()] == " << *Args[A->getArgNo()] << "\n";
      }
      U = Args[A->getArgNo()];
    }
  }

  void lowerConstantOperands(Instruction* I, Value* InitializationContext) {
    if (verbose)
      errs() << "Before arg lowering: " << *I << "\n";

    for (unsigned Index = I->getNumOperands(); Index--;) {
      Use& U = I->getOperandUse(Index);
      lowerConstantOperand(U, I, InitializationContext);
      if (ultraVerbose)
        errs() << "After Index = " << Index << ", I = " << *I << "\n";
    }
    
    if (verbose)
      errs() << "After arg lowering: " << *I << "\n";
  }

  bool earlyLowerInstruction(Instruction* I) {
    if (verbose)
      errs() << "Early lowering: " << *I << "\n";

    if (IntrinsicInst* II = dyn_cast<IntrinsicInst>(I)) {
      if (verbose)
        errs() << "It's an intrinsic.\n";
      switch (II->getIntrinsicID()) {
      case Intrinsic::memset:
      case Intrinsic::memset_inline:
        lowerConstantOperand(II->getArgOperandUse(0), I, LowRawNull);
        lowerConstantOperand(II->getArgOperandUse(1), I, LowRawNull);
        lowerConstantOperand(II->getArgOperandUse(2), I, LowRawNull);
        if (hasPtrsForCheck(II->getArgOperand(0)->getType())) {
          Instruction* CI = CallInst::Create(
            Memset,
            { MyThread, II->getArgOperand(0), castInt(II->getArgOperand(1), Int32Ty, II),
              makeIntPtr(II->getArgOperand(2), II), getOrigin(II->getDebugLoc()) });
          ReplaceInstWithInst(II, CI);
        }
        return true;
      case Intrinsic::memcpy:
      case Intrinsic::memcpy_inline:
      case Intrinsic::memmove:
        lowerConstantOperand(II->getArgOperandUse(0), I, LowRawNull);
        lowerConstantOperand(II->getArgOperandUse(1), I, LowRawNull);
        lowerConstantOperand(II->getArgOperandUse(2), I, LowRawNull);
        if (hasPtrsForCheck(II->getArgOperand(0)->getType())) {
          assert(hasPtrsForCheck(II->getArgOperand(1)->getType()));
          Instruction* CI = CallInst::Create(
            Memmove,
            { MyThread, II->getArgOperand(0), II->getArgOperand(1), makeIntPtr(II->getArgOperand(2), II),
              getOrigin(II->getDebugLoc()) });
          ReplaceInstWithInst(II, CI);
        }
        return true;

      case Intrinsic::lifetime_start:
      case Intrinsic::lifetime_end:
        II->eraseFromParent();
        return true;

      case Intrinsic::vastart:
        lowerConstantOperand(II->getArgOperandUse(0), I, LowRawNull);
        checkWritePtr(II->getArgOperand(0), II);
        storePtr(ArgBufferPtr, ptrPtr(II->getArgOperand(0), II), StoreKind::Barriered, II);
        II->eraseFromParent();
        return true;
        
      case Intrinsic::vacopy: {
        lowerConstantOperand(II->getArgOperandUse(0), I, LowRawNull);
        lowerConstantOperand(II->getArgOperandUse(1), I, LowRawNull);
        checkWritePtr(II->getArgOperand(0), II);
        checkReadPtr(II->getArgOperand(1), II);
        Value* Load = loadPtr(ptrPtr(II->getArgOperand(1), II), II);
        storePtr(Load, ptrPtr(II->getArgOperand(0), II), StoreKind::Barriered, II);
        II->eraseFromParent();
        return true;
      }
        
      case Intrinsic::vaend:
        II->eraseFromParent();
        return true;

      case Intrinsic::stacksave:
        // Stupidly, in earlyLower, we have to make sure we don't RAUW with a constant, because that
        // would later confuse lowerConstant. WTF.
        II->replaceAllUsesWith(badPtr(LowRawNull, II));
        II->eraseFromParent();
        return true;

      case Intrinsic::stackrestore:
        II->eraseFromParent();
        return true;

      case Intrinsic::assume:
        II->eraseFromParent();
        return true;

      default:
        if (!II->getCalledFunction()->doesNotAccessMemory()
            && !isa<ConstrainedFPIntrinsic>(II)) {
          if (verbose)
            llvm::errs() << "Unhandled intrinsic: " << *II << "\n";
          std::string str;
          raw_string_ostream outs(str);
          outs << "Unhandled intrinsic: " << *II;
          CallInst::Create(Error, { getString(str), getOrigin(I->getDebugLoc()) }, "", II)
            ->setDebugLoc(II->getDebugLoc());
        }
        for (Use& U : II->data_ops()) {
          lowerConstantOperand(U, I, LowRawNull);
          if (hasPtrsForCheck(U->getType()))
            U = ptrPtr(U, II);
        }
        if (hasPtrsForCheck(II->getType()))
          hackRAUW(II, [&] () { return badPtr(II, II->getNextNode()); });
        return true;
      }
    }
    
    if (CallInst* CI = dyn_cast<CallInst>(I)) {
      if (verbose) {
        errs() << "It's a call!\n";
        errs() << "Callee = " << CI->getCalledOperand() << "\n";
        if (CI->getCalledOperand())
          errs() << "Callee name = " << CI->getCalledOperand()->getName() << "\n";
      }

      if (Function* F = dyn_cast<Function>(CI->getCalledOperand())) {
        if (shouldPassThrough(F))
          return true;
      }
    }
    
    return false;
  }
  
  // This lowers the instruction "in place", so all references to it are fixed up after this runs.
  void lowerInstruction(Instruction *I, Value* InitializationContext) {
    if (verbose)
      errs() << "Lowering: " << *I << "\n";

    if (PHINode* P = dyn_cast<PHINode>(I)) {
      assert(InitializationContext == LowRawNull);
      for (unsigned Index = P->getNumIncomingValues(); Index--;) {
        lowerConstantOperand(
          P->getOperandUse(Index), P->getIncomingBlock(Index)->getTerminator(), LowRawNull);
      }
      P->mutateType(lowerType(P->getType()));
      return;
    }

    lowerConstantOperands(I, InitializationContext);
    
    if (AllocaInst* AI = dyn_cast<AllocaInst>(I)) {
      if (!AI->hasNUsesOrMore(1)) {
        // By this point we may have dead allocas, due to earlyLowerInstruction. Only happens for allocas
        // used as type hacks for stdfil API.
        return;
      }
      
      Type* LowT = lowerType(AI->getAllocatedType());
      Value* Length = AI->getArraySize();
      if (Length->getType() != IntPtrTy) {
        Instruction* ZExt = new ZExtInst(Length, IntPtrTy, "filc_alloca_length_zext", AI);
        ZExt->setDebugLoc(AI->getDebugLoc());
        Length = ZExt;
      }
      Instruction* Size = BinaryOperator::Create(
        Instruction::Mul, Length, ConstantInt::get(IntPtrTy, DL.getTypeStoreSize(LowT)),
        "filc_alloca_size", AI);
      Size->setDebugLoc(AI->getDebugLoc());
      AI->replaceAllUsesWith(allocate(Size, DL.getABITypeAlign(LowT).value(), AI));
      AI->eraseFromParent();
      return;
    }

    if (LoadInst* LI = dyn_cast<LoadInst>(I)) {
      Type *LowT = lowerType(LI->getType());
      Value* HighP = LI->getPointerOperand();
      Value* Result = loadValueRecurse(
        LowT, HighP, ptrPtr(HighP, LI),
        LI->isVolatile(), LI->getAlign(), LI->getOrdering(), LI->getSyncScopeID(),
        LI);
      LI->replaceAllUsesWith(Result);
      LI->eraseFromParent();
      return;
    }

    if (StoreInst* SI = dyn_cast<StoreInst>(I)) {
      Value* HighP = SI->getPointerOperand();
      storeValueRecurse(
        InstLowTypes[SI], HighP, SI->getValueOperand(), ptrPtr(HighP, SI),
        SI->isVolatile(), SI->getAlign(), SI->getOrdering(), SI->getSyncScopeID(),
        StoreKind::Barriered, SI);
      SI->eraseFromParent();
      return;
    }

    if (FenceInst* FI = dyn_cast<FenceInst>(I)) {
      // We don't need to do anything because it doesn't take operands.
      return;
    }

    if (AtomicCmpXchgInst* AI = dyn_cast<AtomicCmpXchgInst>(I)) {
      assert(!hasPtrsForCheck(InstLowTypes[AI]));
      Value* LowP = prepareForAccess(InstLowTypes[AI], AI->getPointerOperand(), AccessKind::Write, AI);
      AI->getOperandUse(AtomicCmpXchgInst::getPointerOperandIndex()) = LowP;
      return;
    }

    if (AtomicRMWInst* AI = dyn_cast<AtomicRMWInst>(I)) {
      assert(!hasPtrsForCheck(InstLowTypes[AI]));
      AI->getOperandUse(AtomicRMWInst::getPointerOperandIndex()) =
        prepareForAccess(AI->getValOperand()->getType(), AI->getPointerOperand(), AccessKind::Write, AI);
      return;
    }

    if (GetElementPtrInst* GI = dyn_cast<GetElementPtrInst>(I)) {
      GI->setSourceElementType(lowerType(GI->getSourceElementType()));
      GI->setResultElementType(lowerType(GI->getResultElementType()));
      Value* HighP = GI->getOperand(0);
      GI->getOperandUse(0) = ptrPtr(HighP, GI);
      hackRAUW(GI, [&] () { return ptrWithPtr(HighP, GI, GI->getNextNode()); });
      return;
    }

    if (ICmpInst* CI = dyn_cast<ICmpInst>(I)) {
      if (hasPtrsForCheck(CI->getOperand(0)->getType())) {
        CI->getOperandUse(0) = ptrPtr(CI->getOperand(0), CI);
        CI->getOperandUse(1) = ptrPtr(CI->getOperand(1), CI);
      }
      return;
    }

    if (isa<FCmpInst>(I) ||
        isa<BranchInst>(I) ||
        isa<SwitchInst>(I) ||
        isa<TruncInst>(I) ||
        isa<ZExtInst>(I) ||
        isa<SExtInst>(I) ||
        isa<FPTruncInst>(I) ||
        isa<FPExtInst>(I) ||
        isa<UIToFPInst>(I) ||
        isa<SIToFPInst>(I) ||
        isa<FPToUIInst>(I) ||
        isa<FPToSIInst>(I) ||
        isa<BinaryOperator>(I) ||
        isa<UnaryOperator>(I)) {
      // We're gucci.
      return;
    }

    if (isa<ReturnInst>(I)) {
      if (OldF->getReturnType() != VoidTy)
        ReturnPhi->addIncoming(I->getOperand(0), I->getParent());
      ReplaceInstWithInst(I, BranchInst::Create(ReturnB));
      return;
    }

    if (InvokeInst* II = dyn_cast<InvokeInst>(I)) {
      llvm_unreachable("Don't support InvokeInst yet");
      return;
    }
    
    if (CallInst* CI = dyn_cast<CallInst>(I)) {
      if (CI->isInlineAsm()) {
        std::string str;
        raw_string_ostream outs(str);
        outs << "Cannot handle inline asm: " << *CI;
        CallInst::Create(Error, { getString(str), getOrigin(I->getDebugLoc()) }, "", I)
          ->setDebugLoc(I->getDebugLoc());
        if (I->getType() != VoidTy) {
          // We need to produce something to RAUW the call with, but it cannot be a constant, since
          // that would upset lowerConstant.
          Type* LowT = lowerType(I->getType());
          LoadInst* LI = new LoadInst(LowT, LowRawNull, "filc_fake_load", I);
          LI->setDebugLoc(I->getDebugLoc());
          CI->replaceAllUsesWith(LI);
        }
        CI->eraseFromParent();
        return;
      }

      if (verbose)
        errs() << "Dealing with called operand: " << *CI->getCalledOperand() << "\n";

      if (Function* F = dyn_cast<Function>(CI->getCalledOperand()))
        assert(!shouldPassThrough(F));

      Instruction* OriginPtr = GetElementPtrInst::Create(
        FrameTy, Frame, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) },
        "filc_frame_origin_ptr", CI);
      OriginPtr->setDebugLoc(CI->getDebugLoc());
      (new StoreInst(getOrigin(CI->getDebugLoc()), OriginPtr, CI))->setDebugLoc(CI->getDebugLoc());
      
      CallInst::Create(
        CheckFunctionCall, { CI->getCalledOperand(), getOrigin(CI->getDebugLoc()) }, "", CI)
        ->setDebugLoc(CI->getDebugLoc());

      Value* ArgBufferWidePtrValue;
      Value* ArgBufferRawPtrValue;

      FunctionType *FT = CI->getFunctionType();
      
      if (CI->arg_size()) {
        std::vector<Type*> ArgTypes = InstLowTypeVectors[CI];
        std::vector<size_t> Offsets;
        size_t Size = 0;
        size_t Alignment = 1;
        for (size_t Index = 0; Index < CI->arg_size(); ++Index) {
          Type* LowT = ArgTypes[Index];
          size_t ThisSize = DL.getTypeStoreSize(LowT);
          size_t ThisAlignment = DL.getABITypeAlign(LowT).value();
          Size = (Size + ThisAlignment - 1) / ThisAlignment * ThisAlignment;
          Alignment = std::max(Alignment, ThisAlignment);
          Offsets.push_back(Size);
          Size += ThisSize;
        }

        Value* ArgBufferObject = allocateObject(ConstantInt::get(IntPtrTy, Size), Alignment, CI);
        recordObjectAtIndex(ArgBufferObject, ScratchObjectIndex, CI);
        ArgBufferWidePtrValue = ptrWithObject(ArgBufferObject, CI);
        ArgBufferRawPtrValue = ptrPtr(ArgBufferWidePtrValue, CI);
        assert(FT->getNumParams() <= CI->arg_size());
        assert(FT->getNumParams() == CI->arg_size() || FT->isVarArg());
        for (size_t Index = 0; Index < CI->arg_size(); ++Index) {
          Value* Arg = CI->getArgOperand(Index);
          Type* LowT = ArgTypes[Index];
          assert(Arg->getType() == LowT || lowerType(Arg->getType()) == LowT);
          assert(Index < Offsets.size());
          Instruction* ArgSlotPtr = GetElementPtrInst::Create(
            Int8Ty, ArgBufferRawPtrValue,
            { ConstantInt::get(IntPtrTy, Offsets[Index]) }, "filc_arg_slot", CI);
          ArgSlotPtr->setDebugLoc(CI->getDebugLoc());
          storeValueRecurse(
            LowT, ArgBufferWidePtrValue, Arg, ArgSlotPtr, false, DL.getABITypeAlign(LowT),
            AtomicOrdering::NotAtomic, SyncScope::System, StoreKind::Barriered, CI);
        }
      } else {
        ArgBufferWidePtrValue = LowWideNull;
        ArgBufferRawPtrValue = LowRawNull;
      }

      Type* LowRetT = lowerType(FT->getReturnType());
      size_t RetPayloadSize;
      size_t RetAlign;
      if (LowRetT == VoidTy) {
        RetPayloadSize = 0;
        RetAlign = 1;
      } else {
        RetPayloadSize = DL.getTypeStoreSize(LowRetT);
        RetAlign = DL.getABITypeAlign(LowRetT).value();
      }
      if (!hasPtrsForCheck(LowRetT))
        RetPayloadSize = std::max(static_cast<size_t>(16), RetPayloadSize);
      RetAlign = std::max(WordSize, RetAlign);
      RetPayloadSize = (RetPayloadSize + RetAlign - 1) / RetAlign * RetAlign;
      assert(!(RetPayloadSize % WordSize));

      size_t RetObjectSize = 17 + RetPayloadSize / WordSize;
      size_t RetPayloadOffset = (RetObjectSize + RetAlign - 1) / RetAlign * RetAlign;
      size_t RetSize = RetPayloadOffset + RetPayloadSize;
      
      ReturnBufferSize = std::max(ReturnBufferSize, RetSize);
      ReturnBufferAlignment = std::max(ReturnBufferAlignment, static_cast<size_t>(RetAlign));

      Instruction* ClearRetBuffer = CallInst::Create(
        RealMemset,
        { FutureReturnBuffer, ConstantInt::get(Int8Ty, 0), ConstantInt::get(IntPtrTy, RetSize),
          ConstantInt::get(Int1Ty, false) }, "", CI);
      ClearRetBuffer->setDebugLoc(CI->getDebugLoc());

      Instruction* RetBufferPayload = GetElementPtrInst::Create(
        Int8Ty, FutureReturnBuffer, { ConstantInt::get(IntPtrTy, RetPayloadOffset) },
        "filc_return_payload", CI);
      RetBufferPayload->setDebugLoc(CI->getDebugLoc());

      Instruction* RetBufferUpper = GetElementPtrInst::Create(
        Int8Ty, RetBufferPayload, { ConstantInt::get(IntPtrTy, RetPayloadSize) },
        "filc_return_upper", CI);
      RetBufferUpper->setDebugLoc(CI->getDebugLoc());

      (new StoreInst(RetBufferPayload, objectLowerPtr(FutureReturnBuffer, CI), CI))
        ->setDebugLoc(CI->getDebugLoc());
      (new StoreInst(RetBufferUpper, objectUpperPtr(FutureReturnBuffer, CI), CI))
        ->setDebugLoc(CI->getDebugLoc());
      (new StoreInst(
        ConstantInt::get(Int16Ty, ObjectFlagReturnBuffer), objectFlagsPtr(FutureReturnBuffer, CI), CI))
        ->setDebugLoc(CI->getDebugLoc());

      Value* RetPtr = createPtr(FutureReturnBuffer, RetBufferPayload, CI);

      assert(!CI->hasOperandBundles());
      CallInst::Create(
        PizlonatedFuncTy, ptrPtr(CI->getCalledOperand(), CI),
        { MyThread, ArgBufferWidePtrValue, RetPtr }, "", CI);

      if (LowRetT != VoidTy) {
        CI->replaceAllUsesWith(
          loadValueRecurse(
            LowRetT, RetPtr, ptrPtr(RetPtr, CI), false, DL.getABITypeAlign(LowRetT),
            AtomicOrdering::NotAtomic, SyncScope::System, CI));
      }
      CI->eraseFromParent();
      return;
    }

    if (VAArgInst* VI = dyn_cast<VAArgInst>(I)) {
      Type* T = VI->getType();
      Type* LowT = lowerType(T);
      size_t Size = DL.getTypeStoreSize(LowT);
      size_t Alignment = DL.getABITypeAlign(LowT).value();
      CallInst* Call = CallInst::Create(
        GetNextBytesForVAArg,
        { MyThread, VI->getPointerOperand(), ConstantInt::get(IntPtrTy, Size),
          ConstantInt::get(IntPtrTy, Alignment), getOrigin(VI->getDebugLoc()) },
        "filc_va_arg", VI);
      Call->setDebugLoc(VI->getDebugLoc());
      Value* Load = loadValueRecurse(
        LowT, Call, ptrPtr(Call, VI), false, DL.getABITypeAlign(LowT), AtomicOrdering::NotAtomic,
        SyncScope::System, VI);
      VI->replaceAllUsesWith(Load);
      VI->eraseFromParent();
      return;
    }

    if (isa<ExtractElementInst>(I) ||
        isa<InsertElementInst>(I) ||
        isa<ShuffleVectorInst>(I) ||
        isa<ExtractValueInst>(I) ||
        isa<InsertValueInst>(I) ||
        isa<SelectInst>(I)) {
      I->mutateType(lowerType(I->getType()));
      return;
    }

    if (isa<LandingPadInst>(I)) {
      llvm_unreachable("Don't support LandingPad yet");
      return;
    }

    if (isa<IndirectBrInst>(I)) {
      llvm_unreachable("Don't support IndirectBr yet (and maybe never will)");
      return;
    }

    if (isa<CallBrInst>(I)) {
      llvm_unreachable("Don't support CallBr yet (and maybe never will)");
      return;
    }

    if (isa<ResumeInst>(I)) {
      llvm_unreachable("Don't support Resume yet");
      return;
    }

    if (isa<CatchSwitchInst>(I)) {
      llvm_unreachable("Don't support CatchSwitch yet");
      return;
    }

    if (isa<CleanupPadInst>(I)) {
      llvm_unreachable("Don't support CleanupPad yet");
      return;
    }

    if (isa<CatchPadInst>(I)) {
      llvm_unreachable("Don't support CatchPad yet");
      return;
    }

    if (isa<CatchReturnInst>(I)) {
      llvm_unreachable("Don't support CatchReturn yet");
      return;
    }

    if (isa<CleanupReturnInst>(I)) {
      llvm_unreachable("Don't support CleanupReturn yet");
      return;
    }

    if (isa<UnreachableInst>(I)) {
      CallInst::Create(
        Error, { getString("llvm unreachable instruction"), getOrigin(I->getDebugLoc()) }, "", I)
        ->setDebugLoc(I->getDebugLoc());
      return;
    }

    if (isa<IntToPtrInst>(I)) {
      hackRAUW(I, [&] () { return badPtr(I, I->getNextNode()); });
      return;
    }

    if (isa<PtrToIntInst>(I)) {
      I->getOperandUse(0) = ptrPtr(I->getOperand(0), I);
      return;
    }

    if (isa<BitCastInst>(I)) {
      if (hasPtrsForCheck(I->getType())) {
        assert(hasPtrsForCheck(I->getOperand(0)->getType()));
        assert(I->getType() == LowRawPtrTy || I->getType() == LowWidePtrTy);
        assert(I->getOperand(0)->getType() == LowRawPtrTy || I->getOperand(0)->getType() == LowWidePtrTy);
        I->replaceAllUsesWith(I->getOperand(0));
        I->eraseFromParent();
      } else
        assert(!hasPtrsForCheck(I->getOperand(0)->getType()));
      return;
    }

    if (isa<AddrSpaceCastInst>(I)) {
      if (hasPtrsForCheck(I->getType())) {
        if (hasPtrsForCheck(I->getOperand(0)->getType())) {
          I->replaceAllUsesWith(I->getOperand(0));
          I->eraseFromParent();
        } else
          hackRAUW(I, [&] () { return badPtr(I, I->getNextNode()); });
      } else if (hasPtrsForCheck(I->getOperand(0)->getType()))
        I->getOperandUse(0) = ptrPtr(I->getOperand(0), I);
      return;
    }

    if (isa<FreezeInst>(I)) {
      if (hasPtrsForCheck(I->getType())) {
        I->replaceAllUsesWith(LowWideNull);
        I->eraseFromParent();
      }
      return;
    }

    errs() << "Unrecognized instruction: " << *I << "\n";
    llvm_unreachable("Unknown instruction");
  }

  bool shouldPassThrough(Function* F) {
    return (F->getName() == "__divdc3" ||
            F->getName() == "__muldc3" ||
            F->getName() == "__divsc3" ||
            F->getName() == "__mulsc3");
  }

  bool shouldPassThrough(GlobalVariable* G) {
    return (G->getName() == "llvm.global_ctors" ||
            G->getName() == "llvm.global_dtors" ||
            G->getName() == "llvm.used");
  }

  bool shouldPassThrough(GlobalValue* G) {
    if (Function* F = dyn_cast<Function>(G))
      return shouldPassThrough(F);
    if (GlobalVariable* V = dyn_cast<GlobalVariable>(G))
      return shouldPassThrough(V);
    return false;
  }

  void lowerThreadLocals() {
    // - Lower all threadlocal variables to pthread_key_t, which is an i32, and a function that allocates
    //   the initial "value" (i.e. object containing the initial value). I guess that function can call
    //   malloc and just store the value.
    // - For all threadlocal variables, add a global ctor that pthread_key_create's them.
    // - For all threadlocal variables, add a global dtor that pthread_key_delete's them.
    // - Lower all llvm.threadlocal.address calls to pthread_getspecific_with_default_np, which passes
    //   the key and the default value function.
    //
    // Since this runs before any Fil-C transformations, the resulting code will be made memory safe by
    // those later transformations.
    //
    // Possible problem: if I do it this way then I'm almost certainly interacting badly with thread local
    // destruction. In particular: you'll be able to observe the __thread variable's value being reset
    // if you access it from one of your own destructors. Except! It appears that my implementation of
    // thread locals in musl doesn't behave that way - if a threadlocal has no destructor then we never
    // NULL it.

    std::vector<GlobalVariable*> ThreadLocals;

    for (GlobalVariable& G : M.globals()) {
      if (G.isThreadLocal())
        ThreadLocals.push_back(&G);
    }
    for (Function& F : M.functions())
      assert(!F.isThreadLocal());
    for (GlobalAlias& G : M.aliases())
      assert(!G.isThreadLocal()); // We could probably support this.
    assert(M.ifunc_empty());

    if (ThreadLocals.empty())
      return;

    std::unordered_map<GlobalVariable*, GlobalVariable*> ThreadLocalKeyMap;
    std::unordered_map<GlobalVariable*, Function*> ThreadLocalInitializerMap;

    FunctionType* InitializerTy = FunctionType::get(LowRawPtrTy, false);
    FunctionCallee Malloc = M.getOrInsertFunction("malloc", LowRawPtrTy, IntPtrTy);
    FunctionCallee PthreadKeyCreate = M.getOrInsertFunction("pthread_key_create", Int32Ty, LowRawPtrTy, LowRawPtrTy);
    FunctionCallee PthreadGetspecificWithDefaultNP = M.getOrInsertFunction("pthread_getspecific_with_default_np", LowRawPtrTy, Int32Ty, LowRawPtrTy);

    for (GlobalVariable* G : ThreadLocals) {
      GlobalVariable* Key = new GlobalVariable(
        M, Int32Ty, false, G->getLinkage(),
        !G->isDeclaration() ? ConstantInt::get(Int32Ty, 0) : nullptr,
        "__piztk_" + G->getName());
      ThreadLocalKeyMap[G] = Key;

      Function* Initializer = Function::Create(
        InitializerTy, G->getLinkage(), 0, "__pizti_" + G->getName(), &M);
      ThreadLocalInitializerMap[G] = Initializer;

      if (!G->isDeclaration()) {
        BasicBlock* RootBB = BasicBlock::Create(C, "filc_thread_local_initializer_root", Initializer);
        Value* Result = CallInst::Create(
          Malloc,
          { ConstantInt::get(IntPtrTy, DL.getTypeStoreSizeBeforeFilC(G->getInitializer()->getType())) },
          "filc_thread_local_allocate", RootBB);
        new StoreInst(G->getInitializer(), Result, RootBB);
        ReturnInst::Create(C, Result, RootBB);
      }
    }

    Function* Ctor = Function::Create(
      CtorDtorTy, GlobalValue::InternalLinkage, 0, "filc_threadlocal_ctor", &M);
    BasicBlock* RootBB = BasicBlock::Create(C, "filc_threadlocal_ctor_root", Ctor);
    for (GlobalVariable* G : ThreadLocals) {
      GlobalVariable* Key = ThreadLocalKeyMap[G];
      assert(Key);
      CallInst::Create(PthreadKeyCreate, { Key, LowRawNull }, "", RootBB);
    }
    ReturnInst::Create(C, RootBB);
    appendToGlobalCtors(M, Ctor, 0, LowRawNull);

    // FIXME: We aren't currently registering anything to clean up the threadlocals. Oh well?

    for (Function& F : M.functions()) {
      for (BasicBlock& BB : F) {
        std::vector<Instruction*> Insts;
        for (Instruction& I : BB)
          Insts.push_back(&I);
        for (Instruction* I : Insts) {
          IntrinsicInst* II = dyn_cast<IntrinsicInst>(I);
          if (!II)
            continue;

          if (II->getIntrinsicID() != Intrinsic::threadlocal_address)
            continue;

          GlobalVariable* OldKey = cast<GlobalVariable>(II->getArgOperand(0));
          assert(OldKey);
          GlobalVariable* Key = ThreadLocalKeyMap[OldKey];
          Function* Initializer = ThreadLocalInitializerMap[OldKey];
          assert(Key);
          assert(Initializer);

          Instruction* KeyValue = new LoadInst(Int32Ty, Key, "filc_threadlocal_key_load", II);
          KeyValue->setDebugLoc(II->getDebugLoc());
          Instruction* Getspecific = CallInst::Create(
            PthreadGetspecificWithDefaultNP, { KeyValue, Initializer }, "filc_getspecific", II);
          Getspecific->setDebugLoc(II->getDebugLoc());
          II->replaceAllUsesWith(Getspecific);
          II->eraseFromParent();
        }
      }
    }

    for (GlobalVariable* G : ThreadLocals) {
      G->replaceAllUsesWith(LowRawNull);
      G->eraseFromParent();
    }
  }

  void prepare() {
    for (Function& F : M.functions()) {
      for (BasicBlock& BB : F) {
        for (Instruction& I : BB) {
          if (isa<IndirectBrInst>(&I))
            llvm_unreachable("Don't support IndirectBr yet (and maybe never will)");
          if (LoadInst* LI = dyn_cast<LoadInst>(&I))
            assert(!LI->getPointerAddressSpace());
          if (StoreInst* SI = dyn_cast<StoreInst>(&I))
            assert(!SI->getPointerAddressSpace());
          if (AtomicCmpXchgInst* AI = dyn_cast<AtomicCmpXchgInst>(&I))
            assert(!AI->getPointerAddressSpace());
          if (AtomicRMWInst* AI = dyn_cast<AtomicRMWInst>(&I))
            assert(!AI->getPointerAddressSpace());
        }
      }

      SplitAllCriticalEdges(F);
    }
  }

public:
  Pizlonator(Module &M)
    : C(M.getContext()), M(M), DL(const_cast<DataLayout&>(M.getDataLayout())) {
  }

  void run() {
    if (verbose)
      errs() << "Going to town on module:\n" << M << "\n";

    PtrBits = DL.getPointerSizeInBits(TargetAS);
    VoidTy = Type::getVoidTy(C);
    Int1Ty = Type::getInt1Ty(C);
    Int8Ty = Type::getInt8Ty(C);
    Int16Ty = Type::getInt16Ty(C);
    Int32Ty = Type::getInt32Ty(C);
    IntPtrTy = Type::getIntNTy(C, PtrBits);
    assert(IntPtrTy == Type::getInt64Ty(C)); // FilC is 64-bit-only, for now.
    Int128Ty = Type::getInt128Ty(C);
    LowRawPtrTy = PointerType::get(C, TargetAS);
    CtorDtorTy = FunctionType::get(VoidTy, false);
    LowRawNull = ConstantPointerNull::get(LowRawPtrTy);

    lowerThreadLocals();
    
    if (verbose)
      errs() << "Module with lowered thread locals:\n" << M << "\n";

    prepare();

    if (verbose)
      errs() << "Prepared module:\n" << M << "\n";

    FunctionName = "<internal>";
    
    LowWidePtrTy = StructType::create({Int128Ty}, "filc_wide_ptr");
    OriginTy = StructType::create({LowRawPtrTy, LowRawPtrTy, Int32Ty, Int32Ty}, "filc_origin");
    ObjectTy = StructType::create({LowRawPtrTy, LowRawPtrTy, Int16Ty, Int8Ty}, "filc_object");
    FrameTy = StructType::create({LowRawPtrTy, LowRawPtrTy, IntPtrTy, LowRawPtrTy}, "filc_frame");
    ThreadTy = StructType::create({Int8Ty, LowRawPtrTy}, "filc_thread_ish");
    ConstantRelocationTy = StructType::create(
      {IntPtrTy, Int32Ty, LowRawPtrTy}, "filc_constant_relocation");
    ConstexprNodeTy = StructType::create(
      { Int32Ty, Int32Ty, LowRawPtrTy, IntPtrTy}, "filc_constexpr_node");
    // See PIZLONATED_SIGNATURE in filc_runtime.h.
    PizlonatedFuncTy = FunctionType::get(
      VoidTy, { LowRawPtrTy, LowWidePtrTy, LowWidePtrTy }, false);
    GlobalGetterTy = FunctionType::get(LowWidePtrTy, { LowRawPtrTy }, false);

    // Fugc the DSO!
    if (GlobalVariable* DSO = M.getGlobalVariable("__dso_handle")) {
      assert(DSO->isDeclaration());
      DSO->replaceAllUsesWith(LowRawNull);
      DSO->eraseFromParent();
    }
    
    // Capture the set of things that need conversion, before we start adding functions and globals.
    auto CaptureType = [&] (GlobalValue* G) {
      GlobalHighTypes[G] = G->getValueType();
      GlobalLowTypes[G] = lowerType(G->getValueType());
    };

    for (GlobalVariable &G : M.globals()) {
      if (shouldPassThrough(&G))
        continue;
      Globals.push_back(&G);
      CaptureType(&G);
    }
    for (Function &F : M.functions()) {
      if (shouldPassThrough(&F)) {
        if (!F.isDeclaration()) {
          errs() << "Cannot define " << F.getName() << "\n";
          llvm_unreachable("Attempt to define pass-through function.");
        }
        continue;
      }
      Functions.push_back(&F);
      CaptureType(&F);
    }
    for (GlobalAlias &G : M.aliases()) {
      Aliases.push_back(&G);
      CaptureType(&G);
    }
    if (!M.ifunc_empty())
      llvm_unreachable("Don't handle ifuncs yet.");

    LowWideNull = ConstantStruct::get(LowWidePtrTy, { ConstantInt::get(Int128Ty, 0) });
    if (verbose)
      errs() << "LowWideNull = " << *LowWideNull << "\n";

    Dummy = makeDummy(Int32Ty);
    FutureReturnBuffer = makeDummy(LowRawPtrTy);

    Pollcheck = M.getOrInsertFunction("filc_pollcheck_outline", VoidTy, LowRawPtrTy, LowRawPtrTy);
    StoreBarrier = M.getOrInsertFunction("filc_store_barrier_outline", VoidTy, LowRawPtrTy, LowRawPtrTy);
    GetNextBytesForVAArg = M.getOrInsertFunction("filc_get_next_bytes_for_va_arg", LowWidePtrTy, LowRawPtrTy, LowWidePtrTy, IntPtrTy, IntPtrTy, LowRawPtrTy);
    AllocateFunction = M.getOrInsertFunction("filc_allocate_function", LowRawPtrTy, LowRawPtrTy, LowRawPtrTy);
    Allocate = M.getOrInsertFunction("filc_allocate", LowRawPtrTy, LowRawPtrTy, IntPtrTy);
    AllocateWithAlignment = M.getOrInsertFunction("filc_allocate_with_alignment", LowRawPtrTy, LowRawPtrTy, IntPtrTy, IntPtrTy);
    CheckReadInt = M.getOrInsertFunction("filc_check_read_int", VoidTy, LowWidePtrTy, IntPtrTy, LowRawPtrTy);
    CheckReadPtr = M.getOrInsertFunction("filc_check_read_ptr", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckWriteInt = M.getOrInsertFunction("filc_check_write_int", VoidTy, LowWidePtrTy, IntPtrTy, LowRawPtrTy);
    CheckWritePtr = M.getOrInsertFunction("filc_check_write_ptr", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckFunctionCall = M.getOrInsertFunction("filc_check_function_call", VoidTy, LowWidePtrTy, LowRawPtrTy);
    Memset = M.getOrInsertFunction("filc_memset", VoidTy, LowRawPtrTy, LowWidePtrTy, Int32Ty, IntPtrTy, LowRawPtrTy);
    Memmove = M.getOrInsertFunction("filc_memmove", VoidTy, LowRawPtrTy, LowWidePtrTy, LowWidePtrTy, IntPtrTy, LowRawPtrTy);
    GlobalInitializationContextCreate = M.getOrInsertFunction("filc_global_initialization_context_create", LowRawPtrTy, LowRawPtrTy);
    GlobalInitializationContextAdd = M.getOrInsertFunction("filc_global_initialization_context_add", Int1Ty, LowRawPtrTy, LowRawPtrTy, LowRawPtrTy);
    GlobalInitializationContextDestroy = M.getOrInsertFunction("filc_global_initialization_context_destroy", VoidTy, LowRawPtrTy);
    ExecuteConstantRelocations = M.getOrInsertFunction("filc_execute_constant_relocations", VoidTy, LowRawPtrTy, LowRawPtrTy, IntPtrTy, LowRawPtrTy);
    DeferOrRunGlobalCtor = M.getOrInsertFunction("filc_defer_or_run_global_ctor", VoidTy, LowRawPtrTy);
    RunGlobalDtor = M.getOrInsertFunction("filc_run_global_dtor", VoidTy, LowRawPtrTy);
    Error = M.getOrInsertFunction("filc_error", VoidTy, LowRawPtrTy, LowRawPtrTy);
    RealMemset = M.getOrInsertFunction("llvm.memset.p0.i64", VoidTy, LowRawPtrTy, Int8Ty, IntPtrTy, Int1Ty);

    auto FixupTypes = [&] (GlobalValue* G, GlobalValue* NewG) {
      GlobalHighTypes[NewG] = GlobalHighTypes[G];
      GlobalLowTypes[NewG] = GlobalLowTypes[G];
    };

    std::vector<GlobalValue*> ToDelete;
    auto HandleGlobal = [&] (GlobalValue* G) {
      Function* NewF = Function::Create(GlobalGetterTy, G->getLinkage(), G->getAddressSpace(),
                                        "pizlonated_" + G->getName(), &M);
      GlobalToGetter[G] = NewF;
      Getters.insert(NewF);
      FixupTypes(G, NewF);
      ToDelete.push_back(G);
    };
    for (GlobalVariable* G : Globals)
      HandleGlobal(G);
    for (GlobalAlias* G : Aliases)
      HandleGlobal(G);
    for (Function* F : Functions) {
      if (F->isIntrinsic())
        continue;
      HandleGlobal(F);
      if (!F->isDeclaration()) {
        FunctionToHiddenFunction[F] = Function::Create(cast<FunctionType>(lowerType(F->getFunctionType())),
                                                       GlobalValue::InternalLinkage, F->getAddressSpace(),
                                                       "Jf_" + F->getName(), &M);
      }
    }
    
    if (GlobalVariable* GlobalCtors = M.getGlobalVariable("llvm.global_ctors")) {
      ConstantArray* Array = cast<ConstantArray>(GlobalCtors->getInitializer());
      std::vector<Constant*> Args;
      for (size_t Index = 0; Index < Array->getNumOperands(); ++Index) {
        ConstantStruct* Struct = cast<ConstantStruct>(Array->getOperand(Index));
        assert(Struct->getOperand(2) == LowRawNull);
        Function* Ctor = cast<Function>(Struct->getOperand(1));
        Function* HiddenCtor = FunctionToHiddenFunction[Ctor];
        assert(HiddenCtor);
        Function* NewF = Function::Create(
          CtorDtorTy, GlobalValue::InternalLinkage, 0, "filc_ctor_forwarder", &M);
        BasicBlock* RootBB = BasicBlock::Create(C, "filc_ctor_forwarder_root", NewF);
        CallInst::Create(DeferOrRunGlobalCtor, { HiddenCtor }, "", RootBB);
        ReturnInst::Create(C, RootBB);
        Args.push_back(ConstantStruct::get(Struct->getType(), Struct->getOperand(0), NewF, LowRawNull));
      }
      GlobalCtors->setInitializer(ConstantArray::get(Array->getType(), Args));
    }

    // NOTE: This *might* be dead code, since modern C/C++ says that the compiler has to do __cxa_atexit
    // from a global constructor instead of registering a global destructor.
    if (GlobalVariable* GlobalDtors = M.getGlobalVariable("llvm.global_dtors")) {
      ConstantArray* Array = cast<ConstantArray>(GlobalDtors->getInitializer());
      std::vector<Constant*> Args;
      for (size_t Index = 0; Index < Array->getNumOperands(); ++Index) {
        ConstantStruct* Struct = cast<ConstantStruct>(Array->getOperand(Index));
        assert(Struct->getOperand(2) == LowRawNull);
        Function* Dtor = cast<Function>(Struct->getOperand(1));
        Function* HiddenDtor = FunctionToHiddenFunction[Dtor];
        Function* NewF = Function::Create(
          CtorDtorTy, GlobalValue::InternalLinkage, 0, "filc_dtor_forwarder", &M);
        BasicBlock* RootBB = BasicBlock::Create(C, "filc_dtor_forwarder_root", NewF);
        CallInst::Create(RunGlobalDtor, { HiddenDtor }, "", RootBB);
        ReturnInst::Create(C, RootBB);
        Args.push_back(ConstantStruct::get(Struct->getType(), Struct->getOperand(0), NewF, LowRawNull));
      }
      GlobalDtors->setInitializer(ConstantArray::get(Array->getType(), Args));
    }

    if (GlobalVariable* Used = M.getGlobalVariable("llvm.used")) {
      ConstantArray* Array = cast<ConstantArray>(Used->getInitializer());
      std::vector<Constant*> Args;
      for (size_t Index = 0; Index < Array->getNumOperands(); ++Index) {
        // NOTE: This could have a GEP, supposedly. Pretend it can't for now.
        Function* GetterF = GlobalToGetter[cast<GlobalValue>(Array->getOperand(Index))];
        assert(GetterF);
        Args.push_back(GetterF);
      }
      Used->setInitializer(ConstantArray::get(Array->getType(), Args));
    }

    for (GlobalVariable* G : Globals) {
      // FIXME: We can probably make this work, but who gives a fugc for now?
      assert(G->getThreadLocalMode() == GlobalValue::NotThreadLocal);

      Function* NewF = GlobalToGetter[G];
      assert(NewF);
      assert(NewF->isDeclaration());

      if (verbose)
        errs() << "Dealing with global: " << *G << "\n";

      if (G->isDeclaration())
        continue;

      Constant* NewC = paddedConstant(
        tryLowerConstantToConstant(G->getInitializer(), ResultMode::NeedConstantWithPtrPlaceholders));
      assert(NewC);
      GlobalVariable* NewDataG = new GlobalVariable(
        M, NewC->getType(), false, GlobalValue::InternalLinkage, NewC,
        "Jg_" + G->getName());
      assert(NewDataG);
      if (G->getAlign().valueOrOne() < WordSize)
        NewDataG->setAlignment(Align(WordSize));
      else
        NewDataG->setAlignment(G->getAlign().valueOrOne());

      GlobalVariable* NewObjectG = objectGlobalForGlobal(NewDataG, G);
      
      BasicBlock* RootBB = BasicBlock::Create(C, "filc_global_getter_root", NewF);

      GlobalVariable* NewPtrG = new GlobalVariable(
        M, LowWidePtrTy, false, GlobalValue::PrivateLinkage, LowWideNull,
        "filc_gptr_" + G->getName());
      
      BasicBlock* FastBB = BasicBlock::Create(C, "filc_global_getter_fast", NewF);
      BasicBlock* SlowBB = BasicBlock::Create(C, "filc_global_getter_slow", NewF);
      BasicBlock* RecurseBB = BasicBlock::Create(C, "filc_global_getter_recurse", NewF);
      BasicBlock* BuildBB = BasicBlock::Create(C, "filc_global_getter_build", NewF);

      Instruction* Branch = BranchInst::Create(SlowBB, FastBB, UndefValue::get(Int1Ty), RootBB);
      Value* LoadPtr = loadPtr(NewPtrG, Branch);
      Branch->getOperandUse(0) = new ICmpInst(
        Branch, ICmpInst::ICMP_EQ, ptrWord(LoadPtr, Branch), ConstantInt::get(Int128Ty, 0),
        "filc_check_global");

      ReturnInst::Create(C, LoadPtr, FastBB);

      Branch = BranchInst::Create(BuildBB, RecurseBB, UndefValue::get(Int1Ty), SlowBB);
      Instruction* MyInitializationContext = CallInst::Create(
        GlobalInitializationContextCreate, { NewF->getArg(0) }, "filc_context_create",
        Branch);
      Value* Ptr = createPtr(NewObjectG, NewDataG, Branch);
      // NOTE: This function call is necessary even for the cases of globals with no meaningful
      // initializer or an initializer we could just optimize to pure data, since we have to tell the
      // runtime about this global so that the GC can find it.
      Instruction* Add = CallInst::Create(
        GlobalInitializationContextAdd,
        { MyInitializationContext, NewPtrG, NewObjectG },
        "filc_context_add", Branch);
      Branch->getOperandUse(0) = Add;

      CallInst::Create(
        GlobalInitializationContextDestroy, { MyInitializationContext }, "", RecurseBB);
      ReturnInst::Create(C, Ptr, RecurseBB);

      Instruction* Return = ReturnInst::Create(C, Ptr, BuildBB);
      if (verbose)
        errs() << "Lowering constant " << *G->getInitializer() << " with initialization context = " << *MyInitializationContext << "\n";
      std::vector<ConstantRelocation> Relocations;
      if (computeConstantRelocations(G->getInitializer(), Relocations)) {
        if (Relocations.size()) {
          std::vector<Constant*> Constants;
          for (const ConstantRelocation& Relocation : Relocations) {
            Constants.push_back(
              ConstantStruct::get(
                ConstantRelocationTy,
                { ConstantInt::get(IntPtrTy, Relocation.Offset),
                  ConstantInt::get(Int32Ty, static_cast<unsigned>(Relocation.Kind)),
                  Relocation.Target }));
          }
          ArrayType* AT = ArrayType::get(ConstantRelocationTy, Constants.size());
          Constant* CA = ConstantArray::get(AT, Constants);
          GlobalVariable* RelocG = new GlobalVariable(
            M, AT, true, GlobalVariable::PrivateLinkage, CA, "filc_constant_relocations");
          CallInst::Create(
            ExecuteConstantRelocations,
            { NewDataG, RelocG, ConstantInt::get(IntPtrTy, Constants.size()), MyInitializationContext },
            "", Return);
        }
      } else {
        Value* C = lowerConstant(G->getInitializer(), Return, MyInitializationContext);
        new StoreInst(C, NewDataG, Return);
      }
      
      CallInst::Create(GlobalInitializationContextDestroy, { MyInitializationContext }, "", Return);
    }
    for (Function* F : Functions) {
      if (F->isIntrinsic())
        continue;
      
      if (verbose)
        errs() << "Function before lowering: " << *F << "\n";

      if (!F->isDeclaration()) {
        FunctionName = getFunctionName(F);
        OldF = F;
        NewF = FunctionToHiddenFunction[F];
        assert(NewF);
      
        MyThread = NewF->getArg(0);
        FixupTypes(F, NewF);
        std::vector<BasicBlock*> Blocks;
        for (BasicBlock& BB : *F)
          Blocks.push_back(&BB);
        assert(!Blocks.empty());
        ReturnBufferSize = 0;
        ReturnBufferAlignment = 0;
        Args.clear();
        for (BasicBlock* BB : Blocks) {
          BB->removeFromParent();
          BB->insertInto(NewF);
        }
        computeFrameIndexMap(Blocks);
        // Snapshot the instructions before we do crazy stuff.
        std::vector<Instruction*> Instructions;
        for (BasicBlock* BB : Blocks) {
          for (Instruction& I : *BB) {
            Instructions.push_back(&I);
            captureTypesIfNecessary(&I);
          }

          // LMAO who needs backwards edge analysis when you don't give a fugc about perf?
          CallInst::Create(
              Pollcheck, { NewF->getArg(0), getOrigin(BB->getTerminator()->getDebugLoc()) }, "",
              BB->getTerminator());
        }

        ReturnB = BasicBlock::Create(C, "filc_return_block", NewF);
        if (F->getReturnType() != VoidTy)
          ReturnPhi = PHINode::Create(lowerType(F->getReturnType()), 1, "filc_return_value", ReturnB);
        ReturnInst* Return = ReturnInst::Create(C, ReturnB);

        if (F->getReturnType() != VoidTy) {
          Type* LowT = lowerType(F->getReturnType());
          storeValueRecurse(
            LowT, NewF->getArg(2), ReturnPhi, ptrPtr(NewF->getArg(2), Return), false,
            DL.getABITypeAlign(LowT), AtomicOrdering::NotAtomic, SyncScope::System, StoreKind::Unbarriered,
            Return);
        }

        Instruction* InsertionPoint = &*Blocks[0]->getFirstInsertionPt();
        StructType* MyFrameTy = StructType::get(
          C, { LowRawPtrTy, LowRawPtrTy, IntPtrTy, ArrayType::get(LowRawPtrTy, FrameSize) });
        Frame = new AllocaInst(MyFrameTy, 0, "filc_my_frame", InsertionPoint);
        Value* ThreadTopFramePtr = GetElementPtrInst::Create(
          ThreadTy, NewF->getArg(0),
          { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) },
          "filc_thread_top_frame_ptr", InsertionPoint);
        new StoreInst(
          new LoadInst(LowRawPtrTy, ThreadTopFramePtr, "filc_thread_top_frame", InsertionPoint),
          GetElementPtrInst::Create(
            FrameTy, Frame, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 0) },
            "filc_frame_parent_ptr", InsertionPoint),
          InsertionPoint);
        new StoreInst(Frame, ThreadTopFramePtr, InsertionPoint);
        new StoreInst(
          LowRawNull,
          GetElementPtrInst::Create(
            FrameTy, Frame, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) },
            "filc_frame_parent_ptr", InsertionPoint),
          InsertionPoint);
        new StoreInst(
          ConstantInt::get(IntPtrTy, FrameSize),
          GetElementPtrInst::Create(
            FrameTy, Frame, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 2) },
            "filc_frame_parent_ptr", InsertionPoint),
          InsertionPoint);
        for (size_t FrameIndex = FrameSize; FrameIndex--;)
          recordObjectAtIndex(LowRawNull, FrameIndex, InsertionPoint);

        recordObjectAtIndex(ptrObject(NewF->getArg(1), InsertionPoint), MyArgsObjectIndex, InsertionPoint);

        new StoreInst(
          new LoadInst(
            LowRawPtrTy,
            GetElementPtrInst::Create(
              FrameTy, Frame, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 0) },
              "filc_frame_parent_ptr", Return),
            "filc_frame_parent", Return),
          GetElementPtrInst::Create(
            ThreadTy, NewF->getArg(0),
            { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) },
            "filc_thread_top_frame_ptr", Return),
          Return);

        ArgBufferPtr = NewF->getArg(1);
        Value* RawDataPtr = ptrPtr(ArgBufferPtr, InsertionPoint);
        size_t ArgOffset = 0;
        for (unsigned Index = 0; Index < F->getFunctionType()->getNumParams(); ++Index) {
          Type* T = F->getFunctionType()->getParamType(Index);
          Type* LowT = lowerType(T);
          size_t Size = DL.getTypeStoreSize(LowT);
          size_t Alignment = DL.getABITypeAlign(LowT).value();
          ArgOffset = (ArgOffset + Alignment - 1) / Alignment * Alignment;
          Instruction* ArgPtr = GetElementPtrInst::Create(
            Int8Ty, RawDataPtr, { ConstantInt::get(IntPtrTy, ArgOffset) }, "filc_arg_ptr",
            InsertionPoint);
          Value* V = loadValueRecurse(
            LowT, ArgBufferPtr, ArgPtr, false, DL.getABITypeAlign(LowT), AtomicOrdering::NotAtomic,
            SyncScope::System, InsertionPoint);
          recordObjects(OldF->getArg(Index), LowT, V, InsertionPoint);
          Args.push_back(V);
          ArgOffset += Size;
        }
        Instruction* ArgEndPtr = GetElementPtrInst::Create(
          Int8Ty, RawDataPtr, { ConstantInt::get(IntPtrTy, ArgOffset) }, "filc_arg_end_ptr",
          InsertionPoint);
        ArgBufferPtr = ptrWithPtr(ArgBufferPtr, ArgEndPtr, InsertionPoint);

        FirstRealBlock = InsertionPoint->getParent();

        std::vector<PHINode*> Phis;
        for (Instruction* I : Instructions) {
          if (PHINode* Phi = dyn_cast<PHINode>(I))
            Phis.push_back(Phi);
          else if (!Phis.empty()) {
            for (PHINode* Phi : Phis)
              recordObjects(Phi, I);
            Phis.clear();
          }
          if (I->isTerminator())
            assert(I->getType()->isVoidTy());
          else
            recordObjects(I, I->getNextNode());
        }
        erase_if(Instructions, [&] (Instruction* I) { return earlyLowerInstruction(I); });
        for (Instruction* I : Instructions)
          lowerInstruction(I, LowRawNull);

        InsertionPoint = &*Blocks[0]->getFirstInsertionPt();
        if (FutureReturnBuffer->hasNUsesOrMore(1)) {
          assert(ReturnBufferSize);
          assert(ReturnBufferAlignment);
          FutureReturnBuffer->replaceAllUsesWith(
            new AllocaInst(
              Int8Ty, 0, ConstantInt::get(IntPtrTy, ReturnBufferSize), Align(ReturnBufferAlignment),
              "filc_return_buffer", InsertionPoint));
        } else {
          assert(!ReturnBufferSize);
          assert(!ReturnBufferAlignment);
        }
        
        GlobalVariable* NewObjectG = objectGlobalForGlobal(NewF, OldF);
        
        Function* GetterF = GlobalToGetter[OldF];
        assert(GetterF);
        assert(GetterF->isDeclaration());
        BasicBlock* RootBB = BasicBlock::Create(C, "filc_function_getter_root", GetterF);
        Return = ReturnInst::Create(C, UndefValue::get(LowWidePtrTy), RootBB);
        Return->getOperandUse(0) = createPtr(NewObjectG, NewF, Return);
      }
      
      FunctionName = "<internal>";

      if (verbose)
        errs() << "New function: " << *NewF << "\n";
    }
    for (GlobalAlias* G : Aliases) {
      Constant* C = G->getAliasee();
      Function* NewF = GlobalToGetter[G];
      Function* TargetF = GlobalToGetter[cast<GlobalValue>(C)];
      assert(NewF);
      assert(TargetF);
      BasicBlock* BB = BasicBlock::Create(this->C, "filc_alias_global", NewF);
      ReturnInst::Create(
        this->C,
        CallInst::Create(GlobalGetterTy, TargetF, { NewF->getArg(0) },
                         "filc_forward_global", BB),
        BB);
    }

    delete Dummy;
    delete FutureReturnBuffer;

    for (GlobalValue* G : ToDelete)
      G->replaceAllUsesWith(UndefValue::get(G->getType())); // FIXME - should be zero
    for (GlobalValue* G : ToDelete)
      G->eraseFromParent();
    
    if (verbose)
      errs() << "Here's the pizlonated module:\n" << M << "\n";
    verifyModule(M);
  }
};

} // anonymous namespace

PreservedAnalyses FilPizlonatorPass::run(Module &M, ModuleAnalysisManager&) {
  Pizlonator P(M);
  P.run();
  return PreservedAnalyses::none();
}

