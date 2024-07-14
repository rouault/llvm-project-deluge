//===- FilPizlonator.cpp - coolest fugcing pass ever written --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file is a part of Fil-C, a Phil Pizlo Special (TM).
//
// See here:
// https://github.com/pizlonator/llvm-project-deluge/blob/deluge/Manifesto.md
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Instrumentation/FilPizlonator.h"

#include <llvm/Analysis/CFG.h>
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

// This has to match the FilC runtime.

static constexpr size_t GCMinAlign = 16;
static constexpr size_t WordSize = 16;
static constexpr size_t WordSizeShift = 4;

typedef uint8_t WordType;

static constexpr WordType WordTypeUnset = 0;
static constexpr WordType WordTypeInt = 1;
static constexpr WordType WordTypePtr = 2;
static constexpr WordType WordTypeFunction = 4;

static constexpr uint16_t ObjectFlagSpecial = 2;
static constexpr uint16_t ObjectFlagGlobal = 4;
static constexpr uint16_t ObjectFlagReadonly = 16;

static constexpr unsigned NumUnwindRegisters = 2;

static constexpr size_t SpecialObjectSize = 32;

static constexpr uint8_t ThreadStateCheckRequested = 2;
static constexpr uint8_t ThreadStateStopRequested = 4;
static constexpr uint8_t ThreadStateDeferredSignal = 8;

enum class AccessKind {
  Read,
  Write
};

enum class ConstantKind {
  Global,
  Expr
};

enum class JmpBufKind {
  setjmp,
  _setjmp,
  sigsetjmp
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

enum class MemoryKind {
  CC,
  Heap
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

struct FunctionOriginKey {
  FunctionOriginKey() = default;

  FunctionOriginKey(Function* OldF, bool CanCatch)
    : OldF(OldF), CanCatch(CanCatch) {
  }

  Function* OldF { nullptr };
  bool CanCatch { false };

  bool operator==(const FunctionOriginKey& Other) const {
    return OldF == Other.OldF && CanCatch == Other.CanCatch;
  }

  size_t hash() const {
    return std::hash<Function*>()(OldF) + static_cast<size_t>(CanCatch);
  }
};

struct EHDataKey {
  EHDataKey() = default;
  EHDataKey(LandingPadInst* LPI) : LPI(LPI) { }

  LandingPadInst* LPI { nullptr };

  bool operator==(const EHDataKey& Other) const {
    if (!LPI)
      return !Other.LPI;

    if (!Other.LPI)
      return false;

    if (LPI->isCleanup() != Other.LPI->isCleanup())
      return false;

    if (LPI->getNumClauses() != Other.LPI->getNumClauses())
      return false;

    for (unsigned Idx = LPI->getNumClauses(); Idx--;) {
      if (LPI->getClause(Idx) != Other.LPI->getClause(Idx))
        return false;
    }

    return true;
  }

  size_t hash() const {
    if (!LPI)
      return 0;

    size_t Result = 0;
    Result += LPI->isCleanup();
    Result += LPI->getNumClauses();
    for (unsigned Idx = LPI->getNumClauses(); Idx--;)
      Result += std::hash<Constant*>()(LPI->getClause(Idx));
    return Result;
  }
};

struct OriginKey {
  OriginKey() = default;

  OriginKey(Function* OldF, DILocation* DI, bool CanCatch, LandingPadInst* LPI)
    : OldF(OldF), DI(DI), CanCatch(CanCatch), LPI(LPI) {
    if (LPI)
      assert(CanCatch);
  }

  Function* OldF { nullptr };
  DILocation* DI { nullptr };
  bool CanCatch { false };
  LandingPadInst* LPI { nullptr };

  bool operator==(const OriginKey& Other) const {
    return OldF == Other.OldF && DI == Other.DI && CanCatch == Other.CanCatch
      && EHDataKey(LPI) == EHDataKey(Other.LPI);
  }

  size_t hash() const {
    return std::hash<Function*>()(OldF) + std::hash<DILocation*>()(DI) + static_cast<size_t>(CanCatch)
      + EHDataKey(LPI).hash();
  }
};

} // anonymous namespace

template<> struct std::hash<FunctionOriginKey> {
  size_t operator()(const FunctionOriginKey& Key) const {
    return Key.hash();
  }
};

template<> struct std::hash<EHDataKey> {
  size_t operator()(const EHDataKey& Key) const {
    return Key.hash();
  }
};

template<> struct std::hash<OriginKey> {
  size_t operator()(const OriginKey& Key) const {
    return Key.hash();
  }
};

namespace {

static constexpr size_t NumSpecialFrameObjects = 0;

enum class MATokenKind {
  None,
  Error,
  Directive,
  Identifier,
  Comma,
  EndLine
};

inline const char* MATokenKindString(MATokenKind Kind) {
  switch (Kind) {
  case MATokenKind::None:
    return "None";
  case MATokenKind::Error:
    return "Error";
  case MATokenKind::Directive:
    return "Directive";
  case MATokenKind::Identifier:
    return "Identifier";
  case MATokenKind::Comma:
    return "Comma";
  case MATokenKind::EndLine:
    return "EndLine";
  }
  llvm_unreachable("Bad MATokenKind");
  return nullptr;
}

struct MAToken {
  MAToken() = default;

  MAToken(MATokenKind Kind, const std::string& Str): Kind(Kind), Str(Str) {}

  MATokenKind Kind { MATokenKind::None };
  std::string Str;
};

class MATokenizer {
  std::string MA;
  size_t Idx { 0 };

  void skipWhitespace() {
    while (Idx < MA.size() && MA[Idx] != '\n' && isspace(MA[Idx]))
      Idx++;
  }

  void skipID() {
    while (Idx < MA.size() && (isalnum(MA[Idx]) ||
                               MA[Idx] == '_' ||
                               MA[Idx] == '$' ||
                               MA[Idx] == '@' ||
                               MA[Idx] == '.'))
      Idx++;
  }
  
public:
  MATokenizer(const std::string& MA): MA(MA) {}

  bool isAtEnd() const { return Idx >= MA.size(); }

  MAToken getNext() {
    skipWhitespace();
    if (isAtEnd())
      return MAToken();
    if (MA[Idx] == '\n') {
      Idx++;
      return MAToken(MATokenKind::EndLine, "\n");
    }
    if (MA[Idx] == ',') {
      Idx++;
      return MAToken(MATokenKind::Comma, "\n");
    }
    if (MA[Idx] == '.') {
      size_t Start = Idx;
      skipID();
      return MAToken(MATokenKind::Directive, MA.substr(Start, Idx - Start));
    }
    if (isalpha(MA[Idx]) || MA[Idx] == '_') {
      size_t Start = Idx;
      skipID();
      return MAToken(MATokenKind::Identifier, MA.substr(Start, Idx - Start));
    }
    return MAToken(MATokenKind::Error, MA.substr(Idx, MA.size() - Idx));
  }

  MAToken getNextSpecific(MATokenKind Kind) {
    MAToken Tok = getNext();
    if (Tok.Kind != Kind) {
      errs() << "Expected " << MATokenKindString(Kind) << " but got: " << Tok.Str << "\n";
      llvm_unreachable("Error parsing module asm");
    }
    return Tok;
  }
};

class Pizlonator {
  static constexpr unsigned TargetAS = 0;
  
  LLVMContext& C;
  Module &M;
  const DataLayout DLBefore;
  const DataLayout DL;

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
  StructType* FunctionOriginTy;
  StructType* OriginTy;
  StructType* OriginWithEHTy;
  StructType* ObjectTy;
  StructType* CCTypeTy;
  StructType* CCPtrTy;
  StructType* FrameTy;
  StructType* ThreadTy;
  StructType* ConstantRelocationTy;
  StructType* ConstexprNodeTy;
  FunctionType* PizlonatedFuncTy;
  FunctionType* GlobalGetterTy;
  FunctionType* CtorDtorTy;
  FunctionType* SetjmpTy;
  FunctionType* SigsetjmpTy;
  Constant* LowRawNull;
  Constant* LowWideNull;
  BitCastInst* Dummy;

  // Low-level functions used by codegen.
  FunctionCallee PollcheckSlow;
  FunctionCallee StoreBarrierSlow;
  FunctionCallee GetNextBytesForVAArg;
  FunctionCallee Allocate;
  FunctionCallee AllocateWithAlignment;
  FunctionCallee CheckReadInt;
  FunctionCallee CheckReadAlignedInt;
  FunctionCallee CheckReadInt8Fail;
  FunctionCallee CheckReadInt16Fail;
  FunctionCallee CheckReadInt32Fail;
  FunctionCallee CheckReadInt64Fail;
  FunctionCallee CheckReadInt128Fail;
  FunctionCallee CheckReadPtrFail;
  FunctionCallee CheckWriteInt;
  FunctionCallee CheckWriteAlignedInt;
  FunctionCallee CheckWriteInt8Fail;
  FunctionCallee CheckWriteInt16Fail;
  FunctionCallee CheckWriteInt32Fail;
  FunctionCallee CheckWriteInt64Fail;
  FunctionCallee CheckWriteInt128Fail;
  FunctionCallee CheckWritePtrFail;
  FunctionCallee CheckFunctionCallFail;
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
  FunctionCallee LandingPad;
  FunctionCallee ResumeUnwind;
  FunctionCallee JmpBufCreate;
  FunctionCallee PromoteArgsToHeap;
  FunctionCallee CCArgsCheckFailure;
  FunctionCallee CCRetsCheckFailure;
  FunctionCallee _Setjmp;

  Constant* EmptyCCType;
  Constant* VoidCCType;
  Constant* IntCCType;
  Constant* PtrCCType;
  Constant* IsMarking;

  std::unordered_map<std::string, GlobalVariable*> Strings;
  std::unordered_map<FunctionOriginKey, GlobalVariable*> FunctionOrigins;
  std::unordered_map<OriginKey, GlobalVariable*> Origins;
  std::unordered_map<EHDataKey, GlobalVariable*> EHDatas; /* the value is a high-level GV, need to lookup
                                                             the getter. */
  std::unordered_map<Constant*, int> EHTypeIDs;
  std::unordered_map<CallBase*, unsigned> Setjmps;
  std::unordered_map<Type*, Constant*> CCTypes;

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
  std::unordered_map<InvokeInst*, LandingPadInst*> LPIs;
  
  BasicBlock* FirstRealBlock;

  BitCastInst* FutureArgBuffer;
  BitCastInst* FutureReturnBuffer;

  BasicBlock* ReturnB;
  PHINode* ReturnPhi;
  BasicBlock* ResumeB;
  BasicBlock* ReallyReturnB;

  size_t ArgBufferSize;
  size_t ArgBufferAlignment;
  size_t ReturnBufferSize;
  size_t ReturnBufferAlignment;

  bool UsesVastart;
  size_t SnapshottedArgsFrameIndex;
  Value* SnapshottedArgsPtr;
  std::vector<Value*> Args;

  Value* MyThread;

  std::unordered_map<ValuePtr, size_t> FrameIndexMap;
  size_t FrameSize;
  Value* Frame;

  std::vector<Instruction*> ToErase;

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

  // What does "CanCatch" mean in this context? CanCatch=true means we're at an origin that is either:
  // - a CallInst in a function that is !doesNotThrow, or
  // - an InvokeInst.
  //
  // Lots of origins don't meet this definition!
  Constant* getFunctionOrigin(bool CanCatch) {
    assert(OldF);
    
    FunctionOriginKey FOK(OldF, CanCatch);
    auto iter = FunctionOrigins.find(FOK);
    if (iter != FunctionOrigins.end())
      return iter->second;

    Constant* Personality = LowRawNull;
    if (CanCatch && OldF->hasPersonalityFn()) {
      assert(GlobalToGetter.count(cast<Function>(OldF->getPersonalityFn())));
      Personality = GlobalToGetter[cast<Function>(OldF->getPersonalityFn())];
    }
    
    bool CanThrow = !OldF->doesNotThrow();
    unsigned NumSetjmps = Setjmps.size();
    
    Constant* C = ConstantStruct::get(
      FunctionOriginTy,
      { getString(getFunctionName(OldF)),
        OldF->getSubprogram() ? getString(OldF->getSubprogram()->getFilename()) : LowRawNull,
        ConstantInt::get(Int32Ty, FrameSize),
        Personality, ConstantInt::get(Int8Ty, CanThrow), ConstantInt::get(Int8Ty, CanCatch),
        ConstantInt::get(Int32Ty, NumSetjmps) });
    GlobalVariable* Result = new GlobalVariable(
      M, FunctionOriginTy, true, GlobalVariable::PrivateLinkage, C, "filc_function_origin");
    FunctionOrigins[FOK] = Result;
    return Result;
  }

