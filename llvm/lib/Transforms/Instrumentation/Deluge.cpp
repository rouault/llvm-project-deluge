#include "llvm/Transforms/Instrumentation/Deluge.h"

#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/TypedPointerType.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/TargetParser/Triple.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace llvm;

// This is the early 2000's Howard Stern of compiler passes.
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

static constexpr bool verbose = false;
static constexpr bool ultraVerbose = false;

static constexpr size_t MinAlign = 16;
static constexpr size_t WordSize = 16;

// This has to match the Deluge runtime.
enum class DelugeWordType {
  OffLimits = 0,
  Int = 1,
  PtrSidecar = 2,
  PtrCapability = 3
};

struct CoreDelugeType {
  size_t Size { 0 };
  size_t Alignment { 0 };
  std::vector<DelugeWordType> WordTypes;

  CoreDelugeType() = default;
  
  CoreDelugeType(size_t Size, size_t Alignment)
    : Size(Size), Alignment(Alignment) {
  }

  bool isValid() const { return !!Size; }

  bool operator==(const CoreDelugeType& Other) const {
    return Size == Other.Size && Alignment == Other.Alignment && WordTypes == Other.WordTypes;
  }

  size_t hash() const {
    size_t Result = Size + Alignment * 3;
    for (DelugeWordType WordType : WordTypes) {
      Result *= 7;
      Result += static_cast<size_t>(WordType);
    }
    return Result;
  }

  bool canBeInt() const {
    if (WordTypes.empty())
      return false;
    for (DelugeWordType WT : WordTypes) {
      if (WT != DelugeWordType::Int)
        return false;
    }
    return true;
  }

  void pad(size_t Alignment) {
    if (Size % WordSize) {
      assert(!WordTypes.empty());
      assert(WordTypes.back() == DelugeWordType::Int);
    }
    Size = (Size + Alignment - 1) / Alignment * Alignment;
    while ((Size + WordSize - 1) / WordSize > WordTypes.size())
      WordTypes.push_back(DelugeWordType::OffLimits);
  }

  // Append two types to each other. Does not work for special types like int or func. Returns the
  // offset that the Other type was appended at.
  size_t append(const CoreDelugeType& Other) {
    assert(!(Size % WordSize));
    assert(!(Other.Size % WordSize));

    Alignment = std::max(Alignment, Other.Alignment);

    pad(Alignment);
    size_t Result = Size;
    WordTypes.insert(WordTypes.end(), Other.WordTypes.begin(), Other.WordTypes.end());
    Size += Other.Size;
    return Result;
  }

  void truncate(size_t NewSize) {
    assert(NewSize <= Size);
    if (NewSize == Size)
      return;
    Size = NewSize;
    while (WordTypes.size() > (Size + WordSize - 1) / WordSize)
      WordTypes.pop_back();
  }
};

struct DelugeType {
  CoreDelugeType Main;
  CoreDelugeType Trailing;

  DelugeType() = default;
  
  DelugeType(size_t Size, size_t Alignment)
    : Main(Size, Alignment) {
  }

  bool canBeInt() const { return Main.canBeInt() && (!Trailing.isValid() || Trailing.canBeInt()); }

  bool operator==(const DelugeType& Other) const {
    return Main == Other.Main && Trailing == Other.Trailing;
  }

  size_t hash() const {
    return Main.hash() + 11 * Trailing.hash();
  }
};

} // anonymous namespace

template<> struct std::hash<DelugeType> {
  size_t operator()(const DelugeType& Key) const {
    return Key.hash();
  }
};

namespace {

struct DelugeTypeData {
  DelugeType Type;
  Constant* TemplateRep { nullptr };
};

enum class ConstantPoolEntryKind {
  Type,
  Heap,
  HardHeap
};

struct ConstantPoolEntry {
  ConstantPoolEntryKind Kind;
  DelugeTypeData* DTD;

  bool operator==(const ConstantPoolEntry& Other) const {
    return Kind == Other.Kind && DTD == Other.DTD;
  }

  size_t hash() const {
    size_t Result = static_cast<size_t>(Kind) * 666;
    Result += reinterpret_cast<size_t>(DTD);
    return Result;
  }
};

} // anonymous namespace

template<> struct std::hash<ConstantPoolEntry> {
  size_t operator()(const ConstantPoolEntry& Key) const {
    return Key.hash();
  }
};

namespace {

enum class ConstantKind {
  Function,
  Global
};

struct ConstantRelocation {
  ConstantRelocation() { }

  ConstantRelocation(size_t Offset, ConstantKind Kind, Function* Target)
    : Offset(Offset)
    , Kind(Kind)
    , Target(Target)
  {
  }
  
  size_t Offset { 0 };
  ConstantKind Kind { ConstantKind::Function };
  Function* Target { nullptr }; // Getter for globals, function for functions
};

class Deluge {
  static constexpr unsigned TargetAS = 0;
  
  LLVMContext& C;
  Module &M;
  DataLayout& DL;
  ModuleAnalysisManager &MAM;

  unsigned PtrBits;
  Type* VoidTy;
  Type* Int1Ty;
  Type* Int8Ty;
  Type* Int32Ty;
  Type* IntPtrTy;
  Type* Int128Ty;
  PointerType* LowRawPtrTy;
  StructType* LowWidePtrTy;
  StructType* OriginTy;
  StructType* AllocaStackTy;
  StructType* ConstantRelocationTy;
  FunctionType* DeludedFuncTy;
  FunctionType* GlobalGetterTy;
  FunctionType* CtorDtorTy;
  Constant* LowRawNull;
  Constant* LowWideNull;
  BitCastInst* Dummy;

  // High-level functions available to the user.
  Value* ZunsafeForgeImpl;
  Value* ZrestrictImpl;
  Value* ZallocImpl;
  Value* ZallocFlexImpl;
  Value* ZalignedAllocImpl;
  Value* ZreallocImpl;
  Value* ZhardAllocImpl;
  Value* ZhardAllocFlexImpl;
  Value* ZhardAlignedAllocImpl;
  Value* ZhardReallocImpl;
  Value* ZtypeofImpl;

  // Low-level functions used by codegen.
  FunctionCallee ValidateType;
  FunctionCallee GetType;
  FunctionCallee GetHeap;
  FunctionCallee TryAllocateInt;
  FunctionCallee TryAllocateIntWithAlignment;
  FunctionCallee AllocateInt;
  FunctionCallee AllocateIntWithAlignment;
  FunctionCallee TryAllocateOne;
  FunctionCallee AllocateOne;
  FunctionCallee TryAllocateMany;
  FunctionCallee TryAllocateManyWithAlignment;
  FunctionCallee AllocateMany;
  FunctionCallee TryAllocateIntFlex;
  FunctionCallee TryAllocateIntFlexWithAlignment;
  FunctionCallee TryAllocateFlex;
  FunctionCallee AllocateUtility;
  FunctionCallee TryReallocateInt;
  FunctionCallee TryReallocateIntWithAlignment;
  FunctionCallee TryReallocate;
  FunctionCallee Deallocate;
  FunctionCallee GetHardHeap;
  FunctionCallee TryHardAllocateInt;
  FunctionCallee TryHardAllocateIntWithAlignment;
  FunctionCallee TryHardAllocateOne;
  FunctionCallee TryHardAllocateMany;
  FunctionCallee TryHardAllocateManyWithAlignment;
  FunctionCallee TryHardAllocateIntFlex;
  FunctionCallee TryHardAllocateIntFlexWithAlignment;
  FunctionCallee TryHardAllocateFlex;
  FunctionCallee TryHardReallocateInt;
  FunctionCallee TryHardReallocateIntWithAlignment;
  FunctionCallee TryHardReallocate;
  FunctionCallee PtrPtr;
  FunctionCallee UpdateSidecar;
  FunctionCallee UpdateCapability;
  FunctionCallee NewSidecar;
  FunctionCallee NewCapability;
  FunctionCallee CheckForge;
  FunctionCallee CheckAccessInt;
  FunctionCallee CheckAccessPtr;
  FunctionCallee CheckAccessFunctionCall;
  FunctionCallee Memset;
  FunctionCallee Memcpy;
  FunctionCallee Memmove;
  FunctionCallee CheckRestrict;
  FunctionCallee VAArgImpl;
  FunctionCallee GlobalInitializationContextCreate;
  FunctionCallee GlobalInitializationContextAdd;
  FunctionCallee GlobalInitializationContextDestroy;
  FunctionCallee AllocaStackPush;
  FunctionCallee AllocaStackRestore;
  FunctionCallee AllocaStackDestroy;
  FunctionCallee ExecuteConstantRelocations;
  FunctionCallee DeferOrRunGlobalCtor;
  FunctionCallee Error;
  FunctionCallee RealMemset;

  // Bonus functions generated by us.
  FunctionCallee MakeConstantPool;

  GlobalVariable* GlobalConstantPoolPtr;

  std::unordered_map<std::string, GlobalVariable*> Strings;
  std::unordered_map<DILocation*, GlobalVariable*> Origins;

  std::vector<GlobalVariable*> Globals;
  std::vector<Function*> Functions;
  std::vector<GlobalAlias*> Aliases;
  std::vector<GlobalIFunc*> IFuncs;
  std::unordered_map<GlobalValue*, Type*> GlobalLowTypes;
  std::unordered_map<GlobalValue*, Type*> GlobalHighTypes;

  std::unordered_map<Type*, Type*> LoweredTypes;

  std::unordered_map<Type*, DelugeTypeData*> TypeMap;
  std::unordered_map<DelugeType, std::unique_ptr<DelugeTypeData>> TypeDatas;
  DelugeTypeData Int;
  DelugeTypeData FunctionDTD;
  DelugeTypeData TypeDTD;
  DelugeTypeData Invalid;
  Value* IntTypeRep;
  Value* FunctionTypeRep;
  Value* TypeTypeRep;

  std::unordered_map<GlobalValue*, Function*> GlobalToGetter;
  std::unordered_map<GlobalValue*, GlobalVariable*> GlobalToGlobal;
  std::unordered_set<Value*> Getters;

  std::vector<ConstantPoolEntry> ConstantPoolEntries;
  std::unordered_map<ConstantPoolEntry, size_t> ConstantPoolEntryIndex;

  std::string FunctionName;
  Function* OldF;
  Function* NewF;

  std::unordered_map<Instruction*, Type*> InstLowTypes;
  std::unordered_map<Instruction*, std::vector<Type*>> InstLowTypeVectors;
  
  BasicBlock* FirstRealBlock;

  BitCastInst* FutureIntFrame;
  BitCastInst* FutureTypedFrame;
  BitCastInst* FutureReturnBuffer;
  BitCastInst* FutureAllocaStack;

  size_t IntFrameSize;
  size_t IntFrameAlignment;
  CoreDelugeType TypedFrameType;
  BasicBlock* ReturnB;
  PHINode* ReturnPhi;

  size_t ReturnBufferSize;
  size_t ReturnBufferAlignment;

  Value* ArgBufferPtr; /* This is left in a state where it's advanced past the last declared
                          arg. */
  std::vector<Value*> Args;

  PHINode* LocalConstantPoolPtr;

  BitCastInst* makeDummy(Type* T) {
    return new BitCastInst(UndefValue::get(T), T, "dummy");
  }

  GlobalVariable* getString(StringRef Str) {
    auto iter = Strings.find(Str.str());
    if (iter != Strings.end())
      return iter->second;

    Constant* C = ConstantDataArray::getString(this->C, Str);
    GlobalVariable* Result = new GlobalVariable(
      M, C->getType(), true, GlobalVariable::PrivateLinkage, C, "deluge_string");
    Strings[Str.str()] = Result;
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
      M, OriginTy, true, GlobalVariable::PrivateLinkage, C, "deluge_origin");
    Origins[Impl] = Result;
    return Result;
  }

  void buildCoreTypeRecurse(CoreDelugeType& CDT, Type* T) {
    CDT.Alignment = std::max(CDT.Alignment, static_cast<size_t>(DL.getABITypeAlign(T).value()));

    assert((CDT.Size + WordSize - 1) / WordSize == CDT.WordTypes.size());
    if (CDT.Size % WordSize) {
      assert(!CDT.WordTypes.empty());
      assert(CDT.WordTypes.back() == DelugeWordType::Int);
    }

    auto Fill = [&] () {
      while ((CDT.Size + WordSize - 1) / WordSize > CDT.WordTypes.size())
        CDT.WordTypes.push_back(DelugeWordType::Int);
    };

    assert(T != LowRawPtrTy);
    
    if (T == LowWidePtrTy) {
      CDT.Size += 32;
      CDT.WordTypes.push_back(DelugeWordType::PtrSidecar);
      CDT.WordTypes.push_back(DelugeWordType::PtrCapability);
      return;
    }

    if (StructType* ST = dyn_cast<StructType>(T)) {
      size_t SizeBefore = CDT.Size;
      const StructLayout* SL = DL.getStructLayout(ST);
      for (unsigned Index = 0; Index < ST->getNumElements(); ++Index) {
        Type* InnerT = ST->getElementType(Index);
        size_t ProposedSize = SizeBefore + SL->getElementOffset(Index);
        assert(ProposedSize >= CDT.Size);
        CDT.Size = ProposedSize;
        Fill();
        buildCoreTypeRecurse(CDT, InnerT);
      }
      size_t ProposedSize = SizeBefore + SL->getSizeInBytes();
      assert(ProposedSize >= CDT.Size);
      CDT.Size = ProposedSize;
      Fill();
      return;
    }
    
    if (ArrayType* AT = dyn_cast<ArrayType>(T)) {
      for (uint64_t Index = AT->getNumElements(); Index--;)
        buildCoreTypeRecurse(CDT, AT->getElementType());
      return;
    }
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(T)) {
      for (unsigned Index = VT->getElementCount().getFixedValue(); Index--;)
        buildCoreTypeRecurse(CDT, VT->getElementType());
      return;
    }

    if (ScalableVectorType* VT = dyn_cast<ScalableVectorType>(T)) {
      llvm_unreachable("Shouldn't see scalable vector types in checkRecurse");
      return;
    }