  Constant* getEHData(LandingPadInst* LPI) {
    if (!LPI)
      return LowRawNull;
    assert(EHDatas.count(LPI));
    assert(GlobalToGetter.count(EHDatas[LPI]));
    return GlobalToGetter[EHDatas[LPI]];
  }

  // See the definition of CanCatch, above.
  Constant* getOrigin(DebugLoc Loc, bool CanCatch = false, LandingPadInst* LPI = nullptr) {
    assert(OldF);
    if (LPI)
      assert(CanCatch);
    
    DILocation* Impl = Loc.get();
    OriginKey OK(OldF, Impl, CanCatch, LPI);
    auto iter = Origins.find(OK);
    if (iter != Origins.end())
      return iter->second;

    unsigned Line = 0;
    unsigned Col = 0;
    if (Loc) {
      Line = Loc.getLine();
      Col = Loc.getCol();
    }

    GlobalVariable* Result;
    if (CanCatch && OldF->hasPersonalityFn()) {
      Constant* C = ConstantStruct::get(
        OriginWithEHTy,
        { getFunctionOrigin(CanCatch), ConstantInt::get(Int32Ty, Line),
          ConstantInt::get(Int32Ty, Col), getEHData(LPI) });
      Result = new GlobalVariable(
        M, OriginWithEHTy, true, GlobalVariable::PrivateLinkage, C, "filc_origin_with_eh");
    } else {
      Constant* C = ConstantStruct::get(
        OriginTy,
        { getFunctionOrigin(CanCatch), ConstantInt::get(Int32Ty, Line), ConstantInt::get(Int32Ty, Col) });
      Result = new GlobalVariable(
        M, OriginTy, true, GlobalVariable::PrivateLinkage, C, "filc_origin");
    }
    Origins[OK] = Result;
    return Result;
  }

  // See definition of CanCatch=true, above.
  Value* getCatchOrigin(DebugLoc Loc, LandingPadInst* LPI = nullptr) {
    return getOrigin(Loc, true, LPI);
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
      if (DLBefore.getTypeStoreSize(T) != DL.getTypeStoreSize(LowT)) {
        errs() << "Error lowering type: " << *T << "\n"
               << "Type after lowering: " << *LowT << "\n"
               << "Predicted lowered size: " << DLBefore.getTypeStoreSize(T) << "\n"
               << "Actual lowered size: " << DL.getTypeStoreSize(LowT) << "\n";
      }
      assert(DLBefore.getTypeStoreSize(T) == DL.getTypeStoreSize(LowT));
    }
    LoweredTypes[T] = LowT;
    return LowT;
  }
  
  void inlineCheckAccess(Value* P, size_t SizeAndAlignment, AccessKind AK, WordType ExpectedType,
                         FunctionCallee AccessFailure, Instruction* InsertBefore) {
    assert(SizeAndAlignment >= 1);
    assert(SizeAndAlignment <= WordSize);
    assert(!(SizeAndAlignment & (SizeAndAlignment - 1)));
    assert(ExpectedType == WordTypeInt || ExpectedType == WordTypePtr);
    assert(!(WordTypeInt & WordTypePtr));
    assert(!WordTypeUnset);
    DebugLoc DL = InsertBefore->getDebugLoc();
    Value* Object = ptrObject(P, InsertBefore);
    Value* Ptr = ptrPtr(P, InsertBefore);
    ICmpInst* NullObject = new ICmpInst(
      InsertBefore, ICmpInst::ICMP_EQ, Object, LowRawNull, "filc_null_access_object");
    NullObject->setDebugLoc(DL);
    Instruction* FailBlockTerm = SplitBlockAndInsertIfThen(NullObject, InsertBefore, true);
    BasicBlock* FailBlock = FailBlockTerm->getParent();
    CallInst::Create(AccessFailure, { P, getOrigin(DL) }, "", FailBlockTerm)->setDebugLoc(DL);
    Instruction* PtrInt = new PtrToIntInst(Ptr, IntPtrTy, "filc_ptr_as_int", InsertBefore);
    PtrInt->setDebugLoc(DL);
    if (SizeAndAlignment > 1) {
      Instruction* Masked = BinaryOperator::Create(
        Instruction::And, PtrInt, ConstantInt::get(IntPtrTy, SizeAndAlignment - 1),
        "filc_ptr_alignment_masked", InsertBefore);
      Masked->setDebugLoc(DL);
      ICmpInst* IsAligned = new ICmpInst(
        InsertBefore, ICmpInst::ICMP_EQ, Masked, ConstantInt::get(IntPtrTy, 0),
        "filc_ptr_is_aligned");
      IsAligned->setDebugLoc(DL);
      SplitBlockAndInsertIfElse(IsAligned, InsertBefore, false, nullptr, nullptr, nullptr, FailBlock);
    }
    if (AK == AccessKind::Write) {
      BinaryOperator* Masked = BinaryOperator::Create(
        Instruction::And, objectFlags(Object, InsertBefore),
        ConstantInt::get(Int16Ty, ObjectFlagReadonly), "filc_flags_masked", InsertBefore);
      Masked->setDebugLoc(DL);
      ICmpInst* IsNotReadOnly = new ICmpInst(
        InsertBefore, ICmpInst::ICMP_EQ, Masked, ConstantInt::get(Int16Ty, 0),
        "filc_object_is_not_read_only");
      IsNotReadOnly->setDebugLoc(DL);
      SplitBlockAndInsertIfElse(
        IsNotReadOnly, InsertBefore, false, nullptr, nullptr, nullptr, FailBlock);
    }
    Instruction* IsBelowUpper = new ICmpInst(
      InsertBefore, ICmpInst::ICMP_ULT, Ptr, objectUpper(Object, InsertBefore),
      "filc_ptr_below_upper");
    IsBelowUpper->setDebugLoc(DL);
    SplitBlockAndInsertIfElse(
      IsBelowUpper, InsertBefore, false, nullptr, nullptr, nullptr, FailBlock);
    Value* Lower = objectLower(Object, InsertBefore);
    Instruction* IsBelowLower = new ICmpInst(
      InsertBefore, ICmpInst::ICMP_ULT, Ptr, Lower, "filc_ptr_below_lower");
    IsBelowLower->setDebugLoc(DL);
    SplitBlockAndInsertIfThen(
      IsBelowLower, InsertBefore, false, nullptr, nullptr, nullptr, FailBlock);
    Instruction* LowerInt = new PtrToIntInst(Lower, IntPtrTy, "filc_lower_as_int", InsertBefore);
    LowerInt->setDebugLoc(DL);
    Instruction* Offset = BinaryOperator::Create(
      Instruction::Sub, PtrInt, LowerInt, "filc_ptr_offset", InsertBefore);
    Offset->setDebugLoc(DL);
    Instruction* WordTypeIndex = BinaryOperator::Create(
      Instruction::LShr, Offset, ConstantInt::get(IntPtrTy, WordSizeShift), "filc_word_type_index",
      InsertBefore);
    WordTypeIndex->setDebugLoc(DL);
    Instruction* WordTypePtr = GetElementPtrInst::Create(
      Int8Ty, objectWordTypesPtr(Object, InsertBefore), { WordTypeIndex }, "filc_word_type_ptr",
      InsertBefore);
    WordTypePtr->setDebugLoc(DL);
    Instruction* WordType = new LoadInst(Int8Ty, WordTypePtr, "filc_word_type", InsertBefore);
    WordType->setDebugLoc(DL);
    ICmpInst* GoodType = new ICmpInst(
      InsertBefore, ICmpInst::ICMP_EQ, WordType, ConstantInt::get(Int8Ty, ExpectedType),
      "filc_good_type");
    GoodType->setDebugLoc(DL);
    Instruction* MaybeBadTypeTerm = SplitBlockAndInsertIfElse(GoodType, InsertBefore, false);
    ICmpInst* UnsetType = new ICmpInst(
      MaybeBadTypeTerm, ICmpInst::ICMP_EQ, WordType, ConstantInt::get(Int8Ty, WordTypeUnset),
      "filc_unset_type");
    UnsetType->setDebugLoc(DL);
    SplitBlockAndInsertIfElse(
      UnsetType, MaybeBadTypeTerm, false, nullptr, nullptr, nullptr, FailBlock);
    Instruction* CAS = new AtomicCmpXchgInst(
      WordTypePtr, ConstantInt::get(Int8Ty, WordTypeUnset), ConstantInt::get(Int8Ty, ExpectedType),
      Align(1), AtomicOrdering::SequentiallyConsistent, AtomicOrdering::SequentiallyConsistent,
      SyncScope::System, MaybeBadTypeTerm);
    CAS->setDebugLoc(DL);
    Instruction* OldWordType = ExtractValueInst::Create(
      Int8Ty, CAS, { 0 }, "filc_old_word_type", MaybeBadTypeTerm);
    OldWordType->setDebugLoc(DL);
    Instruction* Masked = BinaryOperator::Create(
      Instruction::And, OldWordType, ConstantInt::get(Int8Ty, ~ExpectedType), "filc_masked_old_type",
      MaybeBadTypeTerm);
    Masked->setDebugLoc(DL);
    Instruction* GoodOldType = new ICmpInst(
      MaybeBadTypeTerm, ICmpInst::ICMP_EQ, Masked, ConstantInt::get(Int8Ty, 0),
      "filc_good_old_type");
    GoodOldType->setDebugLoc(DL);
    assert(cast<BranchInst>(MaybeBadTypeTerm)->getSuccessor(0) == InsertBefore->getParent());
    BranchInst::Create(InsertBefore->getParent(), FailBlock, GoodOldType, MaybeBadTypeTerm)
      ->setDebugLoc(DL);
    MaybeBadTypeTerm->eraseFromParent();
  }

  void checkReadInt(Value *P, size_t Size, size_t Alignment, Instruction *InsertBefore) {
    if (Size == 1) {
      inlineCheckAccess(P, 1, AccessKind::Read, WordTypeInt, CheckReadInt8Fail, InsertBefore);
      return;
    }
    if (Size == 2 && Alignment >= 2) {
      inlineCheckAccess(P, 2, AccessKind::Read, WordTypeInt, CheckReadInt16Fail, InsertBefore);
      return;
    }
    if (Size == 4 && Alignment >= 4) {
      inlineCheckAccess(P, 4, AccessKind::Read, WordTypeInt, CheckReadInt32Fail, InsertBefore);
      return;
    }
    if (Size == 8 && Alignment >= 8) {
      inlineCheckAccess(P, 8, AccessKind::Read, WordTypeInt, CheckReadInt64Fail, InsertBefore);
      return;
    }
    if (Size == 16 && Alignment >= 16) {
      inlineCheckAccess(P, 16, AccessKind::Read, WordTypeInt, CheckReadInt128Fail, InsertBefore);
      return;
    }
    if (Alignment == 1) {
      CallInst::Create(
        CheckReadInt,
        { P, ConstantInt::get(IntPtrTy, Size), getOrigin(InsertBefore->getDebugLoc()) },
        "", InsertBefore)
        ->setDebugLoc(InsertBefore->getDebugLoc());
      return;
    }
    CallInst::Create(
      CheckReadAlignedInt,
      { P, ConstantInt::get(IntPtrTy, Size), ConstantInt::get(IntPtrTy, Alignment),
        getOrigin(InsertBefore->getDebugLoc()) },
      "", InsertBefore)
      ->setDebugLoc(InsertBefore->getDebugLoc());
  }

  void checkWriteInt(Value *P, size_t Size, size_t Alignment, Instruction *InsertBefore) {
    if (Size == 1) {
      inlineCheckAccess(P, 1, AccessKind::Write, WordTypeInt, CheckWriteInt8Fail, InsertBefore);
      return;
    }
    if (Size == 2 && Alignment >= 2) {
      inlineCheckAccess(P, 2, AccessKind::Write, WordTypeInt, CheckWriteInt16Fail, InsertBefore);
      return;
    }
    if (Size == 4 && Alignment >= 4) {
      inlineCheckAccess(P, 4, AccessKind::Write, WordTypeInt, CheckWriteInt32Fail, InsertBefore);
      return;
    }
    if (Size == 8 && Alignment >= 8) {
      inlineCheckAccess(P, 8, AccessKind::Write, WordTypeInt, CheckWriteInt64Fail, InsertBefore);
      return;
    }
    if (Size == 16 && Alignment >= 16) {
      inlineCheckAccess(P, 16, AccessKind::Write, WordTypeInt, CheckWriteInt128Fail, InsertBefore);
      return;
    }
    if (Alignment == 1) {
      CallInst::Create(
        CheckWriteInt,
        { P, ConstantInt::get(IntPtrTy, Size), getOrigin(InsertBefore->getDebugLoc()) },
        "", InsertBefore)
        ->setDebugLoc(InsertBefore->getDebugLoc());
    }
    CallInst::Create(
      CheckWriteAlignedInt,
      { P, ConstantInt::get(IntPtrTy, Size), ConstantInt::get(IntPtrTy, Alignment),
        getOrigin(InsertBefore->getDebugLoc()) },
      "", InsertBefore)
      ->setDebugLoc(InsertBefore->getDebugLoc());
  }

  void checkInt(Value* P, size_t Size, size_t Alignment, AccessKind AK, Instruction *InsertBefore) {
    switch (AK) {
    case AccessKind::Read:
      checkReadInt(P, Size, Alignment, InsertBefore);
      return;
    case AccessKind::Write:
      checkWriteInt(P, Size, Alignment, InsertBefore);
      return;
    }
    llvm_unreachable("Bad access kind");
  }

  void checkReadPtr(Value *P, Instruction *InsertBefore) {
    inlineCheckAccess(P, WordSize, AccessKind::Read, WordTypePtr, CheckReadPtrFail, InsertBefore);
  }