    CDT.Size += DL.getTypeStoreSize(T);
    while ((CDT.Size + WordSize - 1) / WordSize > CDT.WordTypes.size())
      CDT.WordTypes.push_back(DelugeWordType::Int);
  }

  void buildTypeRep(DelugeTypeData& Data) {
    DelugeType& T = Data.Type;
    
    DelugeTypeData* TrailingData = nullptr;
    if (T.Trailing.isValid()) {
      DelugeType TrailingT;
      TrailingT.Main = T.Trailing;
      TrailingData = dataForType(TrailingT);
    }

    assert(T.Main.Size);
    assert(T.Main.Alignment);
    assert(!(T.Main.Size % T.Main.Alignment) || T.Trailing.isValid());
    if (verbose) {
      errs() << "T.Trailing.isValid() = " << T.Trailing.isValid() << "\n";
      errs() << "T.Trailing.canBeInt() = " << T.Trailing.canBeInt() << "\n";
    }
    assert(!(T.Main.Size % WordSize) || (T.Trailing.isValid() && T.Trailing.canBeInt()));

    std::vector<Constant*> Constants;
    Constants.push_back(ConstantInt::get(IntPtrTy, T.Main.Size));
    Constants.push_back(ConstantInt::get(IntPtrTy, T.Main.Alignment));
    assert((T.Main.Size + WordSize - 1) / WordSize == T.Main.WordTypes.size());
    if (TrailingData)
      Constants.push_back(ConstantExpr::getPtrToInt(TrailingData->TemplateRep, IntPtrTy));
    else
      Constants.push_back(ConstantInt::get(IntPtrTy, 0));
    uint64_t Word = 0;
    size_t Shift = 0;
    auto Flush = [&] () {
      Constants.push_back(ConstantInt::get(IntPtrTy, Word));
      Word = 0;
      Shift = 0;
    };
    for (DelugeWordType Type : T.Main.WordTypes) {
      if (Shift >= PtrBits) {
        assert(Shift == PtrBits);
        Flush();
      }
      Word |= static_cast<uint64_t>(Type) << Shift;
      Shift += 8;
    }
    if (Shift)
      Flush();
    ArrayType* AT = ArrayType::get(IntPtrTy, Constants.size());
    Constant* CA = ConstantArray::get(AT, Constants);
    // FIXME: At some point, we'll want to content-address these.
    Data.TemplateRep =
      new GlobalVariable(M, AT, true, GlobalValue::PrivateLinkage, CA, "deluge_type_template");
  }

  DelugeTypeData* dataForType(const DelugeType& T) {
    if (T.canBeInt())
      return &Int;
    
    auto iter = TypeDatas.find(T);
    if (iter != TypeDatas.end())
      return iter->second.get();

    std::unique_ptr<DelugeTypeData> Data = std::make_unique<DelugeTypeData>();
    Data->Type = T;
    buildTypeRep(*Data);

    DelugeTypeData* Result = Data.get();
    TypeDatas.emplace(T, std::move(Data));
    return Result;
  }

  DelugeTypeData* dataForLowType(Type* T) {
    auto iter = TypeMap.find(T);
    if (iter != TypeMap.end())
      return iter->second;

    DelugeTypeData* Data;
    if (isa<FunctionType>(T))
      Data = &FunctionDTD;
    else if (!hasPtrsForCheck(T))
      Data = &Int;
    else {
      DelugeType DT;
      // Deluge types derived from llvm types never have a trailing component.
      buildCoreTypeRecurse(DT.Main, T);
      assert(DT.Main.Size == DL.getTypeStoreSize(T));
      assert(!(DT.Main.Size % DT.Main.Alignment));

      // FIXME: Find repetitions?
      
      Data = dataForType(DT);
    }

    TypeMap[T] = Data;
    return Data;
  }

  Type* lowerTypeImpl(Type* T) {
    assert(T != LowWidePtrTy);
    
    if (FunctionType* FT = dyn_cast<FunctionType>(T))
      return DeludedFuncTy;

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
      std::string NewName = ("deluded_" + ST->getName()).str();
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
    if (T->isSized())
      assert(DL.getTypeStoreSizeBeforeDeluge(T) == DL.getTypeStoreSize(LowT));
    LoweredTypes[T] = LowT;
    return LowT;
  }

  void checkInt(Value *P, unsigned Size, Instruction *InsertBefore) {
    CallInst::Create(
      CheckAccessInt,
      { P, ConstantInt::get(IntPtrTy, Size), getOrigin(InsertBefore->getDebugLoc()) },
      "", InsertBefore)
      ->setDebugLoc(InsertBefore->getDebugLoc());
  }

  void checkPtr(Value *P, Instruction *InsertBefore) {
    if (verbose)
      errs() << "Inserting call to " << *CheckAccessPtr.getFunctionType() << "\n";
    CallInst::Create(
      CheckAccessPtr, { P, getOrigin(InsertBefore->getDebugLoc()) }, "", InsertBefore)
      ->setDebugLoc(InsertBefore->getDebugLoc());
  }

  Value* sidecarPtr(Value* P, Instruction* InsertBefore) {
    Instruction* SidecarPtr = GetElementPtrInst::Create(
      LowWidePtrTy, P, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 0) },
      "deluge_sidecar_ptr", InsertBefore);
    SidecarPtr->setDebugLoc(InsertBefore->getDebugLoc());
    return SidecarPtr;
  }

  Value* capabilityPtr(Value* P, Instruction* InsertBefore) {
    Instruction* CapabilityPtr = GetElementPtrInst::Create(
      LowWidePtrTy, P, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) },
      "deluge_capability_ptr", InsertBefore);
    CapabilityPtr->setDebugLoc(InsertBefore->getDebugLoc());
    return CapabilityPtr;
  }

  Value* loadPtrPart(
    Value* P, bool isVolatile, Align A, AtomicOrdering AO, Instruction* InsertBefore) {
    // FIXME: I probably only need Unordered, not Monotonic.
    Instruction* Result = new LoadInst(
      Int128Ty, P, "deluge_load_ptr_part", isVolatile,
      std::max(A, Align(WordSize)), getMergedAtomicOrdering(AtomicOrdering::Monotonic, AO),
      SyncScope::System, InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  Value* loadPtrPart(Value* P, Instruction* InsertBefore) {
    return loadPtrPart(P, false, Align(WordSize), AtomicOrdering::Monotonic, InsertBefore);
  }

  Value* loadPtr(Value* P, bool isVolatile, Align A, AtomicOrdering AO, Instruction* InsertBefore) {
    return createPtr(
      loadPtrPart(sidecarPtr(P, InsertBefore), isVolatile, A, AO, InsertBefore),
      loadPtrPart(capabilityPtr(P, InsertBefore), isVolatile, A, AO, InsertBefore),
      InsertBefore);
  }

  Value* loadPtr(Value* P, Instruction* InsertBefore) {
    return loadPtr(P, false, Align(WordSize), AtomicOrdering::Monotonic, InsertBefore);
  }

  Value* extractSidecar(Value* V, Instruction* InsertBefore) {
    Instruction* Sidecar = ExtractValueInst::Create(Int128Ty, V, { 0 }, "deluge_sidecar", InsertBefore);
    Sidecar->setDebugLoc(InsertBefore->getDebugLoc());
    return Sidecar;
  }

  Value* extractCapability(Value* V, Instruction* InsertBefore) {
    Instruction* Capability = ExtractValueInst::Create(Int128Ty, V, { 1 }, "deluge_sidecar", InsertBefore);
    Capability->setDebugLoc(InsertBefore->getDebugLoc());
    return Capability;
  }

  void storePtrPart(
    Value* V, Value* P, bool isVolatile, Align A, AtomicOrdering AO, Instruction* InsertBefore) {
    (new StoreInst(
      V, P, isVolatile, std::max(A, Align(WordSize)),
      getMergedAtomicOrdering(AtomicOrdering::Monotonic, AO), SyncScope::System, InsertBefore))
      ->setDebugLoc(InsertBefore->getDebugLoc());
  }

  void storePtrPart(Value* V, Value* P, Instruction* InsertBefore) {
    storePtrPart(V, P, false, Align(WordSize), AtomicOrdering::Monotonic, InsertBefore);
  }

  // V = pointer value to store, P = pointer to store to.
  void storePtr(Value* V, Value* P,
                bool isVolatile, Align A, AtomicOrdering AO, Instruction* InsertBefore) {
    storePtrPart(extractSidecar(V, InsertBefore), sidecarPtr(P, InsertBefore),
                 isVolatile, A, AO, InsertBefore);
    storePtrPart(extractCapability(V, InsertBefore), capabilityPtr(P, InsertBefore),
                 isVolatile, A, AO, InsertBefore);
  }

  void storePtr(Value* V, Value* P, Instruction* InsertBefore) {
    storePtr(V, P, false, Align(WordSize), AtomicOrdering::Monotonic, InsertBefore);
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

  void checkRecurse(Type *LowT, Value* HighP, Value *P, Instruction *InsertBefore) {
    if (!hasPtrsForCheck(LowT)) {
      checkInt(reforgePtr(HighP, P, InsertBefore), DL.getTypeStoreSize(LowT), InsertBefore);
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
      checkPtr(reforgePtr(HighP, P, InsertBefore), InsertBefore);
      return;
    }

    assert(!isa<PointerType>(LowT));

    if (StructType* ST = dyn_cast<StructType>(LowT)) {
      for (unsigned Index = ST->getNumElements(); Index--;) {
        Type* InnerT = ST->getElementType(Index);
        Value *InnerP = GetElementPtrInst::Create(
          ST, P, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_struct", InsertBefore);
        checkRecurse(InnerT, HighP, InnerP, InsertBefore);
      }
      return;
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(LowT)) {
      for (uint64_t Index = AT->getNumElements(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          AT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_array", InsertBefore);
        checkRecurse(AT->getElementType(), HighP, InnerP, InsertBefore);
      }
      return;
    }
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(LowT)) {
      for (unsigned Index = VT->getElementCount().getFixedValue(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          VT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_vector", InsertBefore);
        checkRecurse(VT->getElementType(), HighP, InnerP, InsertBefore);
      }
      return;
    }

    if (ScalableVectorType* VT = dyn_cast<ScalableVectorType>(LowT)) {
      llvm_unreachable("Shouldn't see scalable vector types in checkRecurse");
      return;
    }

    llvm_unreachable("Should not get here.");
  }

  Value* lowerPtr(Value *HighP, Instruction* InsertBefore) {
    Instruction* Result = CallInst::Create(PtrPtr, { HighP }, "deluge_getlowptr", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  // Insert whatever checks are needed to perform the access and then return the lowered pointer to
  // access.
  Value* prepareForAccess(Type *LowT, Value *HighP, Instruction *InsertBefore) {
    Value* LowP = lowerPtr(HighP, InsertBefore);
    checkRecurse(LowT, HighP, LowP, InsertBefore);
    return LowP;
  }

  Value* loadValueRecurse(Type* LowT, Value* HighP, Value* P,
                          bool isVolatile, Align A, AtomicOrdering AO, SyncScope::ID SS,
                          Instruction* InsertBefore) {
    A = std::min(DL.getABITypeAlign(LowT), A);
    
    if (!hasPtrsForCheck(LowT)) {
      checkInt(reforgePtr(HighP, P, InsertBefore), DL.getTypeStoreSize(LowT), InsertBefore);
      return new LoadInst(LowT, P, "deluge_load", isVolatile, A, AO, SS, InsertBefore);
    }
    
    if (FunctionType* FT = dyn_cast<FunctionType>(LowT)) {
      llvm_unreachable("shouldn't see function types in checkRecurse");
      return nullptr;
    }

    if (isa<TypedPointerType>(LowT)) {
      llvm_unreachable("Shouldn't ever see typed pointers");
      return nullptr;
    }

    assert(LowT != LowRawPtrTy);

    if (LowT == LowWidePtrTy) {
      checkPtr(reforgePtr(HighP, P, InsertBefore), InsertBefore);
      return loadPtr(P, isVolatile, A, AO, InsertBefore);
    }

    assert(!isa<PointerType>(LowT));

    if (StructType* ST = dyn_cast<StructType>(LowT)) {
      Value* Result = UndefValue::get(ST);
      for (unsigned Index = ST->getNumElements(); Index--;) {
        Type* InnerT = ST->getElementType(Index);
        Value *InnerP = GetElementPtrInst::Create(
          ST, P, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_struct", InsertBefore);
        Value* V = loadValueRecurse(InnerT, HighP, InnerP, isVolatile, A, AO, SS, InsertBefore);
        Result = InsertValueInst::Create(Result, V, Index, "deluge_insert_struct", InsertBefore);
      }
      return Result;
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(LowT)) {
      Value* Result = UndefValue::get(AT);
      assert(static_cast<unsigned>(AT->getNumElements()) == AT->getNumElements());
      for (unsigned Index = static_cast<unsigned>(AT->getNumElements()); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          AT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_array", InsertBefore);
        Value* V = loadValueRecurse(
          AT->getElementType(), HighP, InnerP, isVolatile, A, AO, SS, InsertBefore);
        Result = InsertValueInst::Create(Result, V, Index, "deluge_insert_array", InsertBefore);
      }
      return Result;
    }
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(LowT)) {
      Value* Result = UndefValue::get(VT);
      for (unsigned Index = VT->getElementCount().getFixedValue(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          VT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_vector", InsertBefore);
        Value* V = loadValueRecurse(
          VT->getElementType(), HighP, InnerP, isVolatile, A, AO, SS, InsertBefore);
        Result = InsertElementInst::Create(
          Result, V, ConstantInt::get(IntPtrTy, Index), "deluge_insert_vector", InsertBefore);
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

  Value* loadValueUncheckedRecurse(Type* LowT, Value* P,
                                   bool isVolatile, Align A, AtomicOrdering AO, SyncScope::ID SS,
                                   Instruction* InsertBefore) {
    A = std::min(DL.getABITypeAlign(LowT), A);
    
    if (!hasPtrsForCheck(LowT))
      return new LoadInst(LowT, P, "deluge_load", isVolatile, A, AO, SS, InsertBefore);
    
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
          ST, P, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_struct", InsertBefore);
        Value* V = loadValueUncheckedRecurse(InnerT, InnerP, isVolatile, A, AO, SS, InsertBefore);
        Result = InsertValueInst::Create(Result, V, Index, "deluge_insert_struct", InsertBefore);
      }
      return Result;
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(LowT)) {
      Value* Result = UndefValue::get(AT);
      assert(static_cast<unsigned>(AT->getNumElements()) == AT->getNumElements());
      for (unsigned Index = static_cast<unsigned>(AT->getNumElements()); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          AT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_array", InsertBefore);
        Value* V = loadValueUncheckedRecurse(
          AT->getElementType(), InnerP, isVolatile, A, AO, SS, InsertBefore);
        Result = InsertValueInst::Create(Result, V, Index, "deluge_insert_array", InsertBefore);
      }
      return Result;
    }
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(LowT)) {
      Value* Result = UndefValue::get(VT);
      for (unsigned Index = VT->getElementCount().getFixedValue(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          VT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_vector", InsertBefore);
        Value* V = loadValueUncheckedRecurse(
          VT->getElementType(), InnerP, isVolatile, A, AO, SS, InsertBefore);
        Result = InsertElementInst::Create(
          Result, V, ConstantInt::get(IntPtrTy, Index), "deluge_insert_vector", InsertBefore);
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

  void storeValueRecurse(Type* LowT, Value* HighP, Value* V, Value* P,
                         bool isVolatile, Align A, AtomicOrdering AO, SyncScope::ID SS,
                         Instruction* InsertBefore) {
    A = std::min(DL.getABITypeAlign(LowT), A);
    
    if (!hasPtrsForCheck(LowT)) {
      checkInt(reforgePtr(HighP, P, InsertBefore), DL.getTypeStoreSize(LowT), InsertBefore);
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
      checkPtr(reforgePtr(HighP, P, InsertBefore), InsertBefore);
      storePtr(V, P, isVolatile, A, AO, InsertBefore);
      return;
    }

    assert(!isa<PointerType>(LowT));

    if (StructType* ST = dyn_cast<StructType>(LowT)) {
      for (unsigned Index = ST->getNumElements(); Index--;) {
        Type* InnerT = ST->getElementType(Index);
        Value *InnerP = GetElementPtrInst::Create(
          ST, P, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_struct", InsertBefore);
        Value* InnerV = ExtractValueInst::Create(
          InnerT, V, { Index }, "deluge_extract_struct", InsertBefore);
        storeValueRecurse(InnerT, HighP, InnerV, InnerP, isVolatile, A, AO, SS, InsertBefore);
      }
      return;
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(LowT)) {
      assert(static_cast<unsigned>(AT->getNumElements()) == AT->getNumElements());
      for (unsigned Index = static_cast<unsigned>(AT->getNumElements()); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          AT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_array", InsertBefore);
        Value* InnerV = ExtractValueInst::Create(
          AT->getElementType(), V, { Index }, "deluge_extract_array", InsertBefore);
        storeValueRecurse(AT->getElementType(), HighP, InnerV, InnerP, isVolatile, A, AO, SS, InsertBefore);
      }
      return;
    }
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(LowT)) {
      for (unsigned Index = VT->getElementCount().getFixedValue(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          VT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_vector", InsertBefore);
        Value* InnerV = ExtractElementInst::Create(
          V, ConstantInt::get(IntPtrTy, Index), "deluge_extract_vector", InsertBefore);
        storeValueRecurse(VT->getElementType(), HighP, InnerV, InnerP, isVolatile, A, AO, SS, InsertBefore);
      }
      return;
    }

    if (ScalableVectorType* VT = dyn_cast<ScalableVectorType>(LowT)) {
      llvm_unreachable("Shouldn't see scalable vector types in checkRecurse");
      return;
    }

    llvm_unreachable("Should not get here.");
  }

  void storeValueUncheckedRecurse(Type* LowT, Value* V, Value* P,
                                  bool isVolatile, Align A, AtomicOrdering AO, SyncScope::ID SS,
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
      storePtr(V, P, isVolatile, A, AO, InsertBefore);
      return;
    }

    assert(!isa<PointerType>(LowT));

    if (StructType* ST = dyn_cast<StructType>(LowT)) {
      for (unsigned Index = ST->getNumElements(); Index--;) {
        Type* InnerT = ST->getElementType(Index);
        Value *InnerP = GetElementPtrInst::Create(
          ST, P, { ConstantInt::get(Int32Ty, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_struct", InsertBefore);
        Value* InnerV = ExtractValueInst::Create(
          InnerT, V, { Index }, "deluge_extract_struct", InsertBefore);
        storeValueUncheckedRecurse(InnerT, InnerV, InnerP, isVolatile, A, AO, SS, InsertBefore);
      }
      return;
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(LowT)) {
      assert(static_cast<unsigned>(AT->getNumElements()) == AT->getNumElements());
      for (unsigned Index = static_cast<unsigned>(AT->getNumElements()); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          AT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_array", InsertBefore);
        Value* InnerV = ExtractValueInst::Create(
          AT->getElementType(), V, { Index }, "deluge_extract_array", InsertBefore);
        storeValueUncheckedRecurse(
          AT->getElementType(), InnerV, InnerP, isVolatile, A, AO, SS, InsertBefore);
      }
      return;
    }
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(LowT)) {
      for (unsigned Index = VT->getElementCount().getFixedValue(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(
          VT, P, { ConstantInt::get(IntPtrTy, 0), ConstantInt::get(IntPtrTy, Index) },
          "deluge_InnerP_vector", InsertBefore);
        Value* InnerV = ExtractElementInst::Create(
          V, ConstantInt::get(IntPtrTy, Index), "deluge_extract_vector", InsertBefore);
        storeValueUncheckedRecurse(
          VT->getElementType(), InnerV, InnerP, isVolatile, A, AO, SS, InsertBefore);
      }
      return;
    }

    if (ScalableVectorType* VT = dyn_cast<ScalableVectorType>(LowT)) {
      llvm_unreachable("Shouldn't see scalable vector types in checkRecurse");
      return;
    }

    llvm_unreachable("Should not get here.");
    return;
  }

  Value* createPtr(Value* Sidecar, Value* Capability, Instruction* InsertBefore) {
    Instruction* Result = InsertValueInst::Create(
      UndefValue::get(LowWidePtrTy), Sidecar, { 0 }, "deluge_new_capability_sidecar", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    Result = InsertValueInst::Create(
      Result, Capability, { 1 }, "deluge_new_capability_capability", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  Value* forgePtrWithTypeRepAndSize(
    Value* Ptr, Value* Size, Value* TypeRep, Instruction* InsertionPoint) {
    Instruction* Sidecar = CallInst::Create(
      NewSidecar, { Ptr, Size, TypeRep }, "deluge_new_sidecar", InsertionPoint);
    Sidecar->setDebugLoc(InsertionPoint->getDebugLoc());
    Instruction* Capability = CallInst::Create(
      NewCapability, { Ptr, Size, TypeRep }, "deluge_new_capability", InsertionPoint);
    Capability->setDebugLoc(InsertionPoint->getDebugLoc());
    return createPtr(Sidecar, Capability, InsertionPoint);
  }

  Value* forgePtrWithTypeRepAndCount(
    Value* Ptr, Value* TypeRep, Type* LowT, Value* Count, Instruction* InsertionPoint) {
    Instruction* Size = BinaryOperator::Create(
      Instruction::Mul, Count, ConstantInt::get(IntPtrTy, DL.getTypeStoreSize(LowT)),
      "deluge_forge_size", InsertionPoint);
    Size->setDebugLoc(InsertionPoint->getDebugLoc());
    return forgePtrWithTypeRepAndSize(Ptr, Size, TypeRep, InsertionPoint);
  }

  Value* forgePtrWithTypeRepAndUpper(
    Value* Ptr, Value* Upper, Value* TypeRep, Instruction* InsertionPoint) {
    Instruction* PtrAsInt = new PtrToIntInst(Ptr, IntPtrTy, "deluge_ptr_as_int", InsertionPoint);
    PtrAsInt->setDebugLoc(InsertionPoint->getDebugLoc());
    Instruction* UpperAsInt = new PtrToIntInst(Upper, IntPtrTy, "deluge_upper_as_int", InsertionPoint);
    UpperAsInt->setDebugLoc(InsertionPoint->getDebugLoc());
    Instruction* Size = BinaryOperator::Create(
      Instruction::Sub, UpperAsInt, PtrAsInt, "deluge_forge_size", InsertionPoint);
    Size->setDebugLoc(InsertionPoint->getDebugLoc());
    return forgePtrWithTypeRepAndSize(Ptr, Size, TypeRep, InsertionPoint);
  }

  Value* forgePtrWithTypeRep(Value* Ptr, Value* TypeRep, Type* LowT, Instruction* InsertionPoint) {
    size_t Size;
    if (isa<FunctionType>(LowT))
      Size = 1;
    else
      Size = DL.getTypeStoreSize(LowT);
    return forgePtrWithTypeRepAndSize(Ptr, ConstantInt::get(IntPtrTy, Size), TypeRep, InsertionPoint);
  }

  // Why don't we have forgePtrWithTypeDataAndCount, which would allow you to "just" get the size
  // from the DelugeType? Because! That would be wrong!
  //
  // DelugeType is a representation of a repeatable pattern of memory. LLVM types represent C types.
  // So a C struct with a repeating pattern might be represented by a DelugeType of smaller size.

  Value* forgePtrWithLowTypeAndSize(Value* Ptr, Value* Size, Type* LowT, Instruction* InsertionPoint) {
    return forgePtrWithTypeRepAndSize(
      Ptr, Size, getTypeRep(dataForLowType(LowT), InsertionPoint), InsertionPoint);
  }

  Value* forgePtrWithLowTypeAndCount(
    Value* Ptr, Type* LowT, Value* Count, Instruction* InsertionPoint) {
    return forgePtrWithTypeRepAndCount(
      Ptr, getTypeRep(dataForLowType(LowT), InsertionPoint), LowT, Count, InsertionPoint);
  }

  Value* forgePtrWithLowTypeAndUpper(
    Value* Ptr, Value* Upper, Type* LowT, Instruction* InsertionPoint) {
    return forgePtrWithTypeRepAndUpper(
      Ptr, Upper, getTypeRep(dataForLowType(LowT), InsertionPoint), InsertionPoint);
  }

  Value* forgePtrWithLowType(Value* Ptr, Type* LowT, Instruction* InsertionPoint) {
    return forgePtrWithTypeRep(
      Ptr, getTypeRep(dataForLowType(LowT), InsertionPoint), LowT, InsertionPoint);
  }

  Value* reforgePtr(Value* LowWidePtr, Value* NewLowRawPtr, Instruction* InsertionPoint) {
    Instruction* Sidecar = CallInst::Create(
      UpdateSidecar, { LowWidePtr, NewLowRawPtr }, "deluge_update_sidecar", InsertionPoint);
    Sidecar->setDebugLoc(InsertionPoint->getDebugLoc());
    Instruction* Capability = CallInst::Create(
      UpdateCapability, { LowWidePtr, NewLowRawPtr }, "deluge_update_capability", InsertionPoint);
    Capability->setDebugLoc(InsertionPoint->getDebugLoc());
    return createPtr(Sidecar, Capability, InsertionPoint);
  }

  Value* forgeBadPtr(Value* Ptr, Instruction* InsertionPoint) {
    return reforgePtr(LowWideNull, Ptr, InsertionPoint);
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

  bool computeConstantRelocations(Constant* C, std::vector<ConstantRelocation>& Result, size_t Offset = 0) {
    assert(C->getType() != LowWidePtrTy);
    
    if (isa<UndefValue>(C))
      return true;
    
    if (isa<ConstantPointerNull>(C))
      return true;

    if (isa<ConstantAggregateZero>(C))
      return true;

    if (GlobalValue* G = dyn_cast<GlobalValue>(C)) {
      assert(!shouldPassThrough(G));
      assert(!Getters.count(G));
      if (GlobalToGetter.count(G)) {
        Function* Getter = GlobalToGetter[G];
        Result.push_back(ConstantRelocation(Offset, ConstantKind::Global, Getter));
        return true;
      }
      Function* F = dyn_cast<Function>(G);
      if (!F) 
        F = cast<Function>(cast<GlobalAlias>(G)->getAliasee());
      Result.push_back(ConstantRelocation(Offset, ConstantKind::Function, F));
      return true;
    }

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
      const StructLayout* SL = DL.getStructLayout(cast<StructType>(CS->getType()));
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

    // FIXME: Eventually add the ability to handle ConstantExpr relocations. It's not that hard. If
    // we handle those then we won't need this function to return bool. But, it might be prudent to
    // only handle a subset of them, depending on what real C code does.
    assert(isa<ConstantExpr>(C));
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
      Type* LowT = GlobalLowTypes[G];
      assert(!GlobalToGetter.count(nullptr));
      assert(!Getters.count(nullptr));
      assert(!Getters.count(G));
      if (GlobalToGetter.count(G)) {
        Function* Getter = GlobalToGetter[G];
        assert(Getter);
        Instruction* Result = CallInst::Create(
          GlobalGetterTy, Getter, { InitializationContext }, "deluge_call_getter", InsertBefore);
        Result->setDebugLoc(InsertBefore->getDebugLoc());
        return createPtr(ConstantInt::get(Int128Ty, 0), Result, InsertBefore);
      }
      assert(isa<Function>(G) ||
             (isa<GlobalAlias>(G) && isa<Function>(cast<GlobalAlias>(G)->getAliasee())));
      return forgePtrWithLowType(G, LowT, InsertBefore);
    }

    if (isa<ConstantData>(C))
      return C;

    if (ConstantArray* CA = dyn_cast<ConstantArray>(C)) {
      Value* Result = UndefValue::get(lowerType(CA->getType()));
      for (size_t Index = 0; Index < CA->getNumOperands(); ++Index) {
        Instruction* Insert = InsertValueInst::Create(
          Result, lowerConstant(CA->getOperand(Index), InsertBefore, InitializationContext),
          static_cast<unsigned>(Index), "deluge_insert_array", InsertBefore);
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
          Result, LowC, static_cast<unsigned>(Index), "deluge_insert_struct", InsertBefore);
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
          ConstantInt::get(IntPtrTy, Index), "deluge_insert_vector", InsertBefore);
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
      CastInst::CreateIntegerCast(V, T, false, "deluge_makeintptr", InsertionPoint);
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

  Value* getFromConstantPool(ConstantPoolEntry GTE, Instruction* InsertBefore) {
    auto iter = ConstantPoolEntryIndex.find(GTE);
    size_t Index;
    if (iter != ConstantPoolEntryIndex.end())
      Index = iter->second;
    else {
      Index = ConstantPoolEntries.size();
      ConstantPoolEntryIndex[GTE] = Index;
      ConstantPoolEntries.push_back(GTE);
    }
    assert(LocalConstantPoolPtr);
    Instruction* EntryPtr = GetElementPtrInst::Create(
      LowRawPtrTy, LocalConstantPoolPtr, { ConstantInt::get(IntPtrTy, Index) },
      "deluge_constantpool_entry_ptr", InsertBefore);
    assert(EntryPtr->getType()->isPointerTy());
    EntryPtr->setDebugLoc(InsertBefore->getDebugLoc());
    Instruction* Result = new LoadInst(
      LowRawPtrTy, EntryPtr, "deluge_constantpool_load", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  Value* getTypeRepEasy(DelugeTypeData* DTD) {
    if (DTD == &Int)
      return IntTypeRep;
    if (DTD == &FunctionDTD)
      return FunctionTypeRep;
    if (DTD == &TypeDTD)
      return TypeTypeRep;
    if (DTD == &Invalid)
      return LowRawNull;
    return nullptr;
  }

  Value* getTypeRep(DelugeTypeData* DTD, Instruction* InsertBefore) {
    if (Value* TypeRep = getTypeRepEasy(DTD))
      return TypeRep;
    ConstantPoolEntry GTE;
    GTE.Kind = ConstantPoolEntryKind::Type;
    GTE.DTD = DTD;
    return getFromConstantPool(GTE, InsertBefore);
  }

  Value* getTypeRepWithoutConstantPool(DelugeTypeData* DTD, Instruction* InsertBefore) {
    if (Value* TypeRep = getTypeRepEasy(DTD))
      return TypeRep;
    assert(DTD->TemplateRep);
    Instruction* Result = CallInst::Create(GetType, { DTD->TemplateRep }, "deluge_get_type", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  Value* getHeap(DelugeTypeData* DTD, Instruction* InsertBefore, ConstantPoolEntryKind Kind) {
    assert(Kind == ConstantPoolEntryKind::Heap || Kind == ConstantPoolEntryKind::HardHeap);
    ConstantPoolEntry GTE;
    GTE.Kind = Kind;
    GTE.DTD = DTD;
    return getFromConstantPool(GTE, InsertBefore);
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
      case Intrinsic::memcpy:
      case Intrinsic::memcpy_inline:
        lowerConstantOperand(II->getArgOperandUse(0), I, LowRawNull);
        lowerConstantOperand(II->getArgOperandUse(1), I, LowRawNull);
        lowerConstantOperand(II->getArgOperandUse(2), I, LowRawNull);
        // OK, so it seems bad that we're treating memcpy and memcpy_inline the same. But it's fine.
        // The intent of the inline variant is to cover the low-level programming case where you cannot
        // call outside libraries, but you want to describe a memcpy. However, with Deluge, we're always
        // depending on the Deluge runtime somehow, so it's OK to call into the Deluge runtime's memcpy
        // (or memset, or memmove).
        //
        // Also, for now, we just ignore the volatile bit, since the call to the Deluge runtime is going
        // to look volatile enough.
        if (hasPtrsForCheck(II->getArgOperand(0)->getType())) {
          assert(hasPtrsForCheck(II->getArgOperand(1)->getType()));
          Instruction* CI = CallInst::Create(
            Memcpy,
            { II->getArgOperand(0), II->getArgOperand(1), makeIntPtr(II->getArgOperand(2), II),
              getOrigin(II->getDebugLoc()) });
          ReplaceInstWithInst(II, CI);
        }
        return true;
      case Intrinsic::memset:
      case Intrinsic::memset_inline:
        lowerConstantOperand(II->getArgOperandUse(0), I, LowRawNull);
        lowerConstantOperand(II->getArgOperandUse(1), I, LowRawNull);
        lowerConstantOperand(II->getArgOperandUse(2), I, LowRawNull);
        if (hasPtrsForCheck(II->getArgOperand(0)->getType())) {
          Instruction* CI = CallInst::Create(
            Memset,
            { II->getArgOperand(0), castInt(II->getArgOperand(1), Int32Ty, II),
              makeIntPtr(II->getArgOperand(2), II), getOrigin(II->getDebugLoc()) });
          ReplaceInstWithInst(II, CI);
        }
        return true;
      case Intrinsic::memmove:
        lowerConstantOperand(II->getArgOperandUse(0), I, LowRawNull);
        lowerConstantOperand(II->getArgOperandUse(1), I, LowRawNull);
        lowerConstantOperand(II->getArgOperandUse(2), I, LowRawNull);
        if (hasPtrsForCheck(II->getArgOperand(0)->getType())) {
          assert(hasPtrsForCheck(II->getArgOperand(1)->getType()));
          Instruction* CI = CallInst::Create(
            Memmove,
            { II->getArgOperand(0), II->getArgOperand(1), makeIntPtr(II->getArgOperand(2), II),
              getOrigin(II->getDebugLoc()) });
          ReplaceInstWithInst(II, CI);
        }
        return true;

      case Intrinsic::lifetime_start:
      case Intrinsic::lifetime_end:
        // FIXME: We should use these to do more compact allocation of frames. And, if we decide that some
        // allocas don't need to go into the frame, then those can just keep their existing lifetime
        // annotations. Moreover, choosing which allocas don't escape will require analyzing lifetime
        // annotations (since using an alloca before lifetime start or after lifetime end is an escape if
        // we keep the lifetime annotations' meaning).
        II->eraseFromParent();
        return true;

      case Intrinsic::vastart:
        lowerConstantOperand(II->getArgOperandUse(0), I, LowRawNull);
        checkPtr(II->getArgOperand(0), II);
        storePtr(ArgBufferPtr, lowerPtr(II->getArgOperand(0), II), II);
        II->eraseFromParent();
        return true;
        
      case Intrinsic::vacopy: {
        lowerConstantOperand(II->getArgOperandUse(0), I, LowRawNull);
        lowerConstantOperand(II->getArgOperandUse(1), I, LowRawNull);
        checkPtr(II->getArgOperand(0), II);
        checkPtr(II->getArgOperand(1), II);
        Value* Load = loadPtr(lowerPtr(II->getArgOperand(1), II), II);
        storePtr(Load, lowerPtr(II->getArgOperand(0), II), II);
        II->eraseFromParent();
        return true;
      }
        
      case Intrinsic::vaend:
        II->eraseFromParent();
        return true;

      case Intrinsic::stacksave: {
        Instruction* SizePtr = GetElementPtrInst::Create(
          AllocaStackTy, FutureAllocaStack,
          { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) },
          "deluge_alloca_stack_size_ptr", II);
        SizePtr->setDebugLoc(II->getDebugLoc());
        Instruction* Load = new LoadInst(IntPtrTy, SizePtr, "deluge_alloca_stack_size", II);
        Load->setDebugLoc(II->getDebugLoc());
        Instruction* Cast = new IntToPtrInst(Load, LowRawPtrTy, "deluge_alloca_stack_size_to_ptr", II);
        Cast->setDebugLoc(II->getDebugLoc());
        II->replaceAllUsesWith(forgeBadPtr(Cast, II));
        II->eraseFromParent();
        return true;
      }

      case Intrinsic::stackrestore: {
        lowerConstantOperand(II->getArgOperandUse(0), I, LowRawNull);
        Instruction* Cast = new PtrToIntInst(
          lowerPtr(II->getArgOperand(0), II), IntPtrTy, "deluge_alloca_stack_size", II);
        Cast->setDebugLoc(II->getDebugLoc());
        CallInst::Create(AllocaStackRestore, { FutureAllocaStack, Cast }, "", II)
          ->setDebugLoc(II->getDebugLoc());
        II->eraseFromParent();
        return true;
      }

      default:
        if (!II->getCalledFunction()->doesNotAccessMemory()) {
          if (verbose)
            llvm::errs() << "Unhandled intrinsic: " << *II << "\n";
          CallInst::Create(Error, { getOrigin(I->getDebugLoc()) }, "", II)
            ->setDebugLoc(II->getDebugLoc());
        }
        for (Use& U : II->data_ops()) {
          lowerConstantOperand(U, I, LowRawNull);
          if (hasPtrsForCheck(U->getType()))
            U = lowerPtr(U, II);
        }
        if (hasPtrsForCheck(II->getType()))
          hackRAUW(II, [&] () { return forgeBadPtr(II, II->getNextNode()); });
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
      
      if (CI->getCalledOperand() == ZunsafeForgeImpl) {
        if (verbose)
          errs() << "Lowering unsafe forge\n";
        lowerConstantOperand(CI->getArgOperandUse(0), CI, LowRawNull);
        lowerConstantOperand(CI->getArgOperandUse(2), CI, LowRawNull);
        Type* HighT = cast<AllocaInst>(CI->getArgOperand(1))->getAllocatedType();
        Type* LowT = lowerType(HighT);
        Value* LowPtr = lowerPtr(CI->getArgOperand(0), CI);
        Value* TypeRep = getTypeRep(dataForLowType(LowT), CI);
        Instruction* Check = CallInst::Create(
          CheckForge,
          { LowPtr, ConstantInt::get(IntPtrTy, DL.getTypeStoreSize(LowT)), CI->getArgOperand(2), TypeRep,
            getOrigin(CI->getDebugLoc()) },
          "", CI);
        Check->setDebugLoc(CI->getDebugLoc());
        CI->replaceAllUsesWith(
          forgePtrWithTypeRepAndCount(LowPtr, TypeRep, LowT, CI->getArgOperand(2), CI));
        CI->eraseFromParent();
        return true;
      }

      if (CI->getCalledOperand() == ZrestrictImpl) {
        if (verbose)
          errs() << "Lowering restrict\n";
        lowerConstantOperand(CI->getArgOperandUse(0), CI, LowRawNull);
        lowerConstantOperand(CI->getArgOperandUse(2), CI, LowRawNull);
        Type* HighT = cast<AllocaInst>(CI->getArgOperand(1))->getAllocatedType();
        Type* LowT = lowerType(HighT);
        Value* TypeRep = getTypeRep(dataForLowType(LowT), CI);
        Value* LowPtr = lowerPtr(CI->getOperand(0), CI);
        Instruction* NewUpper = GetElementPtrInst::Create(
          LowT, LowPtr, { ConstantInt::get(IntPtrTy, 1) }, "deluge_NewUpper", CI);
        NewUpper->setDebugLoc(CI->getDebugLoc());
        CallInst::Create(
          CheckRestrict, { CI->getOperand(0), NewUpper, TypeRep, getOrigin(CI->getDebugLoc()) }, "", CI)
          ->setDebugLoc(CI->getDebugLoc());
        CI->replaceAllUsesWith(forgePtrWithTypeRepAndUpper(LowPtr, NewUpper, TypeRep, CI));
        CI->eraseFromParent();
        return true;
      }

      bool isHard =
        CI->getCalledOperand() == ZhardAllocImpl ||
        CI->getCalledOperand() == ZhardAllocFlexImpl ||
        CI->getCalledOperand() == ZhardAlignedAllocImpl ||
        CI->getCalledOperand() == ZhardReallocImpl;

      if (CI->getCalledOperand() == ZallocImpl || CI->getCalledOperand() == ZhardAllocImpl) {
        if (verbose)
          errs() << "Lowering alloc\n";
        lowerConstantOperand(CI->getArgOperandUse(1), CI, LowRawNull);
        Type* HighT = cast<AllocaInst>(CI->getArgOperand(0))->getAllocatedType();
        Type* LowT = lowerType(HighT);
        assert(hasPtrsForCheck(HighT) == hasPtrsForCheck(LowT));
        Instruction* Alloc = nullptr;
        
        DelugeTypeData *DTD = dataForLowType(LowT);
        assert(DTD->Type.Main.Size);
        if (!hasPtrsForCheck(HighT)) {
          assert(DTD == &Int);
          size_t Alignment = DL.getABITypeAlign(LowT).value();
          size_t Size = DL.getTypeStoreSize(LowT);
          if (Alignment > MinAlign) {
            Alloc = CallInst::Create(
              isHard ? TryHardAllocateIntWithAlignment : TryAllocateIntWithAlignment,
              { CI->getArgOperand(1), ConstantInt::get(IntPtrTy, Size),
                ConstantInt::get(IntPtrTy, Alignment) },
              "deluge_alloc_int", CI);
          } else {
            Alloc = CallInst::Create(
              isHard ? TryHardAllocateInt : TryAllocateInt,
              { CI->getArgOperand(1), ConstantInt::get(IntPtrTy, Size) },
              "deluge_alloc_int", CI);
          }
        } else {
          Value* Heap = getHeap(
            DTD, CI, isHard ? ConstantPoolEntryKind::HardHeap : ConstantPoolEntryKind::Heap);
          if (Constant* C = dyn_cast<Constant>(CI->getArgOperand(1))) {
            if (C->isOneValue()) {
              Alloc = CallInst::Create(
                isHard ? TryHardAllocateOne : TryAllocateOne,
                { Heap }, "deluge_alloc_one", CI);
            }
          }
          if (!Alloc) {
            Alloc = CallInst::Create(
              isHard ? TryHardAllocateMany : TryAllocateMany,
              { Heap, CI->getArgOperand(1) }, "deluge_alloc_many", CI);
          }
        }
        
        Alloc->setDebugLoc(CI->getDebugLoc());
        CI->replaceAllUsesWith(
          forgePtrWithTypeRepAndCount(Alloc, getTypeRep(DTD, CI), LowT, CI->getArgOperand(1), CI));
        CI->eraseFromParent();
        return true;
      }

      if (CI->getCalledOperand() == ZalignedAllocImpl || CI->getCalledOperand() == ZhardAlignedAllocImpl) {
        if (verbose)
          errs() << "Lowering alloc\n";
        lowerConstantOperand(CI->getArgOperandUse(1), CI, LowRawNull);
        lowerConstantOperand(CI->getArgOperandUse(2), CI, LowRawNull);
        Type* HighT = cast<AllocaInst>(CI->getArgOperand(0))->getAllocatedType();
        Type* LowT = lowerType(HighT);
        assert(hasPtrsForCheck(HighT) == hasPtrsForCheck(LowT));
        Instruction* Alloc;
        
        DelugeTypeData *DTD = dataForLowType(LowT);
        assert(DTD->Type.Main.Size);
        Value* Count = CI->getArgOperand(2);
        Value* PassedAlignment = CI->getArgOperand(1);
        if (!hasPtrsForCheck(HighT)) {
          assert(DTD == &Int);
          size_t Alignment = DL.getABITypeAlign(LowT).value();
          size_t Size = DL.getTypeStoreSize(LowT);
          Value* TypeAlignment = ConstantInt::get(IntPtrTy, Alignment);
          Instruction* Compare = new ICmpInst(
            CI, ICmpInst::ICMP_ULT, PassedAlignment, TypeAlignment, "deluge_aligned_alloc_compare");
          Compare->setDebugLoc(CI->getDebugLoc());
          Instruction* AlignmentValue = SelectInst::Create(
            Compare, TypeAlignment, PassedAlignment, "deluge_aligned_alloc_select", CI);
          AlignmentValue->setDebugLoc(CI->getDebugLoc());
          Alloc = CallInst::Create(
            isHard ? TryHardAllocateIntWithAlignment : TryAllocateIntWithAlignment,
            { Count, ConstantInt::get(IntPtrTy, Size), AlignmentValue },
            "deluge_alloc_int", CI);
        } else {
          Value* Heap = getHeap(
            DTD, CI, isHard ? ConstantPoolEntryKind::HardHeap : ConstantPoolEntryKind::Heap);
          Alloc = CallInst::Create(
            isHard ? TryHardAllocateManyWithAlignment : TryAllocateManyWithAlignment,
            { Heap, Count, PassedAlignment }, "deluge_alloc_many", CI);
        }
        
        Alloc->setDebugLoc(CI->getDebugLoc());
        CI->replaceAllUsesWith(
          forgePtrWithTypeRepAndCount(Alloc, getTypeRep(DTD, CI), LowT, CI->getArgOperand(2), CI));
        CI->eraseFromParent();
        return true;
      }

      if (CI->getCalledOperand() == ZreallocImpl || CI->getCalledOperand() == ZhardReallocImpl) {
        if (verbose)
          errs() << "Lowering realloc\n";
        lowerConstantOperand(CI->getArgOperandUse(0), CI, LowRawNull);
        lowerConstantOperand(CI->getArgOperandUse(2), CI, LowRawNull);
        Value* OrigPtr = lowerPtr(CI->getArgOperand(0), CI);
        Type* HighT = cast<AllocaInst>(CI->getArgOperand(1))->getAllocatedType();
        Type* LowT = lowerType(HighT);
        assert(hasPtrsForCheck(HighT) == hasPtrsForCheck(LowT));
        Instruction* Alloc = nullptr;
        
        DelugeTypeData *DTD = dataForLowType(LowT);
        assert(DTD->Type.Main.Size);
        if (!hasPtrsForCheck(HighT)) {
          assert(DTD == &Int);
          size_t Alignment = DL.getABITypeAlign(LowT).value();
          size_t Size = DL.getTypeStoreSize(LowT);
          if (Alignment > MinAlign) {
            Alloc = CallInst::Create(
              isHard ? TryHardReallocateIntWithAlignment : TryReallocateIntWithAlignment,
              { OrigPtr, CI->getArgOperand(2), ConstantInt::get(IntPtrTy, Size),
                ConstantInt::get(IntPtrTy, Alignment) },
              "deluge_realloc_int", CI);
          } else {
            Alloc = CallInst::Create(
              isHard ? TryHardReallocateInt : TryReallocateInt,
              { OrigPtr, CI->getArgOperand(2), ConstantInt::get(IntPtrTy, Size) },
              "deluge_realloc_int", CI);
          }
        } else {
          Value* Heap = getHeap(
            DTD, CI, isHard ? ConstantPoolEntryKind::HardHeap : ConstantPoolEntryKind::Heap);
          Alloc = CallInst::Create(
            isHard ? TryHardReallocate : TryReallocate,
            { OrigPtr, Heap, CI->getArgOperand(2) }, "deluge_realloc", CI);
        }
        
        Alloc->setDebugLoc(CI->getDebugLoc());
        CI->replaceAllUsesWith(
          forgePtrWithTypeRepAndCount(Alloc, getTypeRep(DTD, CI), LowT, CI->getArgOperand(2), CI));
        CI->eraseFromParent();
        return true;
      }

      if (CI->getCalledOperand() == ZallocFlexImpl || CI->getCalledOperand() == ZhardAllocFlexImpl) {
        if (verbose)
          errs() << "Lowering alloc_flex\n";
        lowerConstantOperand(CI->getArgOperandUse(1), CI, LowRawNull);
        lowerConstantOperand(CI->getArgOperandUse(3), CI, LowRawNull);
        Type* HighT = cast<AllocaInst>(CI->getArgOperand(0))->getAllocatedType();
        Type* LowT = lowerType(HighT);
        assert(hasPtrsForCheck(HighT) == hasPtrsForCheck(LowT));
        Type* HighTrailingT = cast<AllocaInst>(CI->getArgOperand(2))->getAllocatedType();
        Type* LowTrailingT = lowerType(HighTrailingT);
        assert(hasPtrsForCheck(HighTrailingT) == hasPtrsForCheck(LowTrailingT));
        Instruction* Alloc;
        
        DelugeTypeData* BaseDTD = dataForLowType(LowT);
        DelugeTypeData* TrailingDTD = dataForLowType(LowTrailingT);
        assert(BaseDTD->Type.Main.Size);
        assert(TrailingDTD->Type.Main.Size);

        DelugeType FlexType = BaseDTD->Type;
        FlexType.Main.truncate(static_cast<size_t>(cast<ConstantInt>(CI->getArgOperand(1))->getZExtValue()));
        assert(!FlexType.Trailing.isValid());
        assert(!TrailingDTD->Type.Trailing.isValid());
        if (verbose) {
          errs() << "TrailingDTD->Type.Main.isValid() = " << TrailingDTD->Type.Main.isValid() << "\n";
          errs() << "TrailingDTD->Type.Main.canBeInt() = " << TrailingDTD->Type.Main.canBeInt() << "\n";
        }
        FlexType.Trailing = TrailingDTD->Type.Main;
        if (verbose) {
          errs() << "FlexType.Trailing.isValid() = " << FlexType.Trailing.isValid() << "\n";
          errs() << "FlexType.Trailing.canBeInt() = " << FlexType.Trailing.canBeInt() << "\n";
        }
        FlexType.Main.Alignment = std::max(FlexType.Main.Alignment, FlexType.Trailing.Alignment);

        DelugeTypeData* DTD;
        if (FlexType.canBeInt())
          DTD = &Int;
        else
          DTD = dataForType(FlexType); 
        
        size_t BaseSize = FlexType.Main.Size;
        size_t ElementSize = DL.getTypeStoreSize(LowTrailingT);
        Value* Count = CI->getArgOperand(3);
        if (DTD == &Int) {
          size_t Alignment = FlexType.Main.Alignment;
          if (Alignment > MinAlign) {
            Alloc = CallInst::Create(
              isHard ? TryHardAllocateIntFlexWithAlignment : TryAllocateIntFlexWithAlignment,
              { ConstantInt::get(IntPtrTy, BaseSize), ConstantInt::get(IntPtrTy, ElementSize),
                Count, ConstantInt::get(IntPtrTy, Alignment) },
              "deluge_alloc_int", CI);
          } else {
            Alloc = CallInst::Create(
              isHard ? TryHardAllocateIntFlex : TryAllocateIntFlex,
              { ConstantInt::get(IntPtrTy, BaseSize), ConstantInt::get(IntPtrTy, ElementSize), Count },
              "deluge_alloc_int", CI);
          }
        } else {
          Value* Heap = getHeap(
            DTD, CI, isHard ? ConstantPoolEntryKind::HardHeap : ConstantPoolEntryKind::Heap);
          Alloc = CallInst::Create(
            isHard ? TryHardAllocateFlex : TryAllocateFlex,
            { Heap, ConstantInt::get(IntPtrTy, BaseSize), ConstantInt::get(IntPtrTy, ElementSize), Count },
            "deluge_alloc_many", CI);
        }
        
        Alloc->setDebugLoc(CI->getDebugLoc());
        Instruction* ArraySize = BinaryOperator::Create(
          Instruction::Mul, Count, ConstantInt::get(IntPtrTy, DL.getTypeStoreSize(LowTrailingT)),
          "deluge_alloc_array_size", CI);
        ArraySize->setDebugLoc(CI->getDebugLoc());
        Instruction* Size = BinaryOperator::Create(
          Instruction::Add, ArraySize, ConstantInt::get(IntPtrTy, BaseSize),
          "deluge_alloc_size", CI);
        Size->setDebugLoc(CI->getDebugLoc());
        CI->replaceAllUsesWith(forgePtrWithTypeRepAndSize(Alloc, Size, getTypeRep(DTD, CI), CI));
        CI->eraseFromParent();
        return true;
      }

      if (CI->getCalledOperand() == ZtypeofImpl) {
        if (verbose)
          errs() << "Lowering ztypeof\n";
        Type* HighT = cast<AllocaInst>(CI->getArgOperand(0))->getAllocatedType();
        Type* LowT = lowerType(HighT);
        Value* TypeRep = getTypeRep(dataForLowType(LowT), CI);
        CI->replaceAllUsesWith(
          forgePtrWithTypeRepAndSize(TypeRep, ConstantInt::get(IntPtrTy, 1), TypeTypeRep, CI));
        CI->eraseFromParent();
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
      DelugeTypeData* DTD = dataForLowType(LowT);
      assert(DTD->Type.Main.Size);
      if (AI->getParent() != FirstRealBlock || AI->isArrayAllocation()) {
        // This is the especially fun case of a dynamic alloca! We allocate something and then pool it
        // until return.
        Instruction* Alloc = nullptr;;
        if (DTD == &Int) {
          size_t Alignment = DL.getABITypeAlign(LowT).value();
          size_t Size = DL.getTypeStoreSize(LowT);
          if (Alignment > MinAlign) {
            Alloc = CallInst::Create(
              AllocateIntWithAlignment,
              { AI->getArraySize(), ConstantInt::get(IntPtrTy, Size),
                ConstantInt::get(IntPtrTy, Alignment) },
              "deluge_alloca_int", AI);
          } else {
            Alloc = CallInst::Create(
              AllocateInt,
              { AI->getArraySize(), ConstantInt::get(IntPtrTy, Size) },
              "deluge_alloca_int", AI);
          }
        } else {
          Value* Heap = getHeap(DTD, AI, ConstantPoolEntryKind::Heap);
          if (Constant* C = dyn_cast<Constant>(AI->getArraySize())) {
            if (C->isOneValue())
              Alloc = CallInst::Create(AllocateOne, { Heap }, "deluge_alloca_one", AI);
          }
          if (!Alloc) {
            Alloc = CallInst::Create(
              AllocateMany, { Heap, AI->getArraySize() }, "deluge_alloc_many", AI);
          }
        }
        Alloc->setDebugLoc(AI->getDebugLoc());
        CallInst::Create(
          AllocaStackPush, { FutureAllocaStack, Alloc }, "", AI)->setDebugLoc(AI->getDebugLoc());
        AI->replaceAllUsesWith(
          forgePtrWithTypeRepAndCount(Alloc, getTypeRep(DTD, AI), LowT, AI->getArraySize(), AI));
        AI->eraseFromParent();
        return;
      }
      Value* Base;
      size_t Offset;
      if (!hasPtrsForCheck(LowT)) {
        assert(DTD == &Int);
        size_t Alignment = DL.getABITypeAlign(LowT).value();
        size_t Size = DL.getTypeStoreSize(LowT);
        Base = FutureIntFrame;
        IntFrameSize = (IntFrameSize + Alignment - 1) / Alignment * Alignment;
        IntFrameAlignment = std::max(IntFrameAlignment, Alignment);
        Offset = IntFrameSize;
        IntFrameSize += Size;
        // libpas doesn't have the requirement that the size is already aligned, but there are perf
        // benefits to doing so.
        IntFrameSize = (IntFrameSize + IntFrameAlignment - 1) / IntFrameAlignment * IntFrameAlignment;
      } else {
        assert(!DTD->Type.Trailing.isValid());
        Base = FutureTypedFrame;
        Offset = TypedFrameType.append(DTD->Type.Main);
      }
      Instruction* EntryPtr = GetElementPtrInst::Create(
        Int8Ty, Base, { ConstantInt::get(IntPtrTy, Offset) }, "deluge_alloca_entry_ptr", AI);
      EntryPtr->setDebugLoc(AI->getDebugLoc());
      AI->replaceAllUsesWith(
        forgePtrWithTypeRepAndCount(
          EntryPtr, getTypeRep(DTD, AI), LowT, ConstantInt::get(IntPtrTy, 1), AI));
      AI->eraseFromParent();
      return;
    }

    if (LoadInst* LI = dyn_cast<LoadInst>(I)) {
      Type *LowT = lowerType(LI->getType());
      Value* HighP = LI->getPointerOperand();
      Value* Result = loadValueRecurse(
        LowT, HighP, lowerPtr(HighP, LI),
        LI->isVolatile(), LI->getAlign(), LI->getOrdering(), LI->getSyncScopeID(),
        LI);
      LI->replaceAllUsesWith(Result);
      LI->eraseFromParent();
      return;
    }

    if (StoreInst* SI = dyn_cast<StoreInst>(I)) {
      Value* HighP = SI->getPointerOperand();
      storeValueRecurse(
        InstLowTypes[SI], HighP, SI->getValueOperand(), lowerPtr(HighP, SI),
        SI->isVolatile(), SI->getAlign(), SI->getOrdering(), SI->getSyncScopeID(),
        SI);
      SI->eraseFromParent();
      return;
    }

    if (FenceInst* FI = dyn_cast<FenceInst>(I)) {
      // We don't need to do anything because it doesn't take operands.
      return;
    }

    if (AtomicCmpXchgInst* AI = dyn_cast<AtomicCmpXchgInst>(I)) {
      assert(!hasPtrsForCheck(InstLowTypes[AI]));
      Value* LowP = prepareForAccess(InstLowTypes[AI], AI->getPointerOperand(), AI);
      AI->getOperandUse(AtomicCmpXchgInst::getPointerOperandIndex()) = LowP;

      // FIXME: This code is dead right now because clang never generates CAS on pointers.
      if (hasPtrsForCheck(InstLowTypes[AI])) {
        // We *only* allow CAS of pointers. Maybe at some point we might allow CASing structs that
        // have exactly one pointer.
        assert(InstLowTypes[AI] == LowWidePtrTy);
        (new StoreInst(
          ConstantInt::get(Int128Ty, 0), sidecarPtr(LowP, AI),
          false, Align(WordSize), getMergedAtomicOrdering(AtomicOrdering::Monotonic,
                                                          AI->getMergedOrdering()),
          SyncScope::System, AI))
          ->setDebugLoc(AI->getDebugLoc());
        Instruction* CAS = new AtomicCmpXchgInst(
          capabilityPtr(LowP, AI),
          extractCapability(AI->getCompareOperand(), AI), extractCapability(AI->getNewValOperand(), AI),
          Align(WordSize), AI->getSuccessOrdering(), AI->getFailureOrdering(), AI->getSyncScopeID());
        CAS->setDebugLoc(AI->getDebugLoc());
        Instruction* OldValue = ExtractValueInst::Create(
          Int128Ty, CAS, { 0 }, "deluge_cas_extract_old_value", AI);
        OldValue->setDebugLoc(AI->getDebugLoc());
        Instruction* DidSwap = ExtractValueInst::Create(
          Int1Ty, CAS, { 1 }, "deluge_cas_extract_did_swap", AI);
        DidSwap->setDebugLoc(AI->getDebugLoc());
        StructType* ST = StructType::get(C, { InstLowTypes[AI], Int1Ty });
        Instruction* Result = InsertValueInst::Create(
          UndefValue::get(ST), createPtr(ConstantInt::get(Int128Ty, 0), OldValue, AI), { 0 },
          "deluge_cas_insert_old_value", AI);
        Result->setDebugLoc(AI->getDebugLoc());
        Result = InsertValueInst::Create(Result, DidSwap, { 1 }, "deluge_cas_insert_did_swap", AI);
        Result->setDebugLoc(AI->getDebugLoc());
        AI->replaceAllUsesWith(Result);
        AI->eraseFromParent();
      }
      return;
    }

    if (AtomicRMWInst* AI = dyn_cast<AtomicRMWInst>(I)) {
      assert(!hasPtrsForCheck(InstLowTypes[AI]));
      AI->getOperandUse(AtomicRMWInst::getPointerOperandIndex()) =
        prepareForAccess(AI->getValOperand()->getType(), AI->getPointerOperand(), AI);
      return;
    }

    if (GetElementPtrInst* GI = dyn_cast<GetElementPtrInst>(I)) {
      GI->setSourceElementType(lowerType(GI->getSourceElementType()));
      GI->setResultElementType(lowerType(GI->getResultElementType()));
      Value* HighP = GI->getOperand(0);
      GI->getOperandUse(0) = lowerPtr(HighP, GI);
      hackRAUW(GI, [&] () { return reforgePtr(HighP, GI, GI->getNextNode()); });
      return;
    }

    if (ICmpInst* CI = dyn_cast<ICmpInst>(I)) {
      if (hasPtrsForCheck(CI->getOperand(0)->getType())) {
        CI->getOperandUse(0) = lowerPtr(CI->getOperand(0), CI);
        CI->getOperandUse(1) = lowerPtr(CI->getOperand(1), CI);
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
        CallInst::Create(Error, { getOrigin(I->getDebugLoc()) }, "", I)
          ->setDebugLoc(I->getDebugLoc());
        if (I->getType() != VoidTy) {
          // We need to produce something to RAUW the call with, but it cannot be a constant, since
          // that would upset lowerConstant.
          Type* LowT = lowerType(I->getType());
          LoadInst* LI = new LoadInst(LowT, LowRawNull, "deluge_fake_load", I);
          LI->setDebugLoc(I->getDebugLoc());
          CI->replaceAllUsesWith(LI);
        }
        CI->eraseFromParent();
        return;
      }

      if (verbose)
        errs() << "Dealing with called operand: " << *CI->getCalledOperand() << "\n";

      assert(CI->getCalledOperand() != ZunsafeForgeImpl);
      assert(CI->getCalledOperand() != ZrestrictImpl);
      assert(CI->getCalledOperand() != ZallocImpl);
      assert(CI->getCalledOperand() != ZallocFlexImpl);
      assert(CI->getCalledOperand() != ZreallocImpl);
      assert(CI->getCalledOperand() != ZalignedAllocImpl);
      assert(CI->getCalledOperand() != ZhardAllocImpl);
      assert(CI->getCalledOperand() != ZhardAllocFlexImpl);
      assert(CI->getCalledOperand() != ZhardReallocImpl);
      assert(CI->getCalledOperand() != ZhardAlignedAllocImpl);
      assert(CI->getCalledOperand() != ZtypeofImpl);
      
      CallInst::Create(
        CheckAccessFunctionCall, { CI->getCalledOperand(), getOrigin(CI->getDebugLoc()) }, "", CI)
        ->setDebugLoc(CI->getDebugLoc());

      Value* ArgBufferRawPtrValue;
      Value* ArgBufferUpperValue;
      Value* ArgTypeRep;

      FunctionType *FT = CI->getFunctionType();
      
      if (CI->arg_size()) {
        DelugeType ArgType;
        std::vector<Type*> ArgTypes = InstLowTypeVectors[CI];
        std::vector<size_t> Offsets;
        for (size_t Index = 0; Index < CI->arg_size(); ++Index) {
          Type* LowT = ArgTypes[Index];
          ArgType.Main.pad(DL.getABITypeAlign(LowT).value());
          Offsets.push_back(ArgType.Main.Size);
          buildCoreTypeRecurse(ArgType.Main, LowT);
        }

        ArgType.Main.pad(ArgType.Main.Alignment);

        DelugeTypeData* ArgDTD;
        Instruction* ArgBufferRawPtr;
        if (ArgType.canBeInt()) {
          ArgDTD = &Int;
          if (ArgType.Main.Alignment > MinAlign) {
            ArgBufferRawPtr = CallInst::Create(
              AllocateIntWithAlignment,
              { ConstantInt::get(IntPtrTy, ArgType.Main.Size),
                ConstantInt::get(IntPtrTy, 1),
                ConstantInt::get(IntPtrTy, ArgType.Main.Alignment) },
              "deluge_allocate_args", CI);
          } else {
            ArgBufferRawPtr = CallInst::Create(
              AllocateInt,
              { ConstantInt::get(IntPtrTy, ArgType.Main.Size), ConstantInt::get(IntPtrTy, 1) },
              "deluge_allocate_args", CI);
          }
        } else {
          ArgDTD = dataForType(ArgType);
          ArgBufferRawPtr = CallInst::Create(
            AllocateOne, { getHeap(ArgDTD, CI, ConstantPoolEntryKind::Heap) }, "deluge_allocate_args", CI);
        }

        ArgBufferRawPtr->setDebugLoc(CI->getDebugLoc());
        Instruction* ArgBufferUpper = GetElementPtrInst::Create(
          Int8Ty, ArgBufferRawPtr, { ConstantInt::get(IntPtrTy, ArgType.Main.Size) },
          "deluge_arg_buffer_upper", CI);
        ArgBufferUpper->setDebugLoc(CI->getDebugLoc());

        assert(FT->getNumParams() <= CI->arg_size());
        assert(FT->getNumParams() == CI->arg_size() || FT->isVarArg());
        for (size_t Index = 0; Index < CI->arg_size(); ++Index) {
          Value* Arg = CI->getArgOperand(Index);
          Type* LowT = ArgTypes[Index];
          assert(Arg->getType() == LowT || lowerType(Arg->getType()) == LowT);
          assert(Index < Offsets.size());
          Instruction* ArgSlotPtr = GetElementPtrInst::Create(
            Int8Ty, ArgBufferRawPtr, { ConstantInt::get(IntPtrTy, Offsets[Index]) }, "deluge_arg_slot", CI);
          ArgSlotPtr->setDebugLoc(CI->getDebugLoc());
          storeValueUncheckedRecurse(
            LowT, Arg, ArgSlotPtr, false, DL.getABITypeAlign(LowT), AtomicOrdering::NotAtomic,
            SyncScope::System, CI);
        }
        ArgBufferRawPtrValue = ArgBufferRawPtr;
        ArgBufferUpperValue = ArgBufferUpper;
        ArgTypeRep = getTypeRep(ArgDTD, CI);
      } else {
        ArgBufferRawPtrValue = LowRawNull;
        ArgBufferUpperValue = LowRawNull;
        ArgTypeRep = LowRawNull;
      }

      Type* LowRetT = lowerType(FT->getReturnType());
      DelugeTypeData* RetDTD;
      size_t RetSize;
      size_t RetAlign;
      if (LowRetT == VoidTy) {
        RetDTD = &Int;
        RetSize = 0;
        RetAlign = 1;
      } else {
        RetDTD = dataForLowType(LowRetT);
        RetSize = DL.getTypeStoreSize(LowRetT);
        RetAlign = DL.getABITypeAlign(LowRetT).value();
      }
      if (RetDTD == &Int)
        RetSize = std::max(static_cast<size_t>(16), RetSize);
      ReturnBufferSize = std::max(ReturnBufferSize, RetSize);
      ReturnBufferAlignment = std::max(ReturnBufferAlignment, static_cast<size_t>(RetAlign));

      Instruction* ClearRetBuffer = CallInst::Create(
        RealMemset,
        { FutureReturnBuffer, ConstantInt::get(Int8Ty, 0), ConstantInt::get(IntPtrTy, RetSize),
          ConstantInt::get(Int1Ty, false) }, "", CI);
      ClearRetBuffer->setDebugLoc(CI->getDebugLoc());

      Instruction* RetBufferUpper = GetElementPtrInst::Create(
        Int8Ty, FutureReturnBuffer, { ConstantInt::get(IntPtrTy, RetSize) }, "deluge_return_upper", CI);
      RetBufferUpper->setDebugLoc(CI->getDebugLoc());

      assert(!CI->hasOperandBundles());
      CallInst::Create(
        DeludedFuncTy, lowerPtr(CI->getCalledOperand(), CI),
        { ArgBufferRawPtrValue, ArgBufferUpperValue, ArgTypeRep,
          FutureReturnBuffer, RetBufferUpper, getTypeRep(RetDTD, CI) },
        "", CI);

      if (LowRetT != VoidTy) {
        // It's OK to load from the return buffer without breaking it down into atomics for pointers
        // because the return buffer has these tight ABI/compiler-controlled ownership passing semantics.
        // So, they're definitely thread-local.
        CI->replaceAllUsesWith(new LoadInst(LowRetT, FutureReturnBuffer, "deluge_result_load", CI));
      }
      CI->eraseFromParent();
      return;
    }

    if (VAArgInst* VI = dyn_cast<VAArgInst>(I)) {
      Type* T = VI->getType();
      Type* LowT = lowerType(T);
      size_t Size = DL.getTypeStoreSize(LowT);
      size_t Alignment = DL.getABITypeAlign(LowT).value();
      DelugeTypeData* DTD = dataForLowType(LowT);
      assert(DTD->Type.Main.Size);
      assert(!DTD->Type.Trailing.isValid());
      assert(!(Size % DTD->Type.Main.Size));
      CallInst* Call = CallInst::Create(
        VAArgImpl,
        { VI->getPointerOperand(), ConstantInt::get(IntPtrTy, Size / DTD->Type.Main.Size),
          ConstantInt::get(IntPtrTy, Alignment), getTypeRep(DTD, VI), getOrigin(VI->getDebugLoc()) },
        "deluge_va_arg", VI);
      Call->setDebugLoc(VI->getDebugLoc());
      Value* Load = loadValueUncheckedRecurse(
        LowT, Call, false, DL.getABITypeAlign(LowT), AtomicOrdering::NotAtomic, SyncScope::System, VI);
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
      CallInst::Create(Error, { getOrigin(I->getDebugLoc()) }, "", I)->setDebugLoc(I->getDebugLoc());
      return;
    }

    if (isa<IntToPtrInst>(I)) {
      hackRAUW(I, [&] () { return forgeBadPtr(I, I->getNextNode()); });
      return;
    }

    if (isa<PtrToIntInst>(I)) {
      I->getOperandUse(0) = lowerPtr(I->getOperand(0), I);
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
          hackRAUW(I, [&] () { return forgeBadPtr(I, I->getNextNode()); });
      } else if (hasPtrsForCheck(I->getOperand(0)->getType()))
        I->getOperandUse(0) = lowerPtr(I->getOperand(0), I);
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
            G->getName() == "llvm.global_dtors");
  }

  bool shouldPassThrough(GlobalValue* G) {
    if (Function* F = dyn_cast<Function>(G))
      return shouldPassThrough(F);
    if (GlobalVariable* V = dyn_cast<GlobalVariable>(G))
      return shouldPassThrough(V);
    return false;
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
  Deluge(Module &M, ModuleAnalysisManager &MAM)
    : C(M.getContext()), M(M), DL(const_cast<DataLayout&>(M.getDataLayout())), MAM(MAM) {
  }

  void run() {
    if (verbose)
      errs() << "Going to town on module:\n" << M << "\n";

    prepare();

    if (verbose)
      errs() << "Prepared module:\n" << M << "\n";

    FunctionName = "<internal>";
    
    PtrBits = DL.getPointerSizeInBits(TargetAS);
    VoidTy = Type::getVoidTy(C);
    Int1Ty = Type::getInt1Ty(C);
    Int8Ty = Type::getInt8Ty(C);
    Int32Ty = Type::getInt32Ty(C);
    IntPtrTy = Type::getIntNTy(C, PtrBits);
    assert(IntPtrTy == Type::getInt64Ty(C)); // Deluge is 64-bit-only, for now.
    Int128Ty = Type::getInt128Ty(C);
    LowRawPtrTy = PointerType::get(C, TargetAS);
    LowWidePtrTy = StructType::create({Int128Ty, Int128Ty}, "deluge_wide_ptr");
    OriginTy = StructType::create({LowRawPtrTy, LowRawPtrTy, Int32Ty, Int32Ty}, "deluge_origin");
    AllocaStackTy = StructType::create({LowRawPtrTy, IntPtrTy, IntPtrTy}, "deluge_alloca_bag");
    ConstantRelocationTy = StructType::create(
      {IntPtrTy, Int32Ty, LowRawPtrTy}, "deluge_constant_relocation");
    // See DELUDED_SIGNATURE in deluge_runtime.h.
    DeludedFuncTy = FunctionType::get(
      VoidTy, { LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, LowRawPtrTy }, false);
    GlobalGetterTy = FunctionType::get(Int128Ty, { LowRawPtrTy }, false);
    CtorDtorTy = FunctionType::get(VoidTy, false);
    LowRawNull = ConstantPointerNull::get(LowRawPtrTy);

    // Fuck the DSO!
    if (GlobalVariable* DSO = M.getGlobalVariable("__dso_handle")) {
      assert(DSO->isDeclaration());
      DSO->replaceAllUsesWith(LowRawNull);
      DSO->eraseFromParent();
    }
    
    ZunsafeForgeImpl = M.getOrInsertFunction(
      "zunsafe_forge_impl", LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, IntPtrTy).getCallee();
    ZrestrictImpl = M.getOrInsertFunction(
      "zrestrict_impl", LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, IntPtrTy).getCallee();
    ZallocImpl = M.getOrInsertFunction(
      "zalloc_impl", LowRawPtrTy, LowRawPtrTy, IntPtrTy).getCallee();
    ZallocFlexImpl = M.getOrInsertFunction(
      "zalloc_flex_impl", LowRawPtrTy, LowRawPtrTy, IntPtrTy, LowRawPtrTy, IntPtrTy).getCallee();
    ZalignedAllocImpl = M.getOrInsertFunction(
      "zaligned_alloc_impl", LowRawPtrTy, LowRawPtrTy, IntPtrTy, IntPtrTy).getCallee();
    ZreallocImpl = M.getOrInsertFunction(
      "zrealloc_impl", LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, IntPtrTy).getCallee();
    ZhardAllocImpl = M.getOrInsertFunction(
      "zhard_alloc_impl", LowRawPtrTy, LowRawPtrTy, IntPtrTy).getCallee();
    ZhardAllocFlexImpl = M.getOrInsertFunction(
      "zhard_alloc_flex_impl", LowRawPtrTy, LowRawPtrTy, IntPtrTy, LowRawPtrTy, IntPtrTy).getCallee();
    ZhardAlignedAllocImpl = M.getOrInsertFunction(
      "zhard_aligned_alloc_impl", LowRawPtrTy, LowRawPtrTy, IntPtrTy, IntPtrTy).getCallee();
    ZhardReallocImpl = M.getOrInsertFunction(
      "zhard_realloc_impl", LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, IntPtrTy).getCallee();
    ZtypeofImpl = M.getOrInsertFunction("ztypeof_impl", LowRawPtrTy, LowRawPtrTy).getCallee();

    assert(cast<Function>(ZunsafeForgeImpl)->isDeclaration());
    assert(cast<Function>(ZrestrictImpl)->isDeclaration());
    assert(cast<Function>(ZallocImpl)->isDeclaration());
    assert(cast<Function>(ZallocFlexImpl)->isDeclaration());
    assert(cast<Function>(ZalignedAllocImpl)->isDeclaration());
    assert(cast<Function>(ZreallocImpl)->isDeclaration());
    assert(cast<Function>(ZhardAllocImpl)->isDeclaration());
    assert(cast<Function>(ZhardAllocFlexImpl)->isDeclaration());
    assert(cast<Function>(ZhardAlignedAllocImpl)->isDeclaration());
    assert(cast<Function>(ZhardReallocImpl)->isDeclaration());
    assert(cast<Function>(ZtypeofImpl)->isDeclaration());
    
    if (verbose) {
      errs() << "zunsafe_forge_impl = " << ZunsafeForgeImpl << "\n";
      errs() << "zrestrict_impl = " << ZrestrictImpl << "\n";
      errs() << "zalloc_impl = " << ZallocImpl << "\n";
      errs() << "zalloc_flex_impl = " << ZallocFlexImpl << "\n";
      errs() << "zaligned_lloc_impl = " << ZalignedAllocImpl << "\n";
      errs() << "zrealloc_impl = " << ZreallocImpl << "\n";
      errs() << "zhard_alloc_impl = " << ZhardAllocImpl << "\n";
      errs() << "zhard_alloc_flex_impl = " << ZhardAllocFlexImpl << "\n";
      errs() << "zhard_aligned_lloc_impl = " << ZhardAlignedAllocImpl << "\n";
      errs() << "zhard_realloc_impl = " << ZhardReallocImpl << "\n";
      errs() << "ztypeof_impl = " << ZtypeofImpl << "\n";
    }

    LocalConstantPoolPtr = nullptr;

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
    for (GlobalIFunc &G : M.ifuncs()) {
      IFuncs.push_back(&G);
      CaptureType(&G);
    }

    Int.Type = DelugeType(1, 1);
    Int.Type.Main.WordTypes.push_back(DelugeWordType::Int);
    Int.TemplateRep = new GlobalVariable(
      M, ArrayType::get(IntPtrTy, 4), true, GlobalVariable::ExternalLinkage, nullptr,
      "deluge_int_type_template");
    IntTypeRep = new GlobalVariable(
      M, ArrayType::get(IntPtrTy, 4), true, GlobalVariable::ExternalLinkage, nullptr, "deluge_int_type");
    FunctionDTD.Type = DelugeType(0, 0);
    FunctionTypeRep = new GlobalVariable(
      M, ArrayType::get(IntPtrTy, 4), true, GlobalVariable::ExternalLinkage, nullptr,
      "deluge_function_type");
    TypeDTD.Type = DelugeType(0, 0);
    TypeTypeRep = new GlobalVariable(
      M, ArrayType::get(IntPtrTy, 4), true, GlobalVariable::ExternalLinkage, nullptr, "deluge_type_type");
    Invalid.Type = DelugeType(0, 0);
    
    LowWideNull = ConstantStruct::get(
      LowWidePtrTy, { ConstantInt::get(Int128Ty, 0), ConstantInt::get(Int128Ty, 0) });
    if (verbose)
      errs() << "LowWideNull = " << *LowWideNull << "\n";

    Dummy = makeDummy(Int32Ty);
    FutureIntFrame = makeDummy(LowRawPtrTy);
    FutureTypedFrame = makeDummy(LowRawPtrTy);
    FutureReturnBuffer = makeDummy(LowRawPtrTy);
    FutureAllocaStack = makeDummy(LowRawPtrTy);

    ValidateType = M.getOrInsertFunction("deluge_validate_type", VoidTy, LowRawPtrTy, LowRawPtrTy);
    GetType = M.getOrInsertFunction("deluge_get_type", LowRawPtrTy, LowRawPtrTy);
    GetHeap = M.getOrInsertFunction("deluge_get_heap", LowRawPtrTy, LowRawPtrTy);
    TryAllocateInt = M.getOrInsertFunction("deluge_try_allocate_int", LowRawPtrTy, IntPtrTy, IntPtrTy);
    TryAllocateIntWithAlignment = M.getOrInsertFunction("deluge_try_allocate_int_with_alignment", LowRawPtrTy, IntPtrTy, IntPtrTy, IntPtrTy);
    AllocateInt = M.getOrInsertFunction("deluge_allocate_int", LowRawPtrTy, IntPtrTy, IntPtrTy);
    AllocateIntWithAlignment = M.getOrInsertFunction("deluge_allocate_int_with_alignment", LowRawPtrTy, IntPtrTy, IntPtrTy, IntPtrTy);
    TryAllocateOne = M.getOrInsertFunction("deluge_try_allocate_one", LowRawPtrTy, LowRawPtrTy);
    AllocateOne = M.getOrInsertFunction("deluge_allocate_one", LowRawPtrTy, LowRawPtrTy);
    TryAllocateMany = M.getOrInsertFunction("deluge_try_allocate_many", LowRawPtrTy, LowRawPtrTy, IntPtrTy);
    TryAllocateManyWithAlignment = M.getOrInsertFunction("deluge_try_allocate_many_with_alignment", LowRawPtrTy, LowRawPtrTy, IntPtrTy, IntPtrTy);
    AllocateMany = M.getOrInsertFunction("deluge_allocate_many", LowRawPtrTy, LowRawPtrTy, IntPtrTy);
    TryAllocateIntFlex = M.getOrInsertFunction("deluge_try_allocate_int_flex", LowRawPtrTy, IntPtrTy, IntPtrTy, IntPtrTy);
    TryAllocateIntFlexWithAlignment = M.getOrInsertFunction("deluge_try_allocate_int_flex_with_alignment", LowRawPtrTy, IntPtrTy, IntPtrTy, IntPtrTy, IntPtrTy);
    TryAllocateFlex = M.getOrInsertFunction("deluge_try_allocate_flex", LowRawPtrTy, LowRawPtrTy, IntPtrTy, IntPtrTy, IntPtrTy);
    AllocateUtility = M.getOrInsertFunction("deluge_allocate_utility", LowRawPtrTy, IntPtrTy);
    TryReallocateInt = M.getOrInsertFunction("deluge_try_reallocate_int", LowRawPtrTy, LowRawPtrTy, IntPtrTy, IntPtrTy);
    TryReallocateIntWithAlignment = M.getOrInsertFunction("deluge_try_reallocate_int", LowRawPtrTy, LowRawPtrTy, IntPtrTy, IntPtrTy, IntPtrTy);
    TryReallocate = M.getOrInsertFunction("deluge_try_reallocate", LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, IntPtrTy);
    Deallocate = M.getOrInsertFunction("deluge_deallocate", VoidTy, LowRawPtrTy);
    GetHardHeap = M.getOrInsertFunction("deluge_get_hard_heap", LowRawPtrTy, LowRawPtrTy);
    TryHardAllocateInt = M.getOrInsertFunction("deluge_try_hard_allocate_int", LowRawPtrTy, IntPtrTy, IntPtrTy);
    TryHardAllocateIntWithAlignment = M.getOrInsertFunction("deluge_try_hard_allocate_int_with_alignment", LowRawPtrTy, IntPtrTy, IntPtrTy, IntPtrTy);
    TryHardAllocateOne = M.getOrInsertFunction("deluge_try_hard_allocate_one", LowRawPtrTy, LowRawPtrTy);
    TryHardAllocateMany = M.getOrInsertFunction("deluge_try_hard_allocate_many", LowRawPtrTy, LowRawPtrTy, IntPtrTy);
    TryHardAllocateManyWithAlignment = M.getOrInsertFunction("deluge_try_hard_allocate_many_with_alignment", LowRawPtrTy, LowRawPtrTy, IntPtrTy, IntPtrTy);
    TryHardAllocateIntFlex = M.getOrInsertFunction("deluge_try_hard_allocate_int_flex", LowRawPtrTy, IntPtrTy, IntPtrTy, IntPtrTy);
    TryHardAllocateIntFlexWithAlignment = M.getOrInsertFunction("deluge_try_hard_allocate_int_flex_with_alignment", LowRawPtrTy, IntPtrTy, IntPtrTy, IntPtrTy, IntPtrTy);
    TryHardAllocateFlex = M.getOrInsertFunction("deluge_try_hard_allocate_flex", LowRawPtrTy, LowRawPtrTy, IntPtrTy, IntPtrTy, IntPtrTy);
    TryHardReallocateInt = M.getOrInsertFunction("deluge_try_hard_reallocate_int", LowRawPtrTy, LowRawPtrTy, IntPtrTy, IntPtrTy);
    TryHardReallocateIntWithAlignment = M.getOrInsertFunction("deluge_try_hard_reallocate_int", LowRawPtrTy, LowRawPtrTy, IntPtrTy, IntPtrTy, IntPtrTy);
    TryHardReallocate = M.getOrInsertFunction("deluge_try_hard_reallocate", LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, IntPtrTy);
    PtrPtr = M.getOrInsertFunction("deluge_ptr_ptr_impl", LowRawPtrTy, LowWidePtrTy);
    UpdateSidecar = M.getOrInsertFunction("deluge_update_sidecar", Int128Ty, LowWidePtrTy, LowRawPtrTy);
    UpdateCapability = M.getOrInsertFunction("deluge_update_capability", Int128Ty, LowWidePtrTy, LowRawPtrTy);
    NewSidecar = M.getOrInsertFunction("deluge_new_sidecar", Int128Ty, LowRawPtrTy, IntPtrTy, LowRawPtrTy);
    NewCapability = M.getOrInsertFunction("deluge_new_capability", Int128Ty, LowRawPtrTy, IntPtrTy, LowRawPtrTy);
    CheckForge = M.getOrInsertFunction("deluge_check_forge", VoidTy, LowRawPtrTy, IntPtrTy, IntPtrTy, LowRawPtrTy, LowRawPtrTy);
    CheckAccessInt = M.getOrInsertFunction("deluge_check_access_int_impl", VoidTy, LowWidePtrTy, IntPtrTy, LowRawPtrTy);
    CheckAccessPtr = M.getOrInsertFunction("deluge_check_access_ptr_impl", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckAccessFunctionCall = M.getOrInsertFunction("deluge_check_function_call_impl", VoidTy, LowWidePtrTy, LowRawPtrTy);
    Memset = M.getOrInsertFunction("deluge_memset_impl", VoidTy, LowWidePtrTy, Int32Ty, IntPtrTy, LowRawPtrTy);
    Memcpy = M.getOrInsertFunction("deluge_memcpy_impl", VoidTy, LowWidePtrTy, LowWidePtrTy, IntPtrTy, LowRawPtrTy);
    Memmove = M.getOrInsertFunction("deluge_memmove_impl", VoidTy, LowWidePtrTy, LowWidePtrTy, IntPtrTy, LowRawPtrTy);
    CheckRestrict = M.getOrInsertFunction("deluge_check_restrict", VoidTy, LowWidePtrTy, LowRawPtrTy, LowRawPtrTy, LowRawPtrTy);
    VAArgImpl = M.getOrInsertFunction("deluge_va_arg_impl", LowRawPtrTy, LowWidePtrTy, IntPtrTy, IntPtrTy, LowRawPtrTy, LowRawPtrTy);
    GlobalInitializationContextCreate = M.getOrInsertFunction("deluge_global_initialization_context_create", LowRawPtrTy, LowRawPtrTy);
    GlobalInitializationContextAdd = M.getOrInsertFunction("deluge_global_initialization_context_add", Int1Ty, LowRawPtrTy, LowRawPtrTy, Int128Ty);
    GlobalInitializationContextDestroy = M.getOrInsertFunction("deluge_global_initialization_context_destroy", VoidTy, LowRawPtrTy);
    AllocaStackPush = M.getOrInsertFunction("deluge_alloca_stack_push", VoidTy, LowRawPtrTy, LowRawPtrTy);
    AllocaStackRestore = M.getOrInsertFunction(
      "deluge_alloca_stack_restore", VoidTy, LowRawPtrTy, IntPtrTy);
    AllocaStackDestroy = M.getOrInsertFunction("deluge_alloca_stack_destroy", VoidTy, LowRawPtrTy);
    ExecuteConstantRelocations = M.getOrInsertFunction("deluge_execute_constant_relocations", VoidTy, LowRawPtrTy, LowRawPtrTy, IntPtrTy, LowRawPtrTy);
    DeferOrRunGlobalCtor = M.getOrInsertFunction("deluge_defer_or_run_global_ctor", VoidTy, LowRawPtrTy);
    Error = M.getOrInsertFunction("deluge_error", VoidTy, LowRawPtrTy);
    RealMemset = M.getOrInsertFunction("llvm.memset.p0.i64", VoidTy, LowRawPtrTy, Int8Ty, IntPtrTy, Int1Ty);
    MakeConstantPool = M.getOrInsertFunction("deluge_make_constantpool", LowRawPtrTy);

    GlobalConstantPoolPtr = new GlobalVariable(
      M, LowRawPtrTy, false, GlobalValue::PrivateLinkage, LowRawNull, "deluge_global_constantpool_ptr");

    if (GlobalVariable* GlobalCtors = M.getGlobalVariable("llvm.global_ctors")) {
      ConstantArray* Array = cast<ConstantArray>(GlobalCtors->getInitializer());
      std::vector<Constant*> Args;
      for (size_t Index = 0; Index < Array->getNumOperands(); ++Index) {
        ConstantStruct* Struct = cast<ConstantStruct>(Array->getOperand(Index));
        assert(Struct->getOperand(2) == LowRawNull);
        Function* Ctor = cast<Function>(Struct->getOperand(1));
        Function* NewF = Function::Create(
          CtorDtorTy, GlobalValue::PrivateLinkage, 0, "deluge_ctor_forwarder", &M);
        BasicBlock* RootBB = BasicBlock::Create(C, "deluge_ctor_forwarder_root", NewF);
        CallInst::Create(DeferOrRunGlobalCtor, { Ctor }, "", RootBB);
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
        Function* NewF = Function::Create(
          CtorDtorTy, GlobalValue::PrivateLinkage, 0, "deluge_dtor_forwarder", &M);
        BasicBlock* RootBB = BasicBlock::Create(C, "deluge_dtor_forwarder_root", NewF);
        AllocaInst* ReturnBuffer = new AllocaInst(
          Int8Ty, 0, ConstantInt::get(IntPtrTy, 16), "deluge_dtor_return", RootBB);
        Instruction* Upper = GetElementPtrInst::Create(
          Int8Ty, ReturnBuffer, ConstantInt::get(IntPtrTy, 16), "deluge_dtor_return_upper", RootBB);
        CallInst::Create(
          DeludedFuncTy, Dtor,
          { LowRawNull, LowRawNull, LowRawNull, ReturnBuffer, Upper, IntTypeRep },
          "", RootBB);
        ReturnInst::Create(C, RootBB);
        Args.push_back(ConstantStruct::get(Struct->getType(), Struct->getOperand(0), NewF, LowRawNull));
      }
      GlobalDtors->setInitializer(ConstantArray::get(Array->getType(), Args));
    }

    auto FixupTypes = [&] (GlobalValue* G, GlobalValue* NewG) {
      GlobalHighTypes[NewG] = GlobalHighTypes[G];
      GlobalLowTypes[NewG] = GlobalLowTypes[G];
    };

    std::vector<GlobalValue*> ToDelete;
    auto HandleGlobal = [&] (GlobalValue* G) {
      Function* NewF = Function::Create(GlobalGetterTy, G->getLinkage(), G->getAddressSpace(),
                                        "deluded_g_" + G->getName(), &M);
      GlobalToGetter[G] = NewF;
      Getters.insert(NewF);
      FixupTypes(G, NewF);
      ToDelete.push_back(G);
    };
    for (GlobalVariable* G : Globals) {
      if (!G->isDeclaration()) {
        Type* T = G->getValueType();
        Type* LowT = lowerType(T);
        // FIXME: We could totally mark these constant if we know that there's no pointer fixup!
        GlobalToGlobal[G] = new GlobalVariable(
          M, LowT, false, GlobalValue::PrivateLinkage, UndefValue::get(LowT),
          "deluge_hidden_global");
      }
      HandleGlobal(G);
    }
    for (GlobalAlias* G : Aliases) {
      if (isa<GlobalVariable>(G->getAliasee()))
        HandleGlobal(G);
    }
    
    for (GlobalVariable* G : Globals) {
      // FIXME: We can probably make this work, but who gives a shit for now?
      assert(G->getThreadLocalMode() == GlobalValue::NotThreadLocal);

      Function* NewF = GlobalToGetter[G];
      assert(NewF);
      assert(NewF->isDeclaration());

      if (verbose)
        errs() << "Dealing with global: " << *G << "\n";

      if (G->isDeclaration())
        continue;

      Type* T = G->getValueType();
      Type* LowT = lowerType(T);

      // FIXME: What if we're dealing with an array? Right now, we'll create a type that is O(array size).
      // We could at least detect repeats here.
      DelugeTypeData* DTD = dataForLowType(LowT);
      
      GlobalVariable* NewDataG = GlobalToGlobal[G];
      assert(NewDataG);
      Constant* NewC = tryLowerConstantToConstant(
        G->getInitializer(), ResultMode::NeedConstantWithPtrPlaceholders);
      assert(NewC);
      NewDataG->setInitializer(NewC);
      
      BasicBlock* RootBB = BasicBlock::Create(C, "deluge_global_getter_root", NewF);

      GlobalVariable* NewPtrG = new GlobalVariable(
        M, Int128Ty, false, GlobalValue::PrivateLinkage, ConstantInt::get(Int128Ty, 0),
        "deluge_gptr_" + G->getName());
      
      BasicBlock* FastBB = BasicBlock::Create(C, "deluge_global_getter_fast", NewF);
      BasicBlock* SlowBB = BasicBlock::Create(C, "deluge_global_getter_slow", NewF);
      BasicBlock* RecurseBB = BasicBlock::Create(C, "deluge_global_getter_recurse", NewF);
      BasicBlock* BuildBB = BasicBlock::Create(C, "deluge_global_getter_build", NewF);

      Instruction* Branch = BranchInst::Create(SlowBB, FastBB, UndefValue::get(Int1Ty), RootBB);
      Value* LoadPtr = loadPtrPart(NewPtrG, Branch);
      Branch->getOperandUse(0) = new ICmpInst(
        Branch, ICmpInst::ICMP_EQ, LoadPtr, ConstantInt::get(Int128Ty, 0), "deluge_check_global");

      ReturnInst::Create(C, LoadPtr, FastBB);

      Branch = BranchInst::Create(BuildBB, RecurseBB, UndefValue::get(Int1Ty), SlowBB);
      Instruction* MyInitializationContext = CallInst::Create(
        GlobalInitializationContextCreate, { NewF->getArg(0) }, "deluge_context_create", Branch);
      Value* TypeRep = getTypeRepWithoutConstantPool(DTD, Branch);
      Instruction* Capability = CallInst::Create(
        NewCapability, { NewDataG, ConstantInt::get(IntPtrTy, DL.getTypeStoreSize(LowT)), TypeRep },
        "deluge_new_capability", Branch);
      Instruction* Add = CallInst::Create(
        GlobalInitializationContextAdd, { MyInitializationContext, NewPtrG, Capability },
        "deluge_context_add", Branch);
      Branch->getOperandUse(0) = Add;

      CallInst::Create(
        GlobalInitializationContextDestroy, { MyInitializationContext }, "", RecurseBB);
      ReturnInst::Create(C, Capability, RecurseBB);

      assert(DTD->Type.Main.Size);
      Instruction* Return = ReturnInst::Create(C, Capability, BuildBB);
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
            M, AT, true, GlobalVariable::PrivateLinkage, CA, "deluge_constant_relocations");
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
      if (F->isIntrinsic() ||
          F == ZunsafeForgeImpl ||
          F == ZrestrictImpl ||
          F == ZallocImpl ||
          F == ZallocFlexImpl ||
          F == ZalignedAllocImpl ||
          F == ZreallocImpl ||
          F == ZhardAllocImpl ||
          F == ZhardAllocFlexImpl ||
          F == ZhardAlignedAllocImpl ||
          F == ZhardReallocImpl ||
          F == ZtypeofImpl)
        continue;
      
      if (verbose)
        errs() << "Function before lowering: " << *F << "\n";

      FunctionName = F->getName();
      OldF = F;
      NewF = Function::Create(cast<FunctionType>(lowerType(F->getFunctionType())),
                              F->getLinkage(), F->getAddressSpace(),
                              "deluded_f_" + F->getName(), &M);
      FixupTypes(F, NewF);
      std::vector<BasicBlock*> Blocks;
      for (BasicBlock& BB : *F)
        Blocks.push_back(&BB);
      if (!Blocks.empty()) {
        assert(!LocalConstantPoolPtr);
        assert(!FutureIntFrame->getNumUses());
        assert(!FutureTypedFrame->getNumUses());
        IntFrameSize = 0;
        IntFrameAlignment = 1;
        TypedFrameType = CoreDelugeType();
        ReturnBufferSize = 0;
        ReturnBufferAlignment = 0;
        Args.clear();
        for (BasicBlock* BB : Blocks) {
          BB->removeFromParent();
          BB->insertInto(NewF);
        }
        // Snapshot the instructions before we do crazy stuff.
        std::vector<Instruction*> Instructions;
        for (BasicBlock* BB : Blocks) {
          for (Instruction& I : *BB) {
            Instructions.push_back(&I);
            captureTypesIfNecessary(&I);
          }
        }

        ReturnB = BasicBlock::Create(C, "deluge_return_block", NewF);
        if (F->getReturnType() != VoidTy)
          ReturnPhi = PHINode::Create(lowerType(F->getReturnType()), 1, "deluge_return_value", ReturnB);
        ReturnInst* Return = ReturnInst::Create(C, ReturnB);

        if (F->getReturnType() != VoidTy) {
          Value* ReturnDataPtr = forgePtrWithTypeRepAndUpper(
            NewF->getArg(3), NewF->getArg(4), NewF->getArg(5), Return);
          new StoreInst(
            ReturnPhi, prepareForAccess(lowerType(F->getReturnType()), ReturnDataPtr, Return), Return);
        }

        Instruction* InsertionPoint = &*Blocks[0]->getFirstInsertionPt();
        // FIXME: OMG this should happen after inlining. But whatever, we don't give a shit about
        // perf for the most part.
        Instruction* FastConstantPoolPtr = new LoadInst(
          LowRawPtrTy, GlobalConstantPoolPtr, "deluge_load_contantpool", InsertionPoint);
        Instruction* NewBlockTerm = SplitBlockAndInsertIfThen(
          new ICmpInst(
            InsertionPoint, ICmpInst::ICMP_EQ, FastConstantPoolPtr, LowRawNull,
            "deluge_check_constantpool"),
          InsertionPoint, false);
        Instruction* SlowConstantPoolPtr = CallInst::Create(
          MakeConstantPool, { }, "deluge_call_make_constantpool", NewBlockTerm);
        LocalConstantPoolPtr = PHINode::Create(LowRawPtrTy, 2, "deluge_constantpool", InsertionPoint);
        LocalConstantPoolPtr->addIncoming(FastConstantPoolPtr, Blocks[0]);
        LocalConstantPoolPtr->addIncoming(SlowConstantPoolPtr, SlowConstantPoolPtr->getParent());

        ArgBufferPtr = forgePtrWithTypeRepAndUpper(
          NewF->getArg(0), NewF->getArg(1), NewF->getArg(2), InsertionPoint);
        Value* RawDataPtr = lowerPtr(ArgBufferPtr, InsertionPoint);
        size_t ArgOffset = 0;
        for (unsigned Index = 0; Index < F->getFunctionType()->getNumParams(); ++Index) {
          Type* T = F->getFunctionType()->getParamType(Index);
          Type* LowT = lowerType(T);
          size_t Size = DL.getTypeStoreSize(LowT);
          size_t Alignment = DL.getABITypeAlign(LowT).value();
          ArgOffset = (ArgOffset + Alignment - 1) / Alignment * Alignment;
          Instruction* ArgPtr = GetElementPtrInst::Create(
            Int8Ty, RawDataPtr, { ConstantInt::get(IntPtrTy, ArgOffset) }, "deluge_arg_ptr",
            InsertionPoint);
          Value* V = loadValueRecurse(
            LowT, ArgBufferPtr, ArgPtr, false, DL.getABITypeAlign(LowT), AtomicOrdering::NotAtomic,
            SyncScope::System, InsertionPoint);
          Args.push_back(V);
          ArgOffset += Size;
        }
        Instruction* ArgEndPtr = GetElementPtrInst::Create(
          Int8Ty, RawDataPtr, { ConstantInt::get(IntPtrTy, ArgOffset) }, "deluge_arg_end_ptr",
          InsertionPoint);
        ArgBufferPtr = reforgePtr(ArgBufferPtr, ArgEndPtr, InsertionPoint);
        CallInst::Create(Deallocate, { NewF->getArg(0) }, "", Return);

        FirstRealBlock = InsertionPoint->getParent();
        
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
              "deluge_return_buffer", InsertionPoint));
        } else {
          assert(!ReturnBufferSize);
          assert(!ReturnBufferAlignment);
        }

        if (FutureAllocaStack->hasNUsesOrMore(1)) {
          AllocaInst* AllocaStack =
            new AllocaInst(AllocaStackTy, 0, "deluge_alloca_bag_alloca", InsertionPoint);
          CallInst::Create(
            RealMemset,
            { AllocaStack, ConstantInt::get(Int8Ty, 0),
              ConstantInt::get(IntPtrTy, DL.getTypeStoreSize(AllocaStackTy)),
              ConstantInt::get(Int1Ty, false) },
            "", InsertionPoint);
          FutureAllocaStack->replaceAllUsesWith(AllocaStack);
          CallInst::Create(AllocaStackDestroy, { AllocaStack }, "", Return);
        }

        InsertionPoint = &*FirstRealBlock->getFirstInsertionPt();
        
        if (IntFrameSize) {
          Instruction* AllocateIntFrame;
          if (IntFrameAlignment > MinAlign) {
            AllocateIntFrame = CallInst::Create(
              AllocateIntWithAlignment,
              { ConstantInt::get(IntPtrTy, IntFrameSize), ConstantInt::get(IntPtrTy, 1),
                ConstantInt::get(IntPtrTy, IntFrameAlignment) },
              "deluge_allocate_int_frame", InsertionPoint);
          } else {
            AllocateIntFrame = CallInst::Create(
              AllocateInt,
              { ConstantInt::get(IntPtrTy, IntFrameSize), ConstantInt::get(IntPtrTy, 1) },
              "deluge_allocate_int_frame", InsertionPoint);
          }
          FutureIntFrame->replaceAllUsesWith(AllocateIntFrame);
          CallInst::Create(Deallocate, { AllocateIntFrame }, "", Return);
        } else
          assert(!FutureIntFrame->getNumUses());

        if (TypedFrameType.isValid()) {
          DelugeType FullFrameType;
          FullFrameType.Main = TypedFrameType;
          DelugeTypeData* DTD = dataForType(FullFrameType);
          Instruction* AllocateTypedFrame = CallInst::Create(
            AllocateOne, { getHeap(DTD, InsertionPoint, ConstantPoolEntryKind::Heap) },
            "deluge_allocate_typed_frame", InsertionPoint);
          FutureTypedFrame->replaceAllUsesWith(AllocateTypedFrame);
          CallInst::Create(Deallocate, { AllocateTypedFrame }, "", Return);
        } else
          assert(!FutureTypedFrame->getNumUses());
      }
      
      NewF->copyAttributesFrom(F);
      NewF->setAttributes(AttributeList());
      F->replaceAllUsesWith(NewF);
      F->eraseFromParent();
      
      FunctionName = "<internal>";

      LocalConstantPoolPtr = nullptr;
      
      if (verbose)
        errs() << "New function: " << *NewF << "\n";
    }
    for (GlobalAlias* G : Aliases) {
      Constant* C = G->getAliasee();
      if (Function* TargetF = dyn_cast<Function>(C)) {
        Function* NewF = Function::Create(DeludedFuncTy, G->getLinkage(), G->getAddressSpace(),
                                          "deluded_f_" + G->getName(), &M);
        BasicBlock* BB = BasicBlock::Create(this->C, "deluge_alias_call", NewF);
        CallInst::Create(
          DeludedFuncTy, TargetF,
          { NewF->getArg(0), NewF->getArg(1), NewF->getArg(2), NewF->getArg(3), NewF->getArg(4),
            NewF->getArg(5) },
          "", BB);
        ReturnInst::Create(this->C, BB);
        FixupTypes(G, NewF);
        G->replaceAllUsesWith(NewF);
        G->eraseFromParent();
        continue;
      }

      if (GlobalVariable* GV = dyn_cast<GlobalVariable>(C)) {
        Function* NewF = GlobalToGetter[G];
        Function* TargetF = GlobalToGetter[GV];
        assert(NewF);
        assert(TargetF);
        BasicBlock* BB = BasicBlock::Create(this->C, "deluge_alias_global", NewF);
        ReturnInst::Create(
          this->C,
          CallInst::Create(GlobalGetterTy, TargetF, { NewF->getArg(0) }, "deluge_forward_global", BB),
          BB);
        continue;
      }
      
      llvm_unreachable("don't know what to do with global aliases yet");
      // FIXME: The GlobalAlias constant expression may produce something that is not at all a valid
      // pointer. It's not at all clear that we get the right behavior here. Probably, we want there to
      // be a compile-time or runtime check that we're producing a pointer that makes sense with a type
      // that makes sense.
      GlobalAlias* NewG = GlobalAlias::create(lowerType(G->getValueType()), G->getAddressSpace(),
                                              G->getLinkage(), "deluded_a_" + G->getName(),
                                              G->getAliasee(), &M);
      FixupTypes(G, NewG);
      NewG->copyAttributesFrom(G);
      G->replaceAllUsesWith(NewG);
      G->eraseFromParent();
    }
    for (GlobalIFunc* G : IFuncs) {
      llvm_unreachable("don't know what to do with global ifuncs yet");
      GlobalIFunc* NewG = GlobalIFunc::create(lowerType(G->getValueType()), G->getAddressSpace(),
                                              G->getLinkage(), "deluded_i_" + G->getName(),
                                              G->getResolver(), &M);
      FixupTypes(G, NewG);
      NewG->copyAttributesFrom(G);
      G->replaceAllUsesWith(NewG);
      G->eraseFromParent();
    }

    // Kill the remnants of the original globals.
    for (GlobalVariable* G : Globals)
      G->setLinkage(GlobalValue::PrivateLinkage);

    Function* MakeConstantPoolFunc = cast<Function>(MakeConstantPool.getCallee());
    MakeConstantPoolFunc->setLinkage(GlobalValue::PrivateLinkage);
    BasicBlock* BB = BasicBlock::Create(C, "deluge_make_constantpool_block", MakeConstantPoolFunc);
    for (auto& pair : TypeDatas) {
      CallInst::Create(ValidateType, {
          CallInst::Create(GetType, { pair.second->TemplateRep }, "", BB),
          LowRawNull
        }, "", BB);
    }
    Instruction* ConstantPoolPtr = CallInst::Create(
      AllocateUtility, { ConstantInt::get(IntPtrTy, 8 * ConstantPoolEntries.size()) },
      "deluge_allocate_constantpool", BB);
    for (size_t Index = ConstantPoolEntries.size(); Index--;) {
      Instruction* EntryPtr = GetElementPtrInst::Create(
        LowRawPtrTy, ConstantPoolPtr, { ConstantInt::get(IntPtrTy, Index) },
        "deluge_constantpool_entry_ptr", BB);
      const ConstantPoolEntry& CPE = ConstantPoolEntries[Index];
      Instruction* Type = CallInst::Create(GetType, { CPE.DTD->TemplateRep }, "deluge_get_type", BB);
      switch (CPE.Kind) {
      case ConstantPoolEntryKind::Type: {
        new StoreInst(Type, EntryPtr, BB);
        break;
      }
      case ConstantPoolEntryKind::Heap: {
        Instruction* Heap = CallInst::Create(GetHeap, { Type }, "deluge_get_heap", BB);
        new StoreInst(Heap, EntryPtr, BB);
        break;
      }
      case ConstantPoolEntryKind::HardHeap: {
        Instruction* Heap = CallInst::Create(GetHardHeap, { Type }, "deluge_get_heap", BB);
        new StoreInst(Heap, EntryPtr, BB);
        break;
      } }
    }
    new StoreInst(ConstantPoolPtr, GlobalConstantPoolPtr, BB);
    new FenceInst(C, AtomicOrdering::AcquireRelease, SyncScope::System, BB);
    ReturnInst::Create(C, ConstantPoolPtr, BB);

    delete Dummy;
    delete FutureIntFrame;
    delete FutureTypedFrame;
    delete FutureReturnBuffer;
    delete FutureAllocaStack;

    for (GlobalValue* G : ToDelete)
      G->replaceAllUsesWith(UndefValue::get(G->getType())); // FIXME - should be zero
    for (GlobalValue* G : ToDelete)
      G->eraseFromParent();
    
    if (verbose)
      errs() << "Here's the deluded module:\n" << M << "\n";
    verifyModule(M);
  }
};

} // anonymous namespace

PreservedAnalyses DelugePass::run(Module &M, ModuleAnalysisManager &MAM) {
  Deluge D(M, MAM);
  D.run();
  return PreservedAnalyses::none();
}