  void checkWritePtr(Value *P, Instruction *InsertBefore) {
    inlineCheckAccess(P, WordSize, AccessKind::Write, WordTypePtr, CheckWritePtrFail, InsertBefore);
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

  Value* objectForSpecialPayload(Value* Payload, Instruction* InsertBefore) {
    Instruction* Result = GetElementPtrInst::Create(
      Int8Ty, Payload, { ConstantInt::get(IntPtrTy, -SpecialObjectSize) },
      "filc_object_for_special_payload", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  Value* ccPtrType(Value* P, Instruction* InsertBefore) {
    Instruction* Result = ExtractValueInst::Create(
      LowRawPtrTy, P, { 0 }, "filc_cc_ptr_type", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  Value* ccPtrBase(Value* P, Instruction* InsertBefore) {
    Instruction* Result = ExtractValueInst::Create(
      LowRawPtrTy, P, { 1 }, "filc_cc_ptr_base", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  Value* createCCPtr(Value* ccType, Value* ccBase, Instruction* InsertBefore) {
    Instruction* Result = InsertValueInst::Create(
      UndefValue::get(CCPtrTy), ccType, { 0 }, "filc_cc_insert_type", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    Result = InsertValueInst::Create(Result, ccBase, { 1 }, "filc_cc_insert_base", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
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
    Instruction* Result = new IntToPtrInst(IntResult, LowRawPtrTy, "filc_ptr_object", InsertBefore);
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

  Value* ptrForSpecialPayload(Value* Payload, Instruction* InsertBefore) {
    return createPtr(objectForSpecialPayload(Payload, InsertBefore), Payload, InsertBefore);
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
    Instruction* UpperPtr = GetElementPtrInst::Create(
      ObjectTy, Object, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) },
      "filc_object_upper_ptr", InsertBefore);
    UpperPtr->setDebugLoc(InsertBefore->getDebugLoc());
    return UpperPtr;
  }

  Value* objectFlagsPtr(Value* Object, Instruction* InsertBefore) {
    Instruction* FlagsPtr = GetElementPtrInst::Create(
      ObjectTy, Object, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 2) },
      "filc_object_flags_ptr", InsertBefore);
    FlagsPtr->setDebugLoc(InsertBefore->getDebugLoc());
    return FlagsPtr;
  }

  Value* objectWordTypesPtr(Value* Object, Instruction* InsertBefore) {
    Instruction* WordTypesPtr = GetElementPtrInst::Create(
      ObjectTy, Object, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 3) },
      "filc_object_flags_ptr", InsertBefore);
    WordTypesPtr->setDebugLoc(InsertBefore->getDebugLoc());
    return WordTypesPtr;
  }

  Value* objectLower(Value* Object, Instruction* InsertBefore) {
    Instruction* Lower = new LoadInst(
      LowRawPtrTy, objectLowerPtr(Object, InsertBefore), "filc_object_lower_load", InsertBefore);
    Lower->setDebugLoc(InsertBefore->getDebugLoc());
    return Lower;
  }

  Value* objectUpper(Value* Object, Instruction* InsertBefore) {
    Instruction* Upper = new LoadInst(
      LowRawPtrTy, objectUpperPtr(Object, InsertBefore), "filc_object_upper_load", InsertBefore);
    Upper->setDebugLoc(InsertBefore->getDebugLoc());
    return Upper;
  }

  Value* objectFlags(Value* Object, Instruction* InsertBefore) {
    Instruction* Flags = new LoadInst(
      Int16Ty, objectFlagsPtr(Object, InsertBefore), "filc_object_flags_load", InsertBefore);
    Flags->setDebugLoc(InsertBefore->getDebugLoc());
    return Flags;
  }

  Value* ptrWithObject(Value* Object, Instruction* InsertBefore) {
    return createPtr(Object, objectLower(Object, InsertBefore), InsertBefore);
  }

  Value* badPtr(Value* P, Instruction* InsertBefore) {
    return createPtr(LowRawNull, P, InsertBefore);
  }

  Value* loadPtr(
    Value* P, bool isVolatile, Align A, AtomicOrdering AO, MemoryKind MK, Instruction* InsertBefore) {
    // FIXME: I probably only need Unordered, not Monotonic.
    Instruction* Result = new LoadInst(
      Int128Ty, P, "filc_load_ptr", isVolatile,
      std::max(A, Align(WordSize)),
      MK == MemoryKind::Heap ? getMergedAtomicOrdering(AtomicOrdering::Monotonic, AO) : AO,
      SyncScope::System, InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return wordToPtr(Result, InsertBefore);
  }

  Value* loadPtr(Value* P, Instruction* InsertBefore) {
    return loadPtr(
      P, false, Align(WordSize), AtomicOrdering::NotAtomic, MemoryKind::Heap, InsertBefore);
  }

  void storeBarrierForObject(Value* Object, Instruction* InsertBefore) {
    assert(MyThread);
    DebugLoc DL = InsertBefore->getDebugLoc();
    ICmpInst* NullObject = new ICmpInst(
      InsertBefore, ICmpInst::ICMP_EQ, Object, LowRawNull, "filc_barrier_null_object");
    NullObject->setDebugLoc(DL);
    Instruction* NotNullTerm = SplitBlockAndInsertIfElse(NullObject, InsertBefore, false);
    LoadInst* IsMarkingByte = new LoadInst(Int8Ty, IsMarking, "filc_is_marking_byte", NotNullTerm);
    IsMarkingByte->setDebugLoc(DL);
    ICmpInst* IsNotMarking = new ICmpInst(
      NotNullTerm, ICmpInst::ICMP_EQ, IsMarkingByte, ConstantInt::get(Int8Ty, 0),
      "filc_is_not_marking");
    IsNotMarking->setDebugLoc(DL);
    Instruction* IsMarkingTerm = SplitBlockAndInsertIfElse(IsNotMarking, NotNullTerm, false);
    CallInst::Create(StoreBarrierSlow, { MyThread, Object }, "", IsMarkingTerm)->setDebugLoc(DL);
  }
  
  void storeBarrierForValue(Value* V, Instruction* InsertBefore) {
    storeBarrierForObject(ptrObject(V, InsertBefore), InsertBefore);
  }
  
  void storePtr(
    Value* V, Value* P, bool isVolatile, Align A, AtomicOrdering AO, MemoryKind MK,
    Instruction* InsertBefore) {
    if (MK == MemoryKind::Heap)
      storeBarrierForValue(V, InsertBefore);
    (new StoreInst(
      ptrWord(V, InsertBefore), P, isVolatile, std::max(A, Align(WordSize)),
      MK == MemoryKind::Heap ? getMergedAtomicOrdering(AtomicOrdering::Monotonic, AO) : AO,
      SyncScope::System, InsertBefore))
      ->setDebugLoc(InsertBefore->getDebugLoc());
  }

  void storePtr(Value* V, Value* P, Instruction* InsertBefore) {
    storePtr(V, P, false, Align(WordSize), AtomicOrdering::NotAtomic, MemoryKind::Heap, InsertBefore);
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

  void checkRecurse(Type *LowT, Value* HighP, Value *P, size_t Alignment, AccessKind AK,
                    Instruction *InsertBefore) {
    if (!hasPtrsForCheck(LowT)) {
      checkInt(ptrWithPtr(HighP, P, InsertBefore), DL.getTypeStoreSize(LowT),
               MinAlign(DL.getABITypeAlign(LowT).value(), Alignment), AK, InsertBefore);
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
        checkRecurse(InnerT, HighP, InnerP, Alignment, AK, InsertBefore);
      }
      return;
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(LowT)) {
      for (uint64_t Index = AT->getNumElements(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          AT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "filc_InnerP_array", InsertBefore);
        checkRecurse(AT->getElementType(), HighP, InnerP, Alignment, AK, InsertBefore);
      }
      return;
    }
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(LowT)) {
      for (unsigned Index = VT->getElementCount().getFixedValue(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          VT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "filc_InnerP_vector", InsertBefore);
        checkRecurse(VT->getElementType(), HighP, InnerP, Alignment, AK, InsertBefore);
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
  Value* prepareForAccess(Type *LowT, Value *HighP, size_t Alignment, AccessKind AK,
                          Instruction *InsertBefore) {
    Value* LowP = ptrPtr(HighP, InsertBefore);
    checkRecurse(LowT, HighP, LowP, Alignment, AK, InsertBefore);
    return LowP;
  }

  Value* loadValueRecurseAfterCheck(
    Type* LowT, Value* P,
    bool isVolatile, Align A, AtomicOrdering AO, SyncScope::ID SS, MemoryKind MK,
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
      return loadPtr(P, isVolatile, A, AO, MK, InsertBefore);

    assert(!isa<PointerType>(LowT));

    if (StructType* ST = dyn_cast<StructType>(LowT)) {
      Value* Result = UndefValue::get(ST);
      for (unsigned Index = ST->getNumElements(); Index--;) {
        Type* InnerT = ST->getElementType(Index);
        Value *InnerP = GetElementPtrInst::Create(
          ST, P, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, Index) },
          "filc_InnerP_struct", InsertBefore);
        Value* V = loadValueRecurseAfterCheck(
          InnerT, InnerP, isVolatile, A, AO, SS, MK, InsertBefore);
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
        Value* V = loadValueRecurseAfterCheck(
          AT->getElementType(), InnerP, isVolatile, A, AO, SS, MK, InsertBefore);
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
        Value* V = loadValueRecurseAfterCheck(
          VT->getElementType(), InnerP, isVolatile, A, AO, SS, MK, InsertBefore);
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
    checkRecurse(LowT, HighP, P, A.value(), AccessKind::Read, InsertBefore);
    return loadValueRecurseAfterCheck(LowT, P, isVolatile, A, AO, SS, MemoryKind::Heap, InsertBefore);
  }

  void storeValueRecurseAfterCheck(
    Type* LowT, Value* V, Value* P,
    bool isVolatile, Align A, AtomicOrdering AO, SyncScope::ID SS, MemoryKind MK,
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
      storePtr(V, P, isVolatile, A, AO, MK, InsertBefore);
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
        storeValueRecurseAfterCheck(
            InnerT, InnerV, InnerP, isVolatile, A, AO, SS, MK, InsertBefore);
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
        storeValueRecurseAfterCheck(
            AT->getElementType(), InnerV, InnerP, isVolatile, A, AO, SS, MK,
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
        storeValueRecurseAfterCheck(
            VT->getElementType(), InnerV, InnerP, isVolatile, A, AO, SS, MK,
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
                         bool isVolatile, Align A, AtomicOrdering AO, SyncScope::ID SS,
                         Instruction* InsertBefore) {
    checkRecurse(LowT, HighP, P, A.value(), AccessKind::Write, InsertBefore);
    storeValueRecurseAfterCheck(LowT, V, P, isVolatile, A, AO, SS, MemoryKind::Heap, InsertBefore);
  }

  Value* allocateObject(Value* Size, size_t Alignment, Instruction* InsertBefore) {
    Instruction* Result;
    if (Alignment <= GCMinAlign) {
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
    Setjmps.clear();
    UsesVastart = false;

    assert(!Blocks.empty());

    auto LiveCast = [&] (Value* V) -> Value* {
      if (isa<Instruction>(V))
        return V;
      if (!isa<Argument>(V) && !isa<Constant>(V) && !isa<MetadataAsValue>(V) && !isa<InlineAsm>(V)
          && !isa<BasicBlock>(V)) {
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

    for (BasicBlock* BB : Blocks) {
      for (Instruction& I : *BB) {
        if (IntrinsicInst* II = dyn_cast<IntrinsicInst>(&I)) {
          if (II->getIntrinsicID() == Intrinsic::vastart) {
            UsesVastart = true;
            SnapshottedArgsFrameIndex = FrameSize++;
          }
        }
        if (UsesVastart)
          break;
      }
      if (UsesVastart)
        break;
    }

    for (BasicBlock* BB : Blocks) {
      for (Instruction& I : *BB) {
        if (CallBase* CI = dyn_cast<CallBase>(&I)) {
          if (Function* F = dyn_cast<Function>(CI->getCalledOperand())) {
            if (isSetjmp(F)) {
              assert(CI->hasFnAttr(Attribute::ReturnsTwice));
              assert(F->hasFnAttribute(Attribute::ReturnsTwice));
              Setjmps[CI] = FrameSize++;
            }
          }
        }
      }
    }
  }

  void recordObjectAtIndex(Value* Object, size_t FrameIndex, Instruction* InsertBefore) {
    assert(FrameIndex < FrameSize);
    Instruction* ObjectsPtr = GetElementPtrInst::Create(
      FrameTy, Frame, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 2) },
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

    Size += DL.getTypeAllocSize(LowT);
    while ((Size + WordSize - 1) / WordSize > WordTypes.size())
      WordTypes.push_back(WordTypeInt);
  }
  
  StructType* argsType(FunctionType* FT) {
    return StructType::get(C, FT->params());
  }

  std::vector<WordType> wordTypesForLowType(Type* LowT) {
    std::vector<WordType> WordTypes;
    size_t Size = 0;
    buildWordTypesRecurse(LowT, LowT, Size, WordTypes);
    return WordTypes;
  }
  
  Constant* ccTypeForLowType(Type* LowT) {
    auto iter = CCTypes.find(LowT);
    if (iter != CCTypes.end())
      return iter->second;

    std::vector<WordType> WordTypes = wordTypesForLowType(LowT);
    Constant* GV;
    if (WordTypes.empty())
      GV = EmptyCCType;
    else if (WordTypes.size() == 1) {
      if (WordTypes[0] == WordTypeInt)
        GV = IntCCType;
      else {
        assert(WordTypes[0] == WordTypePtr);
        GV = PtrCCType;
      }
    } else {
      StructType* CCType = StructType::get(C, { IntPtrTy, ArrayType::get(Int8Ty, WordTypes.size()) });
      Constant* CS = ConstantStruct::get(
        CCType, { ConstantInt::get(IntPtrTy, WordTypes.size()), ConstantDataArray::get(C, WordTypes) });
      GV = new GlobalVariable(
        M, CCType, true, GlobalValue::PrivateLinkage, CS, "filc_cc_type_const");
    }
    CCTypes[LowT] = GV;
    return GV;
  }

  void checkCCType(Type* LowT, Value* CCPtr, FunctionCallee CCCheckFailure, Instruction* InsertBefore) {
    BasicBlock* FailB = BasicBlock::Create(C, "filc_cc_fail_block", NewF);
    CallInst::Create(
      CCCheckFailure, { CCPtr, ccTypeForLowType(LowT), getOrigin(DebugLoc()) }, "",
      FailB);
    new UnreachableInst(C, FailB);

    Value* CCType = ccPtrType(CCPtr, InsertBefore);
    std::vector<WordType> WordTypes = wordTypesForLowType(LowT);
    GetElementPtrInst* NumWordsPtr = GetElementPtrInst::Create(
      CCTypeTy, CCType, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 0) },
      "filc_cc_type_num_words_ptr", InsertBefore);
    LoadInst* NumWords = new LoadInst(IntPtrTy, NumWordsPtr, "filc_cc_type_num_words", InsertBefore);
    ICmpInst* InBounds = new ICmpInst(
      InsertBefore, ICmpInst::ICMP_ULE, ConstantInt::get(IntPtrTy, WordTypes.size()), NumWords,
      "filc_cc_type_big_enough");
    SplitBlockAndInsertIfElse(InBounds, InsertBefore, false, nullptr, nullptr, nullptr, FailB);
    GetElementPtrInst* WordTypesPtr = GetElementPtrInst::Create(
      CCTypeTy, CCType, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) },
      "filc_cc_type_word_types_ptr", InsertBefore);
    for (size_t Index = WordTypes.size(); Index--;) {
      GetElementPtrInst* WordTypeGEP = GetElementPtrInst::Create(
        Int8Ty, WordTypesPtr, { ConstantInt::get(IntPtrTy, Index) }, "filc_cc_word_type_ptr",
        InsertBefore);
      LoadInst* WordType = new LoadInst(Int8Ty, WordTypeGEP, "filc_cc_word_type", InsertBefore);
      assert(WordTypes[Index] == WordTypeInt || WordTypes[Index] == WordTypePtr);
      BinaryOperator* Masked = BinaryOperator::Create(
        Instruction::And, WordType,
        ConstantInt::get(Int8Ty, WordTypes[Index] ^ (WordTypeInt | WordTypePtr)),
        "filc_cc_masked_word_type", InsertBefore);
      ICmpInst* GoodType = new ICmpInst(
        InsertBefore, ICmpInst::ICMP_EQ, Masked, ConstantInt::get(Int8Ty, 0),
        "filc_cc_good_type");
      SplitBlockAndInsertIfElse(GoodType, InsertBefore, false, nullptr, nullptr, nullptr, FailB);
    }
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
      if (Size != DL.getTypeAllocSize(LowT)) {
        errs() << "Size mismatch for global type: " << *OriginalG->getValueType() << "\n"
               << "Type after lowering: " << *LowT << "\n"
               << "Size according to word type builder: " << Size << "\n"
               << "Size according to DataLayout: " << DL.getTypeAllocSize(LowT) << "\n";
      }
      assert(Size == DL.getTypeAllocSize(LowT));
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
      return ConstantAggregateZero::get(lowerType(C->getType()));
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
        GEP->setSourceElementType(lowerType(GEP->getSourceElementType()));
        GEP->setResultElementType(lowerType(GEP->getResultElementType()));
        bool result = GEP->accumulateConstantOffset(DL, OffsetAP);
        GEP->deleteValue();
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
      size_t ElementSize = DL.getTypeAllocSize(cast<ArrayType>(lowerType(CA->getType()))->getElementType());
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
      size_t ElementSize = DL.getTypeAllocSize(
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
    
    if (GlobalValue* G = dyn_cast<GlobalValue>(C)) {
      assert(!shouldPassThrough(G)); // This is a necessary safety check, at least for setjmp, probably for other things too.
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
    DummyUser->deleteValue();
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

    if (CallBase* CI = dyn_cast<CallBase>(I)) {
      std::vector<Type*> Types;
      for (size_t Index = 0; Index < CI->arg_size(); ++Index)
        Types.push_back(lowerType(CI->getArgOperand(Index)->getType()));
      InstLowTypeVectors[I] = std::move(Types);
      if (InvokeInst* II = dyn_cast<InvokeInst>(I))
        LPIs[II] = II->getLandingPadInst();
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
        assert(UsesVastart);
        lowerConstantOperand(II->getArgOperandUse(0), I, LowRawNull);
        checkWritePtr(II->getArgOperand(0), II);
        storePtr(SnapshottedArgsPtr, ptrPtr(II->getArgOperand(0), II), II);
        II->eraseFromParent();
        return true;
        
      case Intrinsic::vacopy: {
        lowerConstantOperand(II->getArgOperandUse(0), I, LowRawNull);
        lowerConstantOperand(II->getArgOperandUse(1), I, LowRawNull);
        checkWritePtr(II->getArgOperand(0), II);
        checkReadPtr(II->getArgOperand(1), II);
        Value* Load = loadPtr(ptrPtr(II->getArgOperand(1), II), II);
        storePtr(Load, ptrPtr(II->getArgOperand(0), II), II);
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

      case Intrinsic::eh_typeid_for: {
        Constant* TypeConstant = cast<Constant>(II->getArgOperand(0));
        assert(EHTypeIDs.count(TypeConstant));
        II->replaceAllUsesWith(ConstantInt::get(Int32Ty, EHTypeIDs[TypeConstant]));
        II->eraseFromParent();
        return true;
      }

      case Intrinsic::eh_sjlj_functioncontext:
      case Intrinsic::eh_sjlj_setjmp:
      case Intrinsic::eh_sjlj_longjmp:
      case Intrinsic::eh_sjlj_setup_dispatch:
        llvm_unreachable("Cannot use sjlj intrinsics.");
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
    
    if (CallBase* CI = dyn_cast<CallBase>(I)) {
      if (verbose) {
        errs() << "It's a call!\n";
        errs() << "Callee = " << CI->getCalledOperand() << "\n";
        if (CI->getCalledOperand())
          errs() << "Callee name = " << CI->getCalledOperand()->getName() << "\n";
      }

      if (Function* F = dyn_cast<Function>(CI->getCalledOperand())) {
        if (isSetjmp(F)) {
          if (verbose)
            errs() << "Lowering some kind of setjmp\n";
          for (Use& Arg : CI->args())
            lowerConstantOperand(Arg, CI, LowRawNull);
          assert(CI->getFunctionType() == F->getFunctionType());
          assert(CI->hasFnAttr(Attribute::ReturnsTwice));
          assert(Setjmps.count(CI));
          Value* ValueArg;
          if (F->getName() == "sigsetjmp") {
            assert(CI->arg_size() == 2);
            ValueArg = CI->getArgOperand(1);
          } else {
            assert(CI->arg_size() == 1);
            ValueArg = ConstantInt::get(Int32Ty, 0);
          }
          unsigned FrameIndex = Setjmps[CI];
          CallInst* Create = CallInst::Create(
            JmpBufCreate,
            { MyThread, ConstantInt::get(Int32Ty, static_cast<int>(getJmpBufKindForSetjmp(F))),
              ValueArg },
            "filc_jmp_buf_create", CI);
          Create->setDebugLoc(CI->getDebugLoc());
          Value* UserJmpBuf = CI->getArgOperand(0);
          checkWritePtr(UserJmpBuf, CI);
          storePtr(ptrForSpecialPayload(Create, CI), ptrPtr(UserJmpBuf, CI), CI);
          recordObjectAtIndex(objectForSpecialPayload(Create, CI), FrameIndex, CI);
          CallInst* NewCI = CallInst::Create(_Setjmp, { Create }, "filc_setjmp", CI);
          CI->replaceAllUsesWith(NewCI);
          NewCI->setDebugLoc(CI->getDebugLoc());
          if (InvokeInst* II = dyn_cast<InvokeInst>(CI))
            BranchInst::Create(II->getNormalDest(), CI)->setDebugLoc(CI->getDebugLoc());
          CI->eraseFromParent();
          return true;
        }

        if ((F->getName() == "zgetlower" || F->getName() == "zgetupper") &&
            F->getFunctionType()->getNumParams() == 1 &&
            F->getFunctionType()->getParamType(0) == LowRawPtrTy &&
            F->getReturnType() == LowRawPtrTy) {
          lowerConstantOperand(CI->getArgOperandUse(0), CI, LowRawNull);
          Value* Object = ptrObject(CI->getArgOperand(0), CI);
          ICmpInst* NullObject = new ICmpInst(
            CI, ICmpInst::ICMP_EQ, Object, LowRawNull, "filc_is_null_object");
          NullObject->setDebugLoc(CI->getDebugLoc());
          Instruction* NotNullTerm = SplitBlockAndInsertIfElse(NullObject, CI, false);
          Value* LowerUpper;
          if (F->getName() == "zgetlower")
            LowerUpper = objectLower(Object, NotNullTerm);
          else {
            assert(F->getName() == "zgetupper");
            LowerUpper = objectUpper(Object, NotNullTerm);
          }
          PHINode* Phi = PHINode::Create(LowRawPtrTy, 2, "filc_lower_upper_phi", CI);
          Phi->addIncoming(LowRawNull, NullObject->getParent());
          Phi->addIncoming(LowerUpper, NotNullTerm->getParent());
          CI->replaceAllUsesWith(ptrWithPtr(CI->getArgOperand(0), Phi, CI));
          CI->eraseFromParent();
          return true;
        }

        if (F->getName() == "zthread_self_id" &&
            F->getFunctionType()->getNumParams() == 0 &&
            F->getReturnType() == Int32Ty) {
          Instruction* TidPtr = GetElementPtrInst::Create(
            ThreadTy, MyThread, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) },
            "filc_tid_ptr", CI);
          TidPtr->setDebugLoc(CI->getDebugLoc());
          Instruction* Tid = new LoadInst(Int32Ty, TidPtr, "filc_tid", CI);
          Tid->setDebugLoc(CI->getDebugLoc());
          CI->replaceAllUsesWith(Tid);
          CI->eraseFromParent();
          return true;
        }

        if (F->getName() == "zthread_self" &&
            F->getFunctionType()->getNumParams() == 0 &&
            F->getReturnType() == LowRawPtrTy) {
          CI->replaceAllUsesWith(ptrForSpecialPayload(MyThread, CI));
          CI->eraseFromParent();
          return true;
        }

        if (F->getName() == "zthread_self_cookie" &&
            F->getFunctionType()->getNumParams() == 0 &&
            F->getReturnType() == LowRawPtrTy) {
          Instruction* CookiePtr = GetElementPtrInst::Create(
            ThreadTy, MyThread, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 4) },
            "filc_cookie_ptr", CI);
          CookiePtr->setDebugLoc(CI->getDebugLoc());
          Instruction* Cookie = new LoadInst(LowWidePtrTy, CookiePtr, "filc_cookie", CI);
          Cookie->setDebugLoc(CI->getDebugLoc());
          CI->replaceAllUsesWith(Cookie);
          CI->eraseFromParent();
          return true;
        }

        if (shouldPassThrough(F)) {
          for (Use& Arg : CI->args())
            lowerConstantOperand(Arg, CI, LowRawNull);
          return true;
        }
      }
    }
    
    if (isa<LandingPadInst>(I)) {
      CallInst* CI = CallInst::Create(LandingPad, { MyThread }, "filc_landing_pad", I);
      CI->setDebugLoc(I->getDebugLoc());
      SplitBlockAndInsertIfElse(CI, I, false, nullptr, nullptr, nullptr, ResumeB);
      StructType* ST = cast<StructType>(I->getType());
      assert(!ST->isOpaque());
      assert(ST->getNumElements() <= NumUnwindRegisters);
      Value* Result = UndefValue::get(lowerType(ST));
      for (unsigned Idx = ST->getNumElements(); Idx--;) {
        Instruction* UnwindRegisterPtr = GetElementPtrInst::Create(
          ThreadTy, MyThread,
          { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 3),
            ConstantInt::get(Int32Ty, Idx) },
          "filc_unwind_register_ptr", I);
        UnwindRegisterPtr->setDebugLoc(I->getDebugLoc());
        LoadInst* RawUnwindRegister = new LoadInst(
          LowWidePtrTy, UnwindRegisterPtr, "filc_load_unwind_register", I);
        (new StoreInst(LowWideNull, UnwindRegisterPtr, I))->setDebugLoc(I->getDebugLoc());
        Value* UnwindRegister;
        if (ST->getElementType(Idx) == LowRawPtrTy)
          UnwindRegister = RawUnwindRegister;
        else {
          assert(isa<IntegerType>(ST->getElementType(Idx)));
          Instruction* Trunc = new TruncInst(
            ptrWord(RawUnwindRegister, I), ST->getElementType(Idx), "filc_trunc_unwind_register", I);
          Trunc->setDebugLoc(I->getDebugLoc());
          UnwindRegister = Trunc;
        }
        Instruction* InsertValue = InsertValueInst::Create(
          Result, UnwindRegister, { Idx }, "filc_insert_unwind_register", I);
        InsertValue->setDebugLoc(I->getDebugLoc());
        Result = InsertValue;
      }
      I->replaceAllUsesWith(Result);
      I->removeFromParent();
      ToErase.push_back(I);
      return true;
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
        Instruction::Mul, Length, ConstantInt::get(IntPtrTy, DL.getTypeAllocSize(LowT)),
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
        SI->isVolatile(), SI->getAlign(), SI->getOrdering(), SI->getSyncScopeID(), SI);
      SI->eraseFromParent();
      return;
    }

    if (FenceInst* FI = dyn_cast<FenceInst>(I)) {
      // We don't need to do anything because it doesn't take operands.
      return;
    }

    if (AtomicCmpXchgInst* AI = dyn_cast<AtomicCmpXchgInst>(I)) {
      Value* LowP = prepareForAccess(
        InstLowTypes[AI], AI->getPointerOperand(), AI->getAlign().value(), AccessKind::Write, AI);
      if (InstLowTypes[AI] == LowWidePtrTy) {
        storeBarrierForValue(AI->getNewValOperand(), AI);
        Value* ExpectedWord = ptrWord(AI->getCompareOperand(), AI);
        Value* NewValueWord = ptrWord(AI->getNewValOperand(), AI);
        Instruction* NewAI = new AtomicCmpXchgInst(
          LowP, ExpectedWord, NewValueWord, AI->getAlign(), AI->getSuccessOrdering(),
          AI->getFailureOrdering(), AI->getSyncScopeID(), AI);
        NewAI->setDebugLoc(AI->getDebugLoc());
        Instruction* ResultWord = ExtractValueInst::Create(
          Int128Ty, NewAI, { 0 }, "filc_cas_result_word", AI);
        ResultWord->setDebugLoc(AI->getDebugLoc());
        Instruction* ResultBit = ExtractValueInst::Create(
          Int1Ty, NewAI, { 1 }, "filc_cas_result_bit", AI);
        ResultBit->setDebugLoc(AI->getDebugLoc());
        StructType* ResultTy = StructType::get(C, { LowWidePtrTy, Int1Ty });
        Instruction* Result = InsertValueInst::Create(
          UndefValue::get(ResultTy), wordToPtr(ResultWord, AI), { 0 }, "filc_cas_create_result_word", AI);
        Result->setDebugLoc(AI->getDebugLoc());
        Result = InsertValueInst::Create(Result, ResultBit, { 1 }, "filc_cas_create_result_bit", AI);
        Result->setDebugLoc(AI->getDebugLoc());
        AI->replaceAllUsesWith(Result);
        AI->eraseFromParent();
        return;
      }
      assert(!hasPtrsForCheck(InstLowTypes[AI]));
      AI->getOperandUse(AtomicCmpXchgInst::getPointerOperandIndex()) = LowP;
      return;
    }

    if (AtomicRMWInst* AI = dyn_cast<AtomicRMWInst>(I)) {
      Value* LowP = prepareForAccess(
        AI->getValOperand()->getType(), AI->getPointerOperand(), AI->getAlign().value(),
        AccessKind::Write, AI);
      if (InstLowTypes[AI] == LowWidePtrTy) {
        storeBarrierForValue(AI->getValOperand(), AI);
        Instruction* NewAI = new AtomicRMWInst(
          AI->getOperation(), LowP, ptrWord(AI->getValOperand(), AI), AI->getAlign(), AI->getOrdering(),
          AI->getSyncScopeID(), AI);
        NewAI->setDebugLoc(AI->getDebugLoc());
        AI->replaceAllUsesWith(wordToPtr(NewAI, AI));
        AI->eraseFromParent();
        return;
      }
      AI->getOperandUse(AtomicRMWInst::getPointerOperandIndex()) = LowP;
      assert(!hasPtrsForCheck(InstLowTypes[AI]));
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

    if (isa<CallBrInst>(I)) {
      llvm_unreachable("Don't support CallBr yet (and maybe never will)");
      return;
    }

    if (CallBase* CI = dyn_cast<CallBase>(I)) {
      assert(isa<CallInst>(CI) || isa<InvokeInst>(CI));
      
      if (CI->isInlineAsm()) {
        assert(isa<CallInst>(CI));
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

      FunctionType *FT = CI->getFunctionType();
      Value* Args;
      if (CI->arg_size()) {
        assert(InstLowTypeVectors.count(CI));
        std::vector<Type*> ArgTypes = InstLowTypeVectors[CI];
        StructType* ArgsType = StructType::get(C, ArgTypes);
        Constant* CCType = ccTypeForLowType(ArgsType);
        size_t ArgsSize = (DL.getTypeAllocSize(ArgsType) + WordSize - 1) / WordSize * WordSize;
        Instruction* ClearArgBuffer = CallInst::Create(
          RealMemset,
          { FutureArgBuffer, ConstantInt::get(Int8Ty, 0), ConstantInt::get(IntPtrTy, ArgsSize),
            ConstantInt::get(Int1Ty, false) }, "", CI);
        ClearArgBuffer->setDebugLoc(CI->getDebugLoc());
        ArgBufferSize = std::max(ArgBufferSize, ArgsSize);
        ArgBufferAlignment = std::max(
          ArgBufferAlignment, static_cast<size_t>(DL.getABITypeAlign(ArgsType).value()));
        assert(CI->arg_size() == ArgsType->getNumElements());
        const StructLayout* SL = DL.getStructLayout(ArgsType);
        for (size_t Index = CI->arg_size(); Index--;) {
          Instruction* ArgSlotPtr = GetElementPtrInst::Create(
            Int8Ty, FutureArgBuffer, { ConstantInt::get(IntPtrTy, SL->getElementOffset(Index)) },
            "filc_arg_slot", CI);
          ArgSlotPtr->setDebugLoc(CI->getDebugLoc());
          Type* LowT = ArgsType->getElementType(Index);
          storeValueRecurseAfterCheck(
            LowT, CI->getArgOperand(Index), ArgSlotPtr, false, DL.getABITypeAlign(LowT),
            AtomicOrdering::NotAtomic, SyncScope::System, MemoryKind::CC, CI);
        }
        Args = createCCPtr(CCType, FutureArgBuffer, CI);
      } else
        Args = createCCPtr(EmptyCCType, LowRawNull, CI);

      Type* LowRetT = lowerType(FT->getReturnType());
      Value* CCRetType;
      size_t RetSize;
      size_t RetAlignment;
      if (LowRetT == VoidTy) {
        CCRetType = VoidCCType;
        RetSize = WordSize;
        RetAlignment = 1;
      } else {
        CCRetType = ccTypeForLowType(LowRetT);
        RetSize = (DL.getTypeAllocSize(LowRetT) + WordSize - 1) / WordSize * WordSize;
        RetAlignment = DL.getABITypeAlign(LowRetT).value();
      }
      ReturnBufferSize = std::max(ReturnBufferSize, RetSize);
      ReturnBufferAlignment = std::max(ReturnBufferAlignment, RetAlignment);
      Value* Rets = createCCPtr(CCRetType, FutureReturnBuffer, CI);
      Instruction* ClearRetBuffer = CallInst::Create(
        RealMemset,
        { FutureReturnBuffer, ConstantInt::get(Int8Ty, 0), ConstantInt::get(IntPtrTy, RetSize),
          ConstantInt::get(Int1Ty, false) }, "", CI);
      ClearRetBuffer->setDebugLoc(CI->getDebugLoc());

      bool CanCatch;
      LandingPadInst* LPI;
      if (isa<CallInst>(CI)) {
        CanCatch = !OldF->doesNotThrow();
        LPI = nullptr;
      } else {
        CanCatch = true;
        assert(LPIs.count(cast<InvokeInst>(CI)));
        LPI = LPIs[cast<InvokeInst>(CI)];
      }
      
      Instruction* OriginPtr = GetElementPtrInst::Create(
        FrameTy, Frame, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) },
        "filc_frame_origin_ptr", CI);
      OriginPtr->setDebugLoc(CI->getDebugLoc());
      (new StoreInst(getOrigin(CI->getDebugLoc(), CanCatch, LPI), OriginPtr, CI))
        ->setDebugLoc(CI->getDebugLoc());

      Value* CalledObject = ptrObject(CI->getCalledOperand(), CI);
      ICmpInst* NullObject = new ICmpInst(
        CI, ICmpInst::ICMP_EQ, CalledObject, LowRawNull, "filc_null_called_object");
      NullObject->setDebugLoc(CI->getDebugLoc());
      Instruction* NewBlockTerm = SplitBlockAndInsertIfThen(NullObject, CI, true);
      BasicBlock* NewBlock = NewBlockTerm->getParent();
      CallInst::Create(
        CheckFunctionCallFail, { CI->getCalledOperand() }, "", NewBlockTerm)
        ->setDebugLoc(CI->getDebugLoc());
      ICmpInst* AtLower = new ICmpInst(
        CI, ICmpInst::ICMP_EQ, ptrPtr(CI->getCalledOperand(), CI), objectLower(CalledObject, CI),
        "filc_call_at_lower");
      AtLower->setDebugLoc(CI->getDebugLoc());
      SplitBlockAndInsertIfElse(AtLower, CI, false, nullptr, nullptr, nullptr, NewBlock);
      BinaryOperator* Masked = BinaryOperator::Create(
        Instruction::And, objectFlags(CalledObject, CI), ConstantInt::get(Int16Ty, ObjectFlagSpecial),
        "filc_call_mask_special", CI);
      Masked->setDebugLoc(CI->getDebugLoc());
      ICmpInst* NotSpecial = new ICmpInst(
        CI, ICmpInst::ICMP_EQ, Masked, ConstantInt::get(Int16Ty, 0), "filc_call_not_special");
      NotSpecial->setDebugLoc(CI->getDebugLoc());
      SplitBlockAndInsertIfThen(NotSpecial, CI, false, nullptr, nullptr, nullptr, NewBlock);
      LoadInst* WordType = new LoadInst(
        Int8Ty, objectWordTypesPtr(CalledObject, CI), "filc_call_word_type", CI);
      WordType->setDebugLoc(CI->getDebugLoc());
      ICmpInst* IsFunction = new ICmpInst(
        CI, ICmpInst::ICMP_EQ, WordType, ConstantInt::get(Int8Ty, WordTypeFunction),
        "filc_call_is_fuction");
      IsFunction->setDebugLoc(CI->getDebugLoc());
      SplitBlockAndInsertIfElse(IsFunction, CI, false, nullptr, nullptr, nullptr, NewBlock);

      assert(!CI->hasOperandBundles());
      CallInst* TheCall = CallInst::Create(
        PizlonatedFuncTy, ptrPtr(CI->getCalledOperand(), CI), { MyThread, Args, Rets }, "filc_call",
        CI);

      if (isa<CallInst>(CI) && CanCatch)
        SplitBlockAndInsertIfThen(TheCall, CI, false, nullptr, nullptr, nullptr, ResumeB);
      else if (InvokeInst* II = dyn_cast<InvokeInst>(CI))
        BranchInst::Create(II->getUnwindDest(), II->getNormalDest(), TheCall, II);
      
      Instruction* PostInsertionPt;
      if (isa<CallInst>(CI))
        PostInsertionPt = CI;
      else
        PostInsertionPt = &*cast<InvokeInst>(CI)->getNormalDest()->getFirstInsertionPt();

      if (LowRetT != VoidTy) {
        CI->replaceAllUsesWith(
          loadValueRecurseAfterCheck(
            LowRetT, FutureReturnBuffer, false, DL.getABITypeAlign(LowRetT),
            AtomicOrdering::NotAtomic, SyncScope::System, MemoryKind::CC, PostInsertionPt));
      }

      CI->eraseFromParent();
      return;
    }

    if (VAArgInst* VI = dyn_cast<VAArgInst>(I)) {
      Type* T = VI->getType();
      Type* LowT = lowerType(T);
      size_t Size = DL.getTypeAllocSize(LowT);
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
      llvm_unreachable("Shouldn't see LandingPad because it should have been handled by "
                       "earlyLowerInstruction.");
      return;
    }

    if (isa<ResumeInst>(I)) {
      // NOTE: This function call is only necessary for checks. If our unwind machinery is working
      // correctly, then these checks should never fire. But I'm paranoid.
      CallInst::Create(ResumeUnwind, { MyThread, getOrigin(I->getDebugLoc()) }, "", I);
      BranchInst* BI = BranchInst::Create(ResumeB, I);
      BI->setDebugLoc(I->getDebugLoc());
      I->eraseFromParent();
      return;
    }

    if (isa<IndirectBrInst>(I)) {
      llvm_unreachable("Don't support IndirectBr yet (and maybe never will)");
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

  bool isSetjmp(Function* F) {
    return (F->getName() == "setjmp" ||
            F->getName() == "_setjmp" ||
            F->getName() == "sigsetjmp");
  }

  JmpBufKind getJmpBufKindForSetjmp(Function* F) {
    if (F->getName() == "setjmp")
      return JmpBufKind::setjmp;
    if (F->getName() == "_setjmp")
      return JmpBufKind::_setjmp;
    if (F->getName() == "sigsetjmp")
      return JmpBufKind::sigsetjmp;
    llvm_unreachable("Bad setjmp kind");
    return JmpBufKind::setjmp;
  }

  bool shouldPassThrough(Function* F) {
    return (F->getName() == "__divdc3" ||
            F->getName() == "__muldc3" ||
            F->getName() == "__divsc3" ||
            F->getName() == "__mulsc3" ||
            F->getName() == "__mulxc3" ||
            isSetjmp(F));
  }

  bool shouldPassThrough(GlobalVariable* G) {
    return (G->getName() == "llvm.global_ctors" ||
            G->getName() == "llvm.global_dtors" ||
            G->getName() == "llvm.used" ||
            G->getName() == "llvm.compiler.used");
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
          { ConstantInt::get(IntPtrTy, DLBefore.getTypeAllocSize(G->getInitializer()->getType())) },
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

  void makeEHDatas() {
    std::vector<LandingPadInst*> LPIs;
    
    for (Function& F : M.functions()) {
      for (BasicBlock& BB : F) {
        LandingPadInst* LPI = dyn_cast<LandingPadInst>(BB.getFirstNonPHI());
        if (LPI)
          LPIs.push_back(LPI);
      }
    }

    if (LPIs.empty())
      return;
    
    std::vector<Constant*> LowTypesAndFilters;
    unsigned NumTypes = 0;
    unsigned NumFilters = 0;
    std::unordered_map<Constant*, int> TypeOrFilterToAction;

    for (LandingPadInst* LPI : LPIs) {
      for (unsigned Idx = LPI->getNumClauses(); Idx--;) {
        if (!LPI->isCatch(Idx))
          continue;

        if (TypeOrFilterToAction.count(LPI->getClause(Idx)))
          continue;

        int EHTypeID = NumTypes++ + 1;
        TypeOrFilterToAction[LPI->getClause(Idx)] = EHTypeID;
        EHTypeIDs[LPI->getClause(Idx)] = EHTypeID;
        LowTypesAndFilters.push_back(LPI->getClause(Idx));
      }
    }

    for (LandingPadInst* LPI : LPIs) {
      for (unsigned ClauseIdx = LPI->getNumClauses(); ClauseIdx--;) {
        if (!LPI->isFilter(ClauseIdx))
          continue;

        Constant* C = LPI->getClause(ClauseIdx);
        if (TypeOrFilterToAction.count(C))
          continue;

        StructType* FilterTy;
        Constant* FilterCS;
        if (cast<ArrayType>(C->getType())->getNumElements()) {
          FilterTy = StructType::get(this->C, { Int32Ty, C->getType() });
          FilterCS = ConstantStruct::get(
            FilterTy,
            { ConstantInt::get(Int32Ty, cast<ArrayType>(C->getType())->getNumElements()), C });
        } else {
          FilterTy = StructType::get(this->C, ArrayRef<Type*>(Int32Ty));
          FilterCS = ConstantStruct::get(FilterTy, ConstantInt::get(Int32Ty, 0));
        }
        Constant* LowC = new GlobalVariable(
          M, FilterTy, true, GlobalValue::PrivateLinkage, FilterCS, "filc_eh_filter");
        TypeOrFilterToAction[C] = -(NumFilters++ + 1);
        LowTypesAndFilters.push_back(LowC);
      }
    }

    Constant* TypeTableC;
    if (LowTypesAndFilters.empty())
      TypeTableC = LowRawNull;
    else {
      ArrayType* TypeTableArrayTy = ArrayType::get(LowRawPtrTy, LowTypesAndFilters.size());
      StructType* TypeTableTy = StructType::get(C, { Int32Ty, TypeTableArrayTy });
      Constant* TypeTableCS = ConstantStruct::get(
        TypeTableTy,
        { ConstantInt::get(Int32Ty, NumTypes),
          ConstantArray::get(TypeTableArrayTy, LowTypesAndFilters) });
      TypeTableC = new GlobalVariable(
        M, TypeTableTy, true, GlobalValue::PrivateLinkage, TypeTableCS, "filc_type_table");
    }

    for (LandingPadInst* LPI : LPIs) {
      if (EHDatas.count(EHDataKey(LPI)))
        continue;

      std::vector<int> Actions;

      for (unsigned Idx = 0; Idx < LPI->getNumClauses(); ++Idx) {
        assert(TypeOrFilterToAction.count(LPI->getClause(Idx)));
        Actions.push_back(TypeOrFilterToAction[LPI->getClause(Idx)]);
      }
      
      if (LPI->isCleanup())
        Actions.push_back(0);

      StructType* EHDataTy = StructType::get(
        C, { LowRawPtrTy, Int32Ty, ArrayType::get(Int32Ty, Actions.size()) });
      Constant* EHDataCS = ConstantStruct::get(
        EHDataTy,
        { TypeTableC, ConstantInt::get(Int32Ty, Actions.size()), ConstantDataArray::get(C, Actions) });
      EHDatas[EHDataKey(LPI)] = new GlobalVariable(
        M, EHDataTy, true, GlobalValue::PrivateLinkage, EHDataCS, "filc_eh_data");
      if (verbose)
        errs() << "For " << *LPI << " created eh data: " << *EHDatas[EHDataKey(LPI)] << "\n";
    }
  }

  GlobalValue* getGlobal(StringRef Name) {
    if (GlobalVariable* GV = M.getNamedGlobal(Name))
      return GV;
    if (GlobalAlias* GA = M.getNamedAlias(Name))
      return GA;
    if (Function* F = M.getFunction(Name))
      return F;
    return nullptr;
  }

  void compileModuleAsm() {
    MATokenizer MAT(M.getModuleInlineAsm());

    for (;;) {
      MAToken Tok = MAT.getNext();
      if (Tok.Kind == MATokenKind::None)
        break;
      if (Tok.Kind == MATokenKind::Error) {
        errs() << "Cannot parse module asm: " << Tok.Str << "\n";
        llvm_unreachable("Error parsing module asm");
      }
      if (Tok.Kind == MATokenKind::EndLine)
        continue;
      if (Tok.Kind == MATokenKind::Directive) {
        if (Tok.Str == ".filc_weak" ||
            Tok.Str == ".filc_globl") {
          std::string Name = MAT.getNextSpecific(MATokenKind::Identifier).Str;
          MAT.getNextSpecific(MATokenKind::EndLine);
          GlobalValue* GV = getGlobal(Name);
          if (!GV) {
            errs() << "Cannot do " << Tok.Str << " to " << Name << " because it doesn't exist.\n";
            llvm_unreachable("Error interpreting module asm");
          }
          if (Tok.Str == ".filc_weak")
            GV->setLinkage(GlobalValue::WeakAnyLinkage);
          else {
            assert(Tok.Str == ".filc_globl");
            GV->setLinkage(GlobalValue::ExternalLinkage);
          }
          continue;
        }
        if (Tok.Str == ".filc_weak_alias" ||
            Tok.Str == ".filc_alias") {
          std::string OldName = MAT.getNextSpecific(MATokenKind::Identifier).Str;
          MAT.getNextSpecific(MATokenKind::Comma);
          std::string NewName = MAT.getNextSpecific(MATokenKind::Identifier).Str;
          MAT.getNextSpecific(MATokenKind::EndLine);
          GlobalValue* GV = getGlobal(OldName);
          Type* T;
          GlobalValue* ExistingGV = getGlobal(NewName);
          if (ExistingGV) {
            T = ExistingGV->getValueType();
            assert(ExistingGV->isDeclaration());
            assert(!Dummy->getNumUses());
            if (!GV) {
              if (isa<Function>(ExistingGV)) {
                assert(isa<FunctionType>(T));
                GV = Function::Create(
                  cast<FunctionType>(T), GlobalValue::ExternalLinkage, OldName, &M);
              } else {
                assert(isa<GlobalVariable>(ExistingGV));
                GV = new GlobalVariable(M, T, false, GlobalValue::ExternalLinkage, nullptr, OldName);
              }
            }
            ExistingGV->replaceAllUsesWith(Dummy);
            ExistingGV->eraseFromParent();
          } else if (GV)
            T = GV->getValueType();
          else {
            T = Int8Ty;
            GV = new GlobalVariable(M, Int8Ty, false, GlobalValue::ExternalLinkage, nullptr, OldName);
          }
          GlobalValue::LinkageTypes Linkage;
          if (Tok.Str == ".filc_weak_alias")
            Linkage = GlobalValue::WeakAnyLinkage;
          else {
            assert(Tok.Str == ".filc_alias");
            Linkage = GlobalValue::ExternalLinkage;
          }
          GlobalAlias* NewGA = GlobalAlias::create(T, GV->getAddressSpace(), Linkage, NewName, GV, &M);
          if (ExistingGV)
            Dummy->replaceAllUsesWith(NewGA);
          continue;
        }
        errs() << "Invalid directive: " << Tok.Str << "\n";
        llvm_unreachable("Error parsing module asm");
      }
      errs() << "Unexpected token: " << Tok.Str << "\n";
      llvm_unreachable("Error parsing module asm");
    }

    M.setModuleInlineAsm("");
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

      // FIXME: Maybe we don't need to split critical edges in those cases where the target is a
      // landing pad?
      //
      // We split critical edges to handle the case where a phi has a constant input, and we need to
      // lower the constant in the predecessor block. However, it's not clear if it's even necessary
      // to split critical edges for that case. If we don't split, then at worst, the predecessors of
      // phis, where the phi is reached via a critical edge, will have some constant lowering that is
      // dead along the other edges.
      SplitAllCriticalEdges(F);
    }
  }

public:
  Pizlonator(Module &M)
    : C(M.getContext()), M(M), DLBefore(M.getDataLayout()), DL(M.getDataLayoutAfterFilC()) {
  }

  void run() {
    if (verbose)
      errs() << "Going to town on module:\n" << M << "\n";

    assert(DLBefore.getPointerSizeInBits(TargetAS) == 128);
    assert(DLBefore.getPointerABIAlignment(TargetAS) == 16);
    assert(DLBefore.isNonIntegralAddressSpace(TargetAS));
    assert(DL.getPointerSizeInBits(TargetAS) == 64);
    assert(DL.getPointerABIAlignment(TargetAS) == 8);
    assert(!DL.isNonIntegralAddressSpace(TargetAS));

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
    SetjmpTy = FunctionType::get(Int32Ty, LowRawPtrTy, false);
    SigsetjmpTy = FunctionType::get(Int32Ty, { LowRawPtrTy, Int32Ty }, false);
    LowRawNull = ConstantPointerNull::get(LowRawPtrTy);

    Dummy = makeDummy(Int32Ty);
    
    lowerThreadLocals();
    makeEHDatas();
    compileModuleAsm();
    
    if (verbose)
      errs() << "Module with lowered thread locals, EH data, and module asm:\n" << M << "\n";

    prepare();

    // Super important that we do this *before* emitting code, because otherwise, llvm will assume
    // that the alignment of any lowered ptr loads/stores we emit is 16 bytes.
    M.setDataLayout(M.getDataLayoutAfterFilC());
    
    if (verbose)
      errs() << "Prepared module:\n" << M << "\n";

    FunctionName = "<internal>";
    
    LowWidePtrTy = StructType::create({Int128Ty}, "filc_wide_ptr");
    FunctionOriginTy = StructType::create(
      {LowRawPtrTy, LowRawPtrTy, Int32Ty, LowRawPtrTy, Int8Ty, Int8Ty, Int32Ty}, "filc_function_origin");
    OriginTy = StructType::create({LowRawPtrTy, Int32Ty, Int32Ty}, "filc_origin");
    OriginWithEHTy = StructType::create(
      {LowRawPtrTy, Int32Ty, Int32Ty, LowRawPtrTy}, "filc_origin_with_eh");
    ObjectTy = StructType::create({LowRawPtrTy, LowRawPtrTy, Int16Ty, Int8Ty}, "filc_object");
    CCTypeTy = StructType::create({IntPtrTy, Int8Ty}, "filc_cc_type");
    CCPtrTy = StructType::create({LowRawPtrTy, LowRawPtrTy}, "filc_cc_ptr");
    FrameTy = StructType::create({LowRawPtrTy, LowRawPtrTy, LowRawPtrTy}, "filc_frame");
    ThreadTy = StructType::create(
      {Int8Ty, Int32Ty, LowRawPtrTy, ArrayType::get(LowWidePtrTy, NumUnwindRegisters), LowWidePtrTy},
      "filc_thread_ish");
    ConstantRelocationTy = StructType::create(
      {IntPtrTy, Int32Ty, LowRawPtrTy}, "filc_constant_relocation");
    ConstexprNodeTy = StructType::create(
      { Int32Ty, Int32Ty, LowRawPtrTy, IntPtrTy}, "filc_constexpr_node");
    // See PIZLONATED_SIGNATURE in filc_runtime.h.
    PizlonatedFuncTy = FunctionType::get(
      Int1Ty, { LowRawPtrTy, CCPtrTy, CCPtrTy }, false);
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
        if (isSetjmp(&F)) {
          assert(F.hasFnAttribute(Attribute::ReturnsTwice));
          if (F.getName() == "setjmp" || F.getName() == "_setjmp") {
            if (F.getFunctionType() != SetjmpTy) {
              errs() << "Unexpected setjmp signature: " << *F.getFunctionType()
                     << ", expected: " << *SetjmpTy << "\n";
            }
            assert(F.getFunctionType() == SetjmpTy);
          } else {
            if (F.getFunctionType() != SigsetjmpTy) {
              errs() << "Unexpected setjmp signature: " << *F.getFunctionType()
                     << ", expected: " << *SigsetjmpTy << "\n";
            }
            assert(F.getName() == "sigsetjmp");
            assert(F.getFunctionType() == SigsetjmpTy);
          }
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

    FutureArgBuffer = makeDummy(LowRawPtrTy);
    FutureReturnBuffer = makeDummy(LowRawPtrTy);

    PollcheckSlow = M.getOrInsertFunction("filc_pollcheck_slow", VoidTy, LowRawPtrTy, LowRawPtrTy);
    StoreBarrierSlow = M.getOrInsertFunction("filc_store_barrier_slow", VoidTy, LowRawPtrTy, LowRawPtrTy);
    GetNextBytesForVAArg = M.getOrInsertFunction("filc_get_next_bytes_for_va_arg", LowWidePtrTy, LowRawPtrTy, LowWidePtrTy, IntPtrTy, IntPtrTy, LowRawPtrTy);
    Allocate = M.getOrInsertFunction("filc_allocate", LowRawPtrTy, LowRawPtrTy, IntPtrTy);
    AllocateWithAlignment = M.getOrInsertFunction("filc_allocate_with_alignment", LowRawPtrTy, LowRawPtrTy, IntPtrTy, IntPtrTy);
    CheckReadInt = M.getOrInsertFunction("filc_check_read_int", VoidTy, LowWidePtrTy, IntPtrTy, LowRawPtrTy);
    CheckReadAlignedInt = M.getOrInsertFunction("filc_check_read_aligned_int", VoidTy, LowWidePtrTy, IntPtrTy, IntPtrTy, LowRawPtrTy);
    CheckReadInt8Fail = M.getOrInsertFunction("filc_check_read_int8_fail", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckReadInt16Fail = M.getOrInsertFunction("filc_check_read_int16_fail", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckReadInt32Fail = M.getOrInsertFunction("filc_check_read_int32_fail", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckReadInt64Fail = M.getOrInsertFunction("filc_check_read_int64_fail", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckReadInt128Fail = M.getOrInsertFunction("filc_check_read_int128_fail", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckReadPtrFail = M.getOrInsertFunction("filc_check_read_ptr_fail", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckWriteInt = M.getOrInsertFunction("filc_check_write_int", VoidTy, LowWidePtrTy, IntPtrTy, LowRawPtrTy);
    CheckWriteAlignedInt = M.getOrInsertFunction("filc_check_write_aligned_int", VoidTy, LowWidePtrTy, IntPtrTy, IntPtrTy, LowRawPtrTy);
    CheckWriteInt8Fail = M.getOrInsertFunction("filc_check_write_int8_fail", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckWriteInt16Fail = M.getOrInsertFunction("filc_check_write_int16_fail", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckWriteInt32Fail = M.getOrInsertFunction("filc_check_write_int32_fail", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckWriteInt64Fail = M.getOrInsertFunction("filc_check_write_int64_fail", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckWriteInt128Fail = M.getOrInsertFunction("filc_check_write_int128_fail", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckWritePtrFail = M.getOrInsertFunction("filc_check_write_ptr_fail", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckFunctionCallFail = M.getOrInsertFunction("filc_check_function_call_fail", VoidTy, LowWidePtrTy);
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
    LandingPad = M.getOrInsertFunction("filc_landing_pad", Int1Ty, LowRawPtrTy);
    ResumeUnwind = M.getOrInsertFunction("filc_resume_unwind", VoidTy, LowRawPtrTy, LowRawPtrTy);
    JmpBufCreate = M.getOrInsertFunction("filc_jmp_buf_create", LowRawPtrTy, LowRawPtrTy, Int32Ty, Int32Ty);
    PromoteArgsToHeap = M.getOrInsertFunction("filc_promote_args_to_heap", LowWidePtrTy, LowRawPtrTy, CCPtrTy, IntPtrTy);
    CCArgsCheckFailure = M.getOrInsertFunction("filc_cc_args_check_failure", VoidTy, CCPtrTy, LowRawPtrTy, LowRawPtrTy);
    CCRetsCheckFailure = M.getOrInsertFunction("filc_cc_rets_check_failure", VoidTy, CCPtrTy, LowRawPtrTy, LowRawPtrTy);
    _Setjmp = M.getOrInsertFunction("_setjmp", Int32Ty, LowRawPtrTy);
    cast<Function>(_Setjmp.getCallee())->addFnAttr(Attribute::ReturnsTwice);

    EmptyCCType = M.getOrInsertGlobal("filc_empty_cc_type", CCTypeTy);
    VoidCCType = M.getOrInsertGlobal("filc_void_cc_type", CCTypeTy);
    IntCCType = M.getOrInsertGlobal("filc_int_cc_type", CCTypeTy);
    PtrCCType = M.getOrInsertGlobal("filc_ptr_cc_type", CCTypeTy);
    IsMarking = M.getOrInsertGlobal("filc_is_marking", Int8Ty);

    auto FixupTypes = [&] (GlobalValue* G, GlobalValue* NewG) {
      GlobalHighTypes[NewG] = GlobalHighTypes[G];
      GlobalLowTypes[NewG] = GlobalLowTypes[G];
    };

    std::vector<GlobalValue*> ToDelete;
    auto HandleGlobal = [&] (GlobalValue* G) {
      if (verbose)
        errs() << "Handling global: " << G->getName() << "\n";
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
    if (verbose) {
      errs() << "ToDelete values:";
      for (GlobalValue* GV : ToDelete)
        errs() << " " << GV->getName();
      errs() << "\n";
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

    auto HandleUsed = [&] (GlobalVariable* Used) {
      ConstantArray* Array = cast<ConstantArray>(Used->getInitializer());
      std::vector<Constant*> Args;
      for (size_t Index = 0; Index < Array->getNumOperands(); ++Index) {
        // NOTE: This could have a GEP, supposedly. Pretend it can't for now.
        Function* GetterF = GlobalToGetter[cast<GlobalValue>(Array->getOperand(Index))];
        assert(GetterF);
        Args.push_back(GetterF);
      }
      Used->setInitializer(ConstantArray::get(Array->getType(), Args));
    };
    if (GlobalVariable* Used = M.getGlobalVariable("llvm.used"))
      HandleUsed(Used);
    if (GlobalVariable* Used = M.getGlobalVariable("llvm.compiler.used"))
      HandleUsed(Used);

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

        SmallVector<std::pair<const BasicBlock*, const BasicBlock*>> BackEdges;
        FindFunctionBackedges(*F, BackEdges);
        std::unordered_set<const BasicBlock*> LoopFooters;
        for (std::pair<const BasicBlock*, const BasicBlock*>& Edge : BackEdges)
          LoopFooters.insert(Edge.first);
      
        MyThread = NewF->getArg(0);
        FixupTypes(F, NewF);
        std::vector<BasicBlock*> Blocks;
        for (BasicBlock& BB : *F)
          Blocks.push_back(&BB);
        assert(!Blocks.empty());
        ArgBufferSize = 0;
        ArgBufferAlignment = 0;
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

          if (LoopFooters.count(BB)) {
            DebugLoc DL = BB->getTerminator()->getDebugLoc();
            GetElementPtrInst* StatePtr = GetElementPtrInst::Create(
              ThreadTy, MyThread,
              { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 0) },
              "filc_thread_state_ptr", BB->getTerminator());
            StatePtr->setDebugLoc(DL);
            LoadInst* StateLoad = new LoadInst(
              Int8Ty, StatePtr, "filc_thread_state_load", BB->getTerminator());
            StateLoad->setDebugLoc(DL);
            BinaryOperator* Masked = BinaryOperator::Create(
              Instruction::And, StateLoad,
              ConstantInt::get(
                Int8Ty,
                ThreadStateCheckRequested | ThreadStateStopRequested | ThreadStateDeferredSignal),
              "filc_thread_state_masked", BB->getTerminator());
            Masked->setDebugLoc(DL);
            ICmpInst* PollcheckNotNeeded = new ICmpInst(
              BB->getTerminator(), ICmpInst::ICMP_EQ, Masked, ConstantInt::get(Int8Ty, 0),
              "filc_pollcheck_not_needed");
            Masked->setDebugLoc(DL);
            Instruction* NewTerm =
              SplitBlockAndInsertIfElse(PollcheckNotNeeded, BB->getTerminator(), false);
            CallInst::Create(
              PollcheckSlow, { MyThread, getOrigin(DL) }, "", NewTerm)->setDebugLoc(DL);
          }
        }

        ReturnB = BasicBlock::Create(C, "filc_return_block", NewF);
        if (F->getReturnType() != VoidTy)
          ReturnPhi = PHINode::Create(lowerType(F->getReturnType()), 1, "filc_return_value", ReturnB);

        ReallyReturnB = BasicBlock::Create(C, "filc_really_return_block", NewF);
        BranchInst* ReturnBranch = BranchInst::Create(ReallyReturnB, ReturnB);
        ReturnInst* Return = ReturnInst::Create(C, ConstantInt::getFalse(Int1Ty), ReallyReturnB);

        if (F->getReturnType() != VoidTy) {
          Type* LowT = lowerType(F->getReturnType());
          checkCCType(LowT, NewF->getArg(2), CCRetsCheckFailure, ReturnBranch);
          storeValueRecurseAfterCheck(
            LowT, ReturnPhi, ccPtrBase(NewF->getArg(2), ReturnBranch), false,
            DL.getABITypeAlign(LowT), AtomicOrdering::NotAtomic, SyncScope::System, MemoryKind::CC,
            ReturnBranch);
        }

        ResumeB = BasicBlock::Create(C, "filc_resume_block", NewF);
        ReturnInst* ResumeReturn = ReturnInst::Create(C, ConstantInt::getTrue(Int1Ty), ResumeB);

        Instruction* InsertionPoint = &*Blocks[0]->getFirstInsertionPt();
        StructType* MyFrameTy = StructType::get(
          C, { LowRawPtrTy, LowRawPtrTy, ArrayType::get(LowRawPtrTy, FrameSize) });
        Frame = new AllocaInst(MyFrameTy, 0, "filc_my_frame", InsertionPoint);
        Value* ThreadTopFramePtr = GetElementPtrInst::Create(
          ThreadTy, MyThread,
          { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 2) },
          "filc_thread_top_frame_ptr", InsertionPoint);
        new StoreInst(
          new LoadInst(LowRawPtrTy, ThreadTopFramePtr, "filc_thread_top_frame", InsertionPoint),
          GetElementPtrInst::Create(
            FrameTy, Frame, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 0) },
            "filc_frame_parent_ptr", InsertionPoint),
          InsertionPoint);
        new StoreInst(Frame, ThreadTopFramePtr, InsertionPoint);
        new StoreInst(
          getOrigin(DebugLoc()),
          GetElementPtrInst::Create(
            FrameTy, Frame, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) },
            "filc_frame_parent_ptr", InsertionPoint),
          InsertionPoint);
        for (size_t FrameIndex = FrameSize; FrameIndex--;)
          recordObjectAtIndex(LowRawNull, FrameIndex, InsertionPoint);

        auto PopFrame = [&] (Instruction* Return) {
          new StoreInst(
            new LoadInst(
              LowRawPtrTy,
              GetElementPtrInst::Create(
                FrameTy, Frame, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 0) },
                "filc_frame_parent_ptr", Return),
              "filc_frame_parent", Return),
            GetElementPtrInst::Create(
              ThreadTy, MyThread,
              { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 2) },
              "filc_thread_top_frame_ptr", Return),
            Return);
        };
        PopFrame(Return);
        PopFrame(ResumeReturn);

        Value* ArgCCPtr = NewF->getArg(1);

        StructType* ArgsTy = cast<StructType>(lowerType(argsType(F->getFunctionType())));
        checkCCType(ArgsTy, ArgCCPtr, CCArgsCheckFailure, InsertionPoint);

        Value* RawDataPtr = ccPtrBase(ArgCCPtr, InsertionPoint);
        const StructLayout* SL = DL.getStructLayout(ArgsTy);
        assert(F->getFunctionType()->getNumParams() == ArgsTy->getNumElements());
        size_t LastOffset;
        for (unsigned Index = 0; Index < F->getFunctionType()->getNumParams(); ++Index) {
          Type* LowT = ArgsTy->getElementType(Index);
          size_t ArgOffset = SL->getElementOffset(Index);
          Instruction* ArgPtr = GetElementPtrInst::Create(
            Int8Ty, RawDataPtr, { ConstantInt::get(IntPtrTy, ArgOffset) }, "filc_arg_ptr",
            InsertionPoint);
          Value* V = loadValueRecurseAfterCheck(
            LowT, ArgPtr, false, DL.getABITypeAlign(LowT), AtomicOrdering::NotAtomic,
            SyncScope::System, MemoryKind::CC, InsertionPoint);
          Args.push_back(V);
          LastOffset = ArgOffset + DL.getTypeAllocSize(LowT);
        }
        if (UsesVastart) {
          // Do this after we have recorded all the args for GC, so it's safe to have a pollcheck.
          SnapshottedArgsPtr = CallInst::Create(
            PromoteArgsToHeap, { MyThread, ArgCCPtr, ConstantInt::get(IntPtrTy, LastOffset) }, "",
            InsertionPoint);
          recordObjectAtIndex(
            ptrObject(SnapshottedArgsPtr, InsertionPoint), SnapshottedArgsFrameIndex, InsertionPoint);
        }

        FirstRealBlock = InsertionPoint->getParent();

        std::vector<PHINode*> Phis;
        for (Instruction* I : Instructions) {
          if (PHINode* Phi = dyn_cast<PHINode>(I)) {
            Phis.push_back(Phi);
            continue;
          }
          if (!Phis.empty()) {
            for (PHINode* Phi : Phis)
              recordObjects(Phi, I);
            Phis.clear();
          }
          if (InvokeInst* II = dyn_cast<InvokeInst>(I))
            recordObjects(I, &*II->getNormalDest()->getFirstInsertionPt());
          else if (I->isTerminator())
            assert(I->getType()->isVoidTy());
          else
            recordObjects(I, I->getNextNode());
        }
        assert(Phis.empty());
        erase_if(Instructions, [&] (Instruction* I) { return earlyLowerInstruction(I); });
        for (Instruction* I : Instructions)
          lowerInstruction(I, LowRawNull);

        InsertionPoint = &*Blocks[0]->getFirstInsertionPt();
        auto MakeCCBuffer = [&] (BitCastInst* FutureBuffer, size_t Size, size_t Alignment) {
          if (FutureBuffer->hasNUsesOrMore(1)) {
            assert(Size);
            assert(Alignment);
            FutureBuffer->replaceAllUsesWith(
              new AllocaInst(
                Int8Ty, 0, ConstantInt::get(IntPtrTy, Size), Align(Alignment),
                "filc_cc_buffer", InsertionPoint));
          } else {
            assert(!Size);
            assert(!Alignment);
          }
        };
        MakeCCBuffer(FutureArgBuffer, ArgBufferSize, ArgBufferAlignment);
        MakeCCBuffer(FutureReturnBuffer, ReturnBufferSize, ReturnBufferAlignment);
        
        GlobalVariable* NewObjectG = objectGlobalForGlobal(NewF, OldF);
        
        Function* GetterF = GlobalToGetter[OldF];
        assert(GetterF);
        assert(GetterF->isDeclaration());
        BasicBlock* RootBB = BasicBlock::Create(C, "filc_function_getter_root", GetterF);
        Return = ReturnInst::Create(C, UndefValue::get(LowWidePtrTy), RootBB);
        Return->getOperandUse(0) = createPtr(NewObjectG, NewF, Return);

        if (Setjmps.size())
          assert(NewF->callsFunctionThatReturnsTwice());
        
        if (verbose)
          errs() << "New function: " << *NewF << "\n";
      }
      
      FunctionName = "<internal>";
      OldF = nullptr;
      NewF = nullptr;
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

    Dummy->deleteValue();
    FutureArgBuffer->deleteValue();
    FutureReturnBuffer->deleteValue();

    if (verbose)
      errs() << "Deleting ToErase values.\n";
    for (Instruction* I : ToErase)
      I->deleteValue();
    if (verbose)
      errs() << "RAUWing ToDelete values.\n";
    for (GlobalValue* G : ToDelete)
      G->replaceAllUsesWith(UndefValue::get(G->getType())); // FIXME - should be zero
    if (verbose)
      errs() << "Erasing ToDelete values.\n";
    for (GlobalValue* G : ToDelete) {
      if (verbose)
        errs() << "Erasing " << G->getName() << "\n";
      G->eraseFromParent();
    }

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

