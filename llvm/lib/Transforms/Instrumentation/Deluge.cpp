#include "llvm/Transforms/Instrumentation/Deluge.h"

#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/TypedPointerType.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/TargetParser/Triple.h>
#include <vector>
#include <unordered_map>

using namespace llvm;

namespace {

static constexpr bool verbose = false;
static constexpr bool ultraVerbose = false;

static constexpr size_t MinAlign = 16;

// This has to match the Deluge runtime.
enum class DelugeWordType {
  OffLimits = 0,
  Int = 1,
  PtrPart1 = 2,
  PtrPart2 = 3,
  PtrPart3 = 4,
  PtrPart4 = 5,
  Function = 6,
  Type = 7,
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
    if (Size % 8) {
      assert(!WordTypes.empty());
      assert(WordTypes.back() == DelugeWordType::Int);
    }
    Size = (Size + Alignment - 1) / Alignment * Alignment;
    while ((Size + 7) / 8 > WordTypes.size())
      WordTypes.push_back(DelugeWordType::OffLimits);
  }

  // Append two types to each other. Does not work for special types like int or func. Returns the
  // offset that the Other type was appended at.
  size_t append(const CoreDelugeType& Other) {
    assert(!(Size % 8));
    assert(!(Other.Size % 8));

    Alignment = std::max(Alignment, Other.Alignment);

    pad(Alignment);
    size_t Result = Size;
    WordTypes.insert(WordTypes.end(), Other.WordTypes.begin(), Other.WordTypes.end());
    Size += Other.Size;
    return Result;
  }
};

struct DelugeType {
  CoreDelugeType Main;
  CoreDelugeType Trailing;

  DelugeType() = default;
  
  DelugeType(size_t Size, size_t Alignment)
    : Main(Size, Alignment) {
  }

  bool canBeInt() const { return Main.canBeInt() && !Trailing.isValid(); }

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
  Constant* TypeRep { nullptr };
};

enum ConstantPoolEntryKind {
  Heap
};

struct ConstantPoolEntry {
  ConstantPoolEntryKind Kind;
  union {
    struct {
      DelugeTypeData* DTD;
    } Heap;
  } u;

  bool operator==(const ConstantPoolEntry& Other) const {
    if (Kind != Other.Kind)
      return false;
    switch (Kind) {
    case ConstantPoolEntryKind::Heap:
      return u.Heap.DTD == Other.u.Heap.DTD;
    }
    llvm_unreachable("bad kind");
    return false;
  }

  size_t hash() const {
    size_t Result = static_cast<size_t>(Kind) * 666;
    switch (Kind) {
    case ConstantPoolEntryKind::Heap:
      return Result + reinterpret_cast<size_t>(u.Heap.DTD);
    }
    llvm_unreachable("bad kind");
    return 0;
  }
};

} // anonymous namespace

template<> struct std::hash<ConstantPoolEntry> {
  size_t operator()(const ConstantPoolEntry& Key) const {
    return Key.hash();
  }
};

namespace {

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
  PointerType* LowRawPtrTy;
  StructType* LowWidePtrTy;
  StructType* OriginTy;
  StructType* GlobalInitializationContextTy;
  FunctionType* DeludedFuncTy;
  FunctionType* GlobalGetterTy;
  Constant* LowRawNull;
  Constant* LowWideNull;
  BitCastInst* Dummy;

  // High-level functions available to the user.
  Value* ZunsafeForgeImpl;
  Value* ZrestrictImpl;
  Value* ZallocImpl;
  Value* ZreallocImpl;

  // Low-level functions used by codegen.
  FunctionCallee GetHeap;
  FunctionCallee TryAllocateInt;
  FunctionCallee TryAllocateIntWithAlignment;
  FunctionCallee AllocateInt;
  FunctionCallee AllocateIntWithAlignment;
  FunctionCallee TryAllocateOne;
  FunctionCallee AllocateOne;
  FunctionCallee TryAllocateMany;
  FunctionCallee AllocateUtility;
  FunctionCallee TryReallocateInt;
  FunctionCallee TryReallocateIntWithAlignment;
  FunctionCallee TryReallocate;
  FunctionCallee Deallocate;
  FunctionCallee CheckAccessInt;
  FunctionCallee CheckAccessPtr;
  FunctionCallee CheckAccessFunctionCall;
  FunctionCallee Memset;
  FunctionCallee Memcpy;
  FunctionCallee Memmove;
  FunctionCallee CheckRestrict;
  FunctionCallee VAArgImpl;
  FunctionCallee GlobalInitializationContextFind;
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
  DelugeTypeData Invalid;

  std::unordered_map<GlobalVariable*, Function*> GlobalToGetter;
  std::unordered_map<Function*, GlobalVariable*> GetterToGlobal;

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

    assert((CDT.Size + 7) / 8 == CDT.WordTypes.size());
    if (CDT.Size % 8) {
      assert(!CDT.WordTypes.empty());
      assert(CDT.WordTypes.back() == DelugeWordType::Int);
    }

    auto Fill = [&] () {
      while ((CDT.Size + 7) / 8 > CDT.WordTypes.size())
        CDT.WordTypes.push_back(DelugeWordType::Int);
    };

    assert(T != LowRawPtrTy);
    
    if (T == LowWidePtrTy) {
      CDT.Size += 32;
      CDT.WordTypes.push_back(DelugeWordType::PtrPart1);
      CDT.WordTypes.push_back(DelugeWordType::PtrPart2);
      CDT.WordTypes.push_back(DelugeWordType::PtrPart3);
      CDT.WordTypes.push_back(DelugeWordType::PtrPart4);
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
    while ((CDT.Size + 7) / 8 > CDT.WordTypes.size())
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
    assert(!(T.Main.Size % T.Main.Alignment));

    std::vector<Constant*> Constants;
    Constants.push_back(ConstantInt::get(IntPtrTy, T.Main.Size));
    Constants.push_back(ConstantInt::get(IntPtrTy, T.Main.Alignment));
    if (TrailingData)
      Constants.push_back(ConstantExpr::getPtrToInt(TrailingData->TypeRep, IntPtrTy));
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
    Data.TypeRep = new GlobalVariable(M, AT, true, GlobalValue::PrivateLinkage, CA, "deluge_type");
  }

  DelugeTypeData* dataForType(const DelugeType& T) {
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

  // This happens to work just as well for high types and low types.
  bool hasPtrsForCheck(Type *T) {
    if (FunctionType* FT = dyn_cast<FunctionType>(T)) {
      llvm_unreachable("shouldn't see function types in hasPtrsForCheck");
      return false;
    }

    if (isa<TypedPointerType>(T)) {
      llvm_unreachable("Shouldn't ever see typed pointers");
      return false;
    }

    if (isa<PointerType>(T))
      return T->getPointerAddressSpace() == TargetAS;

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
    Instruction* Result = ExtractValueInst::Create(
      LowRawPtrTy, HighP, { 0 }, "deluge_getlowptr", InsertBefore);
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

  Constant* forgePtrConstant(Constant* Ptr, Constant* Lower, Constant* Upper, Constant* TypeRep) {
    return ConstantStruct::get(LowWidePtrTy, { Ptr, Lower, Upper, TypeRep });
  }

  Constant* forgePtrConstantWithLowType(Constant* Ptr, Constant* Lower, Constant* Upper, Type* LowT) {
    return forgePtrConstant(Ptr, Lower, Upper, dataForLowType(LowT)->TypeRep);
  }

  Constant* forgePtrConstantWithLowType(Constant* Ptr, Type* LowT) {
    if (isa<FunctionType>(LowT)) {
      return forgePtrConstant(
        Ptr, Ptr, ConstantExpr::getGetElementPtr(Int8Ty, Ptr, ConstantInt::get(IntPtrTy, 1)),
        FunctionDTD.TypeRep);
    }
    return forgePtrConstantWithLowType(
      Ptr, Ptr, ConstantExpr::getGetElementPtr(LowT, Ptr, ConstantInt::get(IntPtrTy, 1)), LowT);
  }

  Value* forgePtr(Value* Ptr, Value* Lower, Value* Upper, Value* TypeRep, Instruction* InsertionPoint) {
    Instruction* Result = InsertValueInst::Create(
      UndefValue::get(LowWidePtrTy), Ptr, { 0 }, "deluge_forge_ptr", InsertionPoint);
    Result->setDebugLoc(InsertionPoint->getDebugLoc());
    Result = InsertValueInst::Create(Result, Lower, { 1 }, "deluge_forge_lower", InsertionPoint);
    Result->setDebugLoc(InsertionPoint->getDebugLoc());
    Result = InsertValueInst::Create(Result, Upper, { 2 }, "deluge_forge_upper", InsertionPoint);
    Result->setDebugLoc(InsertionPoint->getDebugLoc());
    Result = InsertValueInst::Create(Result, TypeRep, { 3 }, "deluge_forge_type", InsertionPoint);
    Result->setDebugLoc(InsertionPoint->getDebugLoc());
    return Result;
  }

  Value* forgePtrWithLowType(Value* Ptr, Value* Lower, Value* Upper, Type* LowT, Instruction* InsertionPoint) {
    return forgePtr(Ptr, Lower, Upper, dataForLowType(LowT)->TypeRep, InsertionPoint);
  }

  Value* forgePtrWithLowType(Value* Ptr, Type* LowT, Instruction* InsertionPoint) {
    if (isa<FunctionType>(LowT))
      return forgePtr(Ptr, LowRawNull, LowRawNull, FunctionDTD.TypeRep, InsertionPoint);
    Instruction* Upper = GetElementPtrInst::Create(LowT, Ptr, { ConstantInt::get(IntPtrTy, 1) }, "deluge_upper", InsertionPoint);
    Upper->setDebugLoc(InsertionPoint->getDebugLoc());
    return forgePtrWithLowType(Ptr, Ptr, Upper, LowT, InsertionPoint);
  }

  Constant* reforgePtrConstant(Constant* LowWidePtr, Constant* NewLowRawPtr) {
    if (verbose)
      errs() << "LowWidePtr = " << *LowWidePtr << ", NewLowRawPtr = " << *NewLowRawPtr << "\n";
    if (isa<ConstantAggregateZero>(LowWidePtr))
      return forgePtrConstant(NewLowRawPtr, LowRawNull, LowRawNull, LowRawNull);
    ConstantStruct* CS = cast<ConstantStruct>(LowWidePtr);
    assert(CS->getNumOperands() == 4);
    assert(CS->getType() == LowWidePtrTy);
    return forgePtrConstant(NewLowRawPtr, CS->getOperand(1), CS->getOperand(2), CS->getOperand(3));
  }

  Value* reforgePtr(Value* LowWidePtr, Value* NewLowRawPtr, Instruction* InsertionPoint) {
    Instruction* Result = InsertValueInst::Create(
      LowWidePtr, NewLowRawPtr, { 0 }, "deluge_reforge", InsertionPoint);
    Result->setDebugLoc(InsertionPoint->getDebugLoc());
    return Result;
  }

  Value* forgeBadPtr(Value* Ptr, Instruction* InsertionPoint) {
    return reforgePtr(LowWideNull, Ptr, InsertionPoint);
  }

  Value* lowerConstant(Constant* C, Instruction* InsertBefore, Value* InitializationContext) {
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
      Type* LowT = GlobalLowTypes[G];
      assert(!GlobalToGetter.count(nullptr));
      assert(!GetterToGlobal.count(nullptr));
      if (GlobalToGetter.count(dyn_cast<GlobalVariable>(G)) ||
          GetterToGlobal.count(dyn_cast<Function>(G))) {
        Function* Getter;
        if (!(Getter = dyn_cast<Function>(G)))
          Getter = GlobalToGetter[cast<GlobalVariable>(G)];
        assert(Getter);
        Instruction* Result = CallInst::Create(
          GlobalGetterTy, Getter, { InitializationContext }, "deluge_call_getter", InsertBefore);
        Result->setDebugLoc(InsertBefore->getDebugLoc());
        return Result;
      }
      assert(isa<Function>(G));
      return forgePtrConstantWithLowType(G, LowT);
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
          Result, LowC, static_cast<unsigned>(Index), "deluge_insert_array", InsertBefore);
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
          ConstantInt::get(IntPtrTy, Index), "deluge_insert_array", InsertBefore);
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
    lowerInstruction(CEInst);
    Value* Result = DummyUser->getOperand(0);
    delete DummyUser;
    return Result;
  }

  Value* makeIntPtr(Value* V, Instruction *InsertionPoint) {
    if (V->getType() == IntPtrTy)
      return V;
    Instruction* Result =
      CastInst::CreateIntegerCast(V, IntPtrTy, false, "deluge_makeintptr", InsertionPoint);
    Result->setDebugLoc(InsertionPoint->getDebugLoc());
    return Result;
  }

  template<typename Func>
  void hackRAUW(Value* V, const Func& GetNewValue) {
    assert(!Dummy->getNumUses());
    V->replaceAllUsesWith(Dummy);
    Dummy->replaceAllUsesWith(GetNewValue());
  }

  Value* getHeap(DelugeTypeData* DTD, Instruction* InsertBefore) {
    ConstantPoolEntry GTE;
    GTE.Kind = ConstantPoolEntryKind::Heap;
    GTE.u.Heap.DTD = DTD;
    auto iter = ConstantPoolEntryIndex.find(GTE);
    size_t Index;
    if (iter != ConstantPoolEntryIndex.end())
      Index = iter->second;
    else {
      Index = ConstantPoolEntries.size();
      ConstantPoolEntryIndex[GTE] = Index;
      ConstantPoolEntries.push_back(GTE);
    }
    Instruction* EntryPtr = GetElementPtrInst::Create(
      LowRawPtrTy, LocalConstantPoolPtr, { ConstantInt::get(IntPtrTy, Index) },
      "deluge_constantpool_entry_ptr", InsertBefore);
    EntryPtr->setDebugLoc(InsertBefore->getDebugLoc());
    Instruction* Result = new LoadInst(
      LowRawPtrTy, EntryPtr, "deluge_constantpool_heap_load", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
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

  bool earlyLowerInstruction(Instruction* I) {
    if (verbose)
      errs() << "Early lowering: " << *I << "\n";

    // FIXME: Anywhere this uses operands, it must do the operand lowering thing that happens at the
    // top of lowerInstruction().

    if (IntrinsicInst* II = dyn_cast<IntrinsicInst>(I)) {
      if (verbose)
        errs() << "It's an intrinsic.\n";
      switch (II->getIntrinsicID()) {
      case Intrinsic::memcpy:
      case Intrinsic::memcpy_inline:
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
        if (hasPtrsForCheck(II->getArgOperand(0)->getType())) {
          Instruction* CI = CallInst::Create(
            Memset,
            { II->getArgOperand(0), II->getArgOperand(1), makeIntPtr(II->getArgOperand(2), II),
              getOrigin(II->getDebugLoc()) });
          ReplaceInstWithInst(II, CI);
        }
        return true;
      case Intrinsic::memmove:
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
        // annotations.
        II->eraseFromParent();
        return true;

      case Intrinsic::vastart:
        checkPtr(II->getArgOperand(0), II);
        (new StoreInst(ArgBufferPtr, lowerPtr(II->getArgOperand(0), II), II))
          ->setDebugLoc(II->getDebugLoc());
        II->eraseFromParent();
        return true;
        
      case Intrinsic::vacopy: {
        checkPtr(II->getArgOperand(0), II);
        checkPtr(II->getArgOperand(1), II);
        Instruction* Load = new LoadInst(
          LowWidePtrTy, lowerPtr(II->getArgOperand(1), II), "deluge_vacopy_load", II);
        Load->setDebugLoc(II->getDebugLoc());
        new StoreInst(Load, lowerPtr(II->getArgOperand(0), II), II);
        II->eraseFromParent();
        return true;
      }
        
      case Intrinsic::vaend:
        II->eraseFromParent();
        return true;

      default:
        if (!II->getCalledFunction()->doesNotAccessMemory()) {
          if (verbose)
            llvm::errs() << "Unhandled intrinsic: " << *II << "\n";
          CallInst::Create(Error, "", II)->setDebugLoc(II->getDebugLoc());
        }
        for (Use& U : II->data_ops()) {
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
      
      if (CI->getCalledOperand() == ZunsafeForgeImpl) {
        if (verbose)
          errs() << "Lowering unsafe forge\n";
        Type* HighT = cast<AllocaInst>(CI->getArgOperand(1))->getAllocatedType();
        Type* LowT = lowerType(HighT);
        Value* LowPtr = lowerPtr(CI->getArgOperand(0), CI);
        CI->replaceAllUsesWith(
          forgePtrWithLowType(
            LowPtr, LowPtr,
            GetElementPtrInst::Create(
              LowT, LowPtr, { ConstantInt::get(IntPtrTy, 1) }, "deluge_upper_unsafe_forge", CI),
            LowT, CI));
        CI->eraseFromParent();
        return true;
      }

      if (CI->getCalledOperand() == ZrestrictImpl) {
        if (verbose)
          errs() << "Lowering restrict\n";
        Type* HighT = cast<AllocaInst>(CI->getArgOperand(1))->getAllocatedType();
        Type* LowT = lowerType(HighT);
        Value* TypeRep = dataForLowType(LowT)->TypeRep;
        Value* LowPtr = lowerPtr(CI->getOperand(0), CI);
        Instruction* NewUpper = GetElementPtrInst::Create(
          LowT, LowPtr, { ConstantInt::get(IntPtrTy, 1) }, "deluge_NewUpper", CI);
        NewUpper->setDebugLoc(CI->getDebugLoc());
        CallInst::Create(
          CheckRestrict, { CI->getOperand(0), NewUpper, TypeRep, getOrigin(CI->getDebugLoc()) }, "", CI)
          ->setDebugLoc(CI->getDebugLoc());
        CI->replaceAllUsesWith(forgePtr(LowPtr, LowPtr, NewUpper, TypeRep, CI));
        CI->eraseFromParent();
        return true;
      }

      if (CI->getCalledOperand() == ZallocImpl) {
        if (verbose)
          errs() << "Lowering alloc\n";
        Type* HighT = cast<AllocaInst>(CI->getArgOperand(0))->getAllocatedType();
        Type* LowT = lowerType(HighT);
        assert(hasPtrsForCheck(HighT) == hasPtrsForCheck(LowT));
        Instruction* Alloc = nullptr;
        
        DelugeTypeData *DTD = dataForLowType(LowT);
        if (!hasPtrsForCheck(HighT)) {
          assert(DTD == &Int);
          size_t Alignment = DL.getABITypeAlign(LowT).value();
          size_t Size = DL.getTypeStoreSize(LowT);
          Instruction* Mul = BinaryOperator::CreateMul(
            CI->getArgOperand(1), ConstantInt::get(IntPtrTy, Size),
            "deluge_alloc_mul", CI);
          Mul->setDebugLoc(CI->getDebugLoc());
          if (Alignment > MinAlign) {
            Alloc = CallInst::Create(
              TryAllocateIntWithAlignment, { Mul, ConstantInt::get(IntPtrTy, Alignment) },
              "deluge_alloc_int", CI);
          } else
            Alloc = CallInst::Create(TryAllocateInt, { Mul }, "deluge_alloc_int", CI);
        } else {
          Value* Heap = getHeap(DTD, CI);
          if (Constant* C = dyn_cast<Constant>(CI->getArgOperand(1))) {
            if (C->isOneValue())
              Alloc = CallInst::Create(TryAllocateOne, { Heap }, "deluge_alloc_one", CI);
          }
          if (!Alloc) {
            Alloc = CallInst::Create(
              TryAllocateMany, { Heap, CI->getArgOperand(1) }, "deluge_alloc_many", CI);
          }
        }
        
        Alloc->setDebugLoc(CI->getDebugLoc());
        Instruction* Upper = GetElementPtrInst::Create(
          LowT, Alloc, { CI->getArgOperand(1) }, "deluge_alloc_upper", CI);
        Upper->setDebugLoc(CI->getDebugLoc());
        CI->replaceAllUsesWith(forgePtr(Alloc, Alloc, Upper, DTD->TypeRep, CI));
        CI->eraseFromParent();
        return true;
      }

      if (CI->getCalledOperand() == ZreallocImpl) {
        if (verbose)
          errs() << "Lowering realloc\n";
        Value* OrigPtr = lowerPtr(CI->getArgOperand(0), CI);
        Type* HighT = cast<AllocaInst>(CI->getArgOperand(1))->getAllocatedType();
        Type* LowT = lowerType(HighT);
        assert(hasPtrsForCheck(HighT) == hasPtrsForCheck(LowT));
        Instruction* Alloc = nullptr;
        
        DelugeTypeData *DTD = dataForLowType(LowT);
        if (!hasPtrsForCheck(HighT)) {
          assert(DTD == &Int);
          size_t Alignment = DL.getABITypeAlign(LowT).value();
          size_t Size = DL.getTypeStoreSize(LowT);
          Instruction* Mul = BinaryOperator::CreateMul(
            CI->getArgOperand(2), ConstantInt::get(IntPtrTy, Size),
            "deluge_alloc_mul", CI);
          Mul->setDebugLoc(CI->getDebugLoc());
          if (Alignment > MinAlign) {
            Alloc = CallInst::Create(
              TryReallocateIntWithAlignment, { OrigPtr, Mul, ConstantInt::get(IntPtrTy, Alignment) },
              "deluge_realloc_int", CI);
          } else
            Alloc = CallInst::Create(TryReallocateInt, { OrigPtr, Mul }, "deluge_realloc_int", CI);
        } else {
          Value* Heap = getHeap(DTD, CI);
          Alloc = CallInst::Create(
            TryReallocate, { OrigPtr, Heap, CI->getArgOperand(2) }, "deluge_realloc", CI);
        }
        
        Alloc->setDebugLoc(CI->getDebugLoc());
        Instruction* Upper = GetElementPtrInst::Create(
          LowT, Alloc, { CI->getArgOperand(2) }, "deluge_alloc_upper", CI);
        Upper->setDebugLoc(CI->getDebugLoc());
        CI->replaceAllUsesWith(forgePtr(Alloc, Alloc, Upper, DTD->TypeRep, CI));
        CI->eraseFromParent();
        return true;
      }
    }
    
    return false;
  }
  
  // FIXME: Eventually, global variables in a module will have some kind of checking associated with
  // them so that if one module uses a global variable defined in another, then you cannot get a type
  // confused wide pointer, with any combination of these things used as the enforcement mechanism:
  // - Accessing a global requires checking some side-state of the global that tells you the type
  //   that the global was really allocated with
  // - Extern globals are implemented as function calls that return a wide pointer with the right type.
  // - Runtime maintains a global registry of types associated with globals, and every module that
  //   knows of a global tells the runtime what it expects of the type. The runtime may then trap if
  //   it detects a contradiction.

  // This lowers the instruction "in place", so all references to it are fixed up after this runs.
  void lowerInstruction(Instruction *I) {
    if (verbose)
      errs() << "Lowering: " << *I << "\n";
    
    for (unsigned Index = I->getNumOperands(); Index--;) {
      Use& U = I->getOperandUse(Index);
      if (Constant* C = dyn_cast<Constant>(U)) {
        if (ultraVerbose)
          errs() << "At Index = " << Index << ", got U = " << *U << ", C = " << *C << "\n";
        Value* NewC = lowerConstant(C, I, LowRawNull);
        if (ultraVerbose)
          errs() << "At Index = " << Index << ", got NewC = " << *NewC <<"\n";
        U = NewC;
      } else if (Argument* A = dyn_cast<Argument>(U)) {
        if (ultraVerbose) {
          errs() << "A = " << *A << "\n";
          errs() << "A->getArgNo() == " << A->getArgNo() << "\n";
          errs() << "Args[A->getArgNo()] == " << *Args[A->getArgNo()] << "\n";
        }
        U = Args[A->getArgNo()];
      }
      if (ultraVerbose)
        errs() << "After Index = " << Index << ", I = " << *I << "\n";
    }
    
    if (verbose)
      errs() << "After arg lowering: " << *I << "\n";

    if (AllocaInst* AI = dyn_cast<AllocaInst>(I)) {
      assert(AI->getParent() == FirstRealBlock); // FIXME: We could totally support this.
      if (!AI->hasNUsesOrMore(1)) {
        // By this point we may have dead allocas, due to earlyLowerInstruction. Only happens for allocas
        // used as type hacks for stdfil API.
        return;
      }
      Type* LowT = lowerType(AI->getAllocatedType());
      Value* Base;
      size_t Offset;
      DelugeTypeData* DTD = dataForLowType(LowT);
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
      Instruction* Upper = GetElementPtrInst::Create(
        LowT, EntryPtr, { ConstantInt::get(IntPtrTy, 1) }, "deluge_alloca_upper", AI);
      Upper->setDebugLoc(AI->getDebugLoc());
      AI->replaceAllUsesWith(forgePtr(EntryPtr, EntryPtr, Upper, DTD->TypeRep, AI));
      AI->eraseFromParent();
      return;
    }

    if (LoadInst* LI = dyn_cast<LoadInst>(I)) {
      Type *LowT = lowerType(LI->getType());
      ReplaceInstWithInst(
        LI, new LoadInst(
          LowT, prepareForAccess(LowT, LI->getPointerOperand(), LI),
          LI->getName(), LI->isVolatile(), LI->getAlign(), LI->getOrdering(), LI->getSyncScopeID()));
      return;
    }

    if (StoreInst* SI = dyn_cast<StoreInst>(I)) {
      if (hasPtrsForCheck(SI->getPointerOperand()->getType())) {
        SI->getOperandUse(StoreInst::getPointerOperandIndex()) =
          prepareForAccess(InstLowTypes[SI], SI->getPointerOperand(), SI);
      }
      return;
    }

    if (FenceInst* FI = dyn_cast<FenceInst>(I)) {
      // We don't need to do anything because it doesn't take operands.
      return;
    }

    if (AtomicCmpXchgInst* AI = dyn_cast<AtomicCmpXchgInst>(I)) {
      if (hasPtrsForCheck(AI->getPointerOperand()->getType())) {
        AI->getOperandUse(AtomicCmpXchgInst::getPointerOperandIndex()) =
          prepareForAccess(InstLowTypes[AI], AI->getPointerOperand(), AI);
      }
      if (hasPtrsForCheck(InstLowTypes[AI]))
        llvm_unreachable("Cannot handle CAS on pointer field, sorry");
      return;
    }

    if (AtomicRMWInst* AI = dyn_cast<AtomicRMWInst>(I)) {
      if (hasPtrsForCheck(AI->getPointerOperand()->getType())) {
        AI->getOperandUse(AtomicRMWInst::getPointerOperandIndex()) =
          prepareForAccess(AI->getValOperand()->getType(), AI->getPointerOperand(), AI);
      }
      if (hasPtrsForCheck(InstLowTypes[AI]))
        llvm_unreachable("Cannot handle CAS on pointer field, sorry");
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
        isa<FPToSIInst>(I)) {
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
      if (CI->isInlineAsm())
        llvm_unreachable("Don't support InlineAsm, because that shit's not memory safe");

      if (verbose)
        errs() << "Dealing with called operand: " << *CI->getCalledOperand() << "\n";

      assert(CI->getCalledOperand() != ZunsafeForgeImpl);
      assert(CI->getCalledOperand() != ZrestrictImpl);
      assert(CI->getCalledOperand() != ZallocImpl);
      assert(CI->getCalledOperand() != ZreallocImpl);
      
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
                ConstantInt::get(IntPtrTy, ArgType.Main.Alignment) },
              "deluge_allocate_args", CI);
          } else {
            ArgBufferRawPtr = CallInst::Create(
              AllocateInt, { ConstantInt::get(IntPtrTy, ArgType.Main.Size) }, "deluge_allocate_args", CI);
          }
        } else {
          ArgDTD = dataForType(ArgType);
          ArgBufferRawPtr = CallInst::Create(
            AllocateOne, { getHeap(ArgDTD, CI) }, "deluge_allocate_args", CI);
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
          new StoreInst(Arg, ArgSlotPtr, CI);
        }
        ArgBufferRawPtrValue = ArgBufferRawPtr;
        ArgBufferUpperValue = ArgBufferUpper;
        ArgTypeRep = ArgDTD->TypeRep;
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
          FutureReturnBuffer, RetBufferUpper, RetDTD->TypeRep },
        "", CI);

      if (LowRetT != VoidTy)
        CI->replaceAllUsesWith(new LoadInst(LowRetT, FutureReturnBuffer, "deluge_result_load", CI));
      CI->eraseFromParent();
      return;
    }

    if (VAArgInst* VI = dyn_cast<VAArgInst>(I)) {
      Type* T = VI->getType();
      Type* LowT = lowerType(T);
      size_t Size = DL.getTypeStoreSize(LowT);
      size_t Alignment = DL.getABITypeAlign(LowT).value();
      DelugeTypeData* DTD = dataForLowType(LowT);
      assert(!DTD->Type.Trailing.isValid());
      assert(!(Size % DTD->Type.Main.Size));
      CallInst* Call = CallInst::Create(
        VAArgImpl,
        { VI->getPointerOperand(), ConstantInt::get(IntPtrTy, Size / DTD->Type.Main.Size),
          ConstantInt::get(IntPtrTy, Alignment), DTD->TypeRep, getOrigin(VI->getDebugLoc()) },
        "deluge_va_arg", VI);
      Call->setDebugLoc(VI->getDebugLoc());
      Instruction* Load = new LoadInst(LowT, Call, "deluge_va_arg_load", VI);
      Load->setDebugLoc(VI->getDebugLoc());
      VI->replaceAllUsesWith(Load);
      VI->eraseFromParent();
      return;
    }

    if (isa<ExtractElementInst>(I) ||
        isa<InsertElementInst>(I) ||
        isa<ShuffleVectorInst>(I) ||
        isa<ExtractValueInst>(I) ||
        isa<InsertValueInst>(I) ||
        isa<PHINode>(I) ||
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
  }

public:
  Deluge(Module &M, ModuleAnalysisManager &MAM)
    : C(M.getContext()), M(M), DL(const_cast<DataLayout&>(M.getDataLayout())), MAM(MAM) {
  }

  void run() {
    if (verbose)
      errs() << "Going to town on module:\n" << M << "\n";

    FunctionName = "<internal>";
    
    PtrBits = DL.getPointerSizeInBits(TargetAS);
    VoidTy = Type::getVoidTy(C);
    Int1Ty = Type::getInt1Ty(C);
    Int8Ty = Type::getInt8Ty(C);
    Int32Ty = Type::getInt32Ty(C);
    IntPtrTy = Type::getIntNTy(C, PtrBits);
    assert(IntPtrTy == Type::getInt64Ty(C)); // Deluge is 64-bit-only, for now.
    LowRawPtrTy = PointerType::get(C, TargetAS);
    LowWidePtrTy = StructType::create(
      {LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, LowRawPtrTy}, "deluge_wide_ptr");
    OriginTy = StructType::create(
      {LowRawPtrTy, LowRawPtrTy, Int32Ty, Int32Ty}, "deluge_origin");
    GlobalInitializationContextTy = StructType::create(
      {LowRawPtrTy, LowWidePtrTy, LowRawPtrTy}, "deluge_global_initialization_context");
    // See DELUDED_SIGNATURE in deluge_runtime.h.
    DeludedFuncTy = FunctionType::get(
      VoidTy, { LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, LowRawPtrTy }, false);
    GlobalGetterTy = FunctionType::get(
      LowWidePtrTy, { LowRawPtrTy }, false);
    LowRawNull = ConstantPointerNull::get(LowRawPtrTy);

    ZunsafeForgeImpl = M.getOrInsertFunction(
      "zunsafe_forge_impl", LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, IntPtrTy).getCallee();
    ZrestrictImpl = M.getOrInsertFunction(
      "zrestrict_impl", LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, IntPtrTy).getCallee();
    ZallocImpl = M.getOrInsertFunction(
      "zalloc_impl", LowRawPtrTy, LowRawPtrTy, IntPtrTy).getCallee();
    ZreallocImpl = M.getOrInsertFunction(
      "zrealloc_impl", LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, IntPtrTy).getCallee();

    assert(cast<Function>(ZunsafeForgeImpl)->isDeclaration());
    assert(cast<Function>(ZrestrictImpl)->isDeclaration());
    assert(cast<Function>(ZallocImpl)->isDeclaration());
    assert(cast<Function>(ZreallocImpl)->isDeclaration());
    
    if (verbose) {
      errs() << "zunsafe_forge_impl = " << ZunsafeForgeImpl << "\n";
      errs() << "zrestrict_impl = " << ZrestrictImpl << "\n";
      errs() << "zalloc_impl = " << ZallocImpl << "\n";
      errs() << "zrealloc_impl = " << ZreallocImpl << "\n";
    }

    // Capture the set of things that need conversion, before we start adding functions and globals.
    auto CaptureType = [&] (GlobalValue* G) {
      GlobalHighTypes[G] = G->getValueType();
      GlobalLowTypes[G] = lowerType(G->getValueType());
    };
    
    for (GlobalVariable &G : M.globals()) {
      Globals.push_back(&G);
      CaptureType(&G);
    }
    for (Function &F : M.functions()) {
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
    Int.TypeRep = new GlobalVariable(
      M, ArrayType::get(IntPtrTy, 4), true, GlobalVariable::ExternalLinkage, nullptr, "deluge_int_type");
    FunctionDTD.Type = DelugeType(1, 1);
    FunctionDTD.Type.Main.WordTypes.push_back(DelugeWordType::Function);
    FunctionDTD.TypeRep = new GlobalVariable(
      M, ArrayType::get(IntPtrTy, 4), true, GlobalVariable::ExternalLinkage, nullptr, "deluge_function_type");
    Invalid.Type = DelugeType(0, 0);
    Invalid.TypeRep = LowRawNull;
    
    LowWideNull = forgePtrConstant(LowRawNull, LowRawNull, LowRawNull, Invalid.TypeRep);

    Dummy = makeDummy(Int32Ty);
    FutureIntFrame = makeDummy(LowRawPtrTy);
    FutureTypedFrame = makeDummy(LowRawPtrTy);
    FutureReturnBuffer = makeDummy(LowRawPtrTy);

    GetHeap = M.getOrInsertFunction("deluge_get_heap", LowRawPtrTy, LowRawPtrTy);
    TryAllocateInt = M.getOrInsertFunction("deluge_try_allocate_int", LowRawPtrTy, IntPtrTy);
    TryAllocateIntWithAlignment = M.getOrInsertFunction("deluge_try_allocate_int_with_alignment", LowRawPtrTy, IntPtrTy);
    AllocateInt = M.getOrInsertFunction("deluge_allocate_int", LowRawPtrTy, IntPtrTy);
    AllocateIntWithAlignment = M.getOrInsertFunction("deluge_allocate_int_with_alignment", LowRawPtrTy, IntPtrTy, IntPtrTy);
    TryAllocateOne = M.getOrInsertFunction("deluge_try_allocate_one", LowRawPtrTy, LowRawPtrTy);
    AllocateOne = M.getOrInsertFunction("deluge_allocate_one", LowRawPtrTy, LowRawPtrTy);
    TryAllocateMany = M.getOrInsertFunction("deluge_try_allocate_many", LowRawPtrTy, LowRawPtrTy, IntPtrTy);
    AllocateUtility = M.getOrInsertFunction("deluge_allocate_utility", LowRawPtrTy, IntPtrTy);
    TryReallocateInt = M.getOrInsertFunction("deluge_try_reallocate_int", LowRawPtrTy, LowRawPtrTy, IntPtrTy);
    TryReallocateIntWithAlignment = M.getOrInsertFunction("deluge_try_reallocate_int", LowRawPtrTy, LowRawPtrTy, IntPtrTy, IntPtrTy);
    TryReallocate = M.getOrInsertFunction("deluge_try_reallocate", LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, IntPtrTy);
    Deallocate = M.getOrInsertFunction("deluge_deallocate", VoidTy, LowRawPtrTy);
    CheckAccessInt = M.getOrInsertFunction("deluge_check_access_int_impl", VoidTy, LowWidePtrTy, IntPtrTy, LowRawPtrTy);
    CheckAccessPtr = M.getOrInsertFunction("deluge_check_access_ptr_impl", VoidTy, LowWidePtrTy, LowRawPtrTy);
    CheckAccessFunctionCall = M.getOrInsertFunction("deluge_check_access_function_call_impl", VoidTy, LowWidePtrTy, LowRawPtrTy);
    Memset = M.getOrInsertFunction("deluge_memset_impl", VoidTy, LowWidePtrTy, Int32Ty, IntPtrTy, LowRawPtrTy);
    Memcpy = M.getOrInsertFunction("deluge_memcpy_impl", VoidTy, LowWidePtrTy, LowWidePtrTy, IntPtrTy, LowRawPtrTy);
    Memmove = M.getOrInsertFunction("deluge_memmove_impl", VoidTy, LowWidePtrTy, LowWidePtrTy, IntPtrTy, LowRawPtrTy);
    CheckRestrict = M.getOrInsertFunction("deluge_check_restrict", VoidTy, LowWidePtrTy, LowRawPtrTy, LowRawPtrTy, LowRawPtrTy);
    VAArgImpl = M.getOrInsertFunction("deluge_va_arg_impl", LowRawPtrTy, LowWidePtrTy, IntPtrTy, IntPtrTy, LowRawPtrTy, LowRawPtrTy);
    GlobalInitializationContextFind = M.getOrInsertFunction("deluge_global_initialization_context_find", LowRawPtrTy, LowRawPtrTy, LowRawPtrTy);
    Error = M.getOrInsertFunction("deluge_error", VoidTy, LowRawPtrTy);
    RealMemset = M.getOrInsertFunction("llvm.memset.p0.i64", VoidTy, LowRawPtrTy, Int8Ty, IntPtrTy, Int1Ty);
    MakeConstantPool = M.getOrInsertFunction("deluge_make_constantpool", LowRawPtrTy);

    GlobalConstantPoolPtr = new GlobalVariable(
      M, LowRawPtrTy, false, GlobalValue::PrivateLinkage, LowRawNull, "deluge_global_constantpool_ptr");

    auto FixupTypes = [&] (GlobalValue* G, GlobalValue* NewG) {
      GlobalHighTypes[NewG] = GlobalHighTypes[G];
      GlobalLowTypes[NewG] = GlobalLowTypes[G];
    };

    for (GlobalVariable* G : Globals) {
      Function* NewF = Function::Create(GlobalGetterTy, G->getLinkage(), G->getAddressSpace(),
                                        "deluded_g_" + G->getName(), &M);
      GlobalToGetter[G] = NewF;
      GetterToGlobal[NewF] = G;
      FixupTypes(G, NewF);
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

      GlobalVariable* NewG = new GlobalVariable(
        M, LowRawPtrTy, false, GlobalValue::PrivateLinkage, LowRawNull, "deluge_gptr_" + G->getName());
      
      BasicBlock* RootBB = BasicBlock::Create(C, "deluge_global_getter_root", NewF);
      BasicBlock* FastBB = BasicBlock::Create(C, "deluge_global_getter_fast", NewF);
      BasicBlock* SlowBB = BasicBlock::Create(C, "deluge_global_getter_slow", NewF);
      BasicBlock* RecurseBB = BasicBlock::Create(C, "deluge_global_getter_recurse", NewF);
      BasicBlock* BuildBB = BasicBlock::Create(C, "deluge_global_getter_build", NewF);

      Instruction* MyInitializationContext = new AllocaInst(
        GlobalInitializationContextTy, 0, ConstantInt::get(IntPtrTy, 1), "deluge_initialization_context",
        RootBB);
      Instruction* LoadPtr = new LoadInst(LowRawPtrTy, NewG, "deluge_fast_load_global", RootBB);
      Instruction* Cmp = new ICmpInst(
        *RootBB, ICmpInst::ICMP_EQ, LoadPtr, LowRawNull, "deluge_check_global");
      BranchInst::Create(SlowBB, FastBB, Cmp, RootBB);

      Instruction* LoadWidePtr = new LoadInst(LowWidePtrTy, LoadPtr, "deluge_fast_load_wide", FastBB);
      ReturnInst::Create(C, LoadWidePtr, FastBB);

      Instruction* Find = CallInst::Create(
        GlobalInitializationContextFind, { NewF->getArg(0), NewF }, "deluge_context_find", SlowBB);
      Cmp = new ICmpInst(
        *SlowBB, ICmpInst::ICMP_EQ, Find, LowRawNull, "deluge_check_find");
      BranchInst::Create(BuildBB, RecurseBB, Cmp, SlowBB);

      Instruction* WidePtrPtr = GetElementPtrInst::Create(
        GlobalInitializationContextTy, Find,
        { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) },
        "deluge_context_ptr_ptr", RecurseBB);
      LoadWidePtr = new LoadInst(LowWidePtrTy, WidePtrPtr, "deluge_recurse_load_wide", RecurseBB);
      ReturnInst::Create(C, LoadWidePtr, RecurseBB);

      Instruction* GetterPtr = GetElementPtrInst::Create(
        GlobalInitializationContextTy, MyInitializationContext,
        { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 0) }, "deluge_context_getter",
        BuildBB);
      WidePtrPtr = GetElementPtrInst::Create(
        GlobalInitializationContextTy, MyInitializationContext,
        { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 1) }, "deluge_context_ptr",
        BuildBB);
      Instruction* OuterPtr = GetElementPtrInst::Create(
        GlobalInitializationContextTy, MyInitializationContext,
        { ConstantInt::get(Int32Ty, 0), ConstantInt::get(Int32Ty, 2) }, "deluge_context_outer",
        BuildBB);
      new StoreInst(NewF, GetterPtr, BuildBB);
      new StoreInst(NewF->getArg(0), OuterPtr, BuildBB);
      // FIXME: What if we're dealing with an array? Right now, we'll create a type that O(array size).
      // We could at least detect repeats here.
      DelugeTypeData* DTD = dataForLowType(LowT);
      Instruction* Alloc;
      if (DTD == &Int) {
        assert(!hasPtrsForCheck(LowT));
        size_t Alignment = DL.getABITypeAlign(LowT).value();
        size_t Size = DL.getTypeStoreSize(LowT);
        if (Alignment > MinAlign) {
          Alloc = CallInst::Create(
            AllocateIntWithAlignment,
            { ConstantInt::get(IntPtrTy, Size), ConstantInt::get(IntPtrTy, Alignment) },
            "deluge_alloc_int", BuildBB);
        } else {
          Alloc = CallInst::Create(
            AllocateInt, { ConstantInt::get(IntPtrTy, Size) }, "deluge_alloc_int", BuildBB);
        }
      } else {
        // It's probably not worth it to use the heap constant pool for this.
        Value* Heap = CallInst::Create(GetHeap, { DTD->TypeRep }, "deluge_get_heap", BuildBB);
        Alloc = CallInst::Create(AllocateOne, { Heap }, "deluge_alloc_one", BuildBB);
      }
      Instruction* Upper = GetElementPtrInst::Create(
        LowT, Alloc, { ConstantInt::get(IntPtrTy, 1) }, "deluge_alloc_upper", BuildBB);
      Instruction* Return = ReturnInst::Create(C, LowWideNull, BuildBB);
      Value* WidePtr = forgePtr(Alloc, Alloc, Upper, DTD->TypeRep, Return);
      new StoreInst(WidePtr, WidePtrPtr, Return);
      Value* C = lowerConstant(G->getInitializer(), Return, MyInitializationContext);
      new StoreInst(C, Alloc, Return);
      DTD = dataForLowType(LowWidePtrTy);
      // This could be so much more efficient, but whatever.
      Value* Heap = CallInst::Create(GetHeap, { DTD->TypeRep }, "deluge_get_heap", Return);
      Alloc = CallInst::Create(AllocateOne, { Heap }, "deluge_alloc_one", Return);
      new StoreInst(WidePtr, Alloc, Return);
      new FenceInst(this->C, AtomicOrdering::AcquireRelease, SyncScope::System, Return);
      new StoreInst(Alloc, NewG, Return);
      Return->getOperandUse(0) = WidePtr;
    }
    for (Function* F : Functions) {
      if (F->isIntrinsic() ||
          F == ZunsafeForgeImpl ||
          F == ZrestrictImpl ||
          F == ZallocImpl ||
          F == ZreallocImpl)
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
          Value* ReturnDataPtr = forgePtr(
            NewF->getArg(3), NewF->getArg(3), NewF->getArg(4), NewF->getArg(5), Return);
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

        ArgBufferPtr = forgePtr(
          NewF->getArg(0), NewF->getArg(0), NewF->getArg(1), NewF->getArg(2), InsertionPoint);
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
          checkRecurse(LowT, ArgBufferPtr, ArgPtr, InsertionPoint);
          Args.push_back(new LoadInst(LowT, ArgPtr, "deluge_load_arg", InsertionPoint));
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
          lowerInstruction(I);

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

        InsertionPoint = &*FirstRealBlock->getFirstInsertionPt();
        
        if (IntFrameSize) {
          Instruction* AllocateIntFrame;
          if (IntFrameAlignment > MinAlign) {
            AllocateIntFrame = CallInst::Create(
              AllocateIntWithAlignment,
              { ConstantInt::get(IntPtrTy, IntFrameSize), ConstantInt::get(IntPtrTy, IntFrameAlignment) },
              "deluge_allocate_int_frame", InsertionPoint);
          } else {
            AllocateIntFrame = CallInst::Create(
              AllocateInt, { ConstantInt::get(IntPtrTy, IntFrameSize) }, "deluge_allocate_int_frame",
              InsertionPoint);
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
            AllocateOne, { getHeap(DTD, InsertionPoint) }, "deluge_allocate_typed_frame", InsertionPoint);
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
      
      if (verbose)
        errs() << "New function: " << *NewF << "\n";
    }
    for (GlobalAlias* G : Aliases) {
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
    Instruction* ConstantPoolPtr = CallInst::Create(
      AllocateUtility, { ConstantInt::get(IntPtrTy, 8 * ConstantPoolEntries.size()) },
      "deluge_allocate_constantpool", BB);
    for (size_t Index = ConstantPoolEntries.size(); Index--;) {
      Instruction* EntryPtr = GetElementPtrInst::Create(
        LowRawPtrTy, ConstantPoolPtr, { ConstantInt::get(IntPtrTy, Index) },
        "deluge_constantpool_entry_ptr", BB);
      const ConstantPoolEntry& CPE = ConstantPoolEntries[Index];
      switch (CPE.Kind) {
      case ConstantPoolEntryKind::Heap: {
        Instruction* Heap = CallInst::Create(GetHeap, { CPE.u.Heap.DTD->TypeRep }, "deluge_get_heap", BB);
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

    if (verbose)
      errs() << "Here's the deluded module:\n" << M << "\n";
  }
};

} // anonymous namespace

PreservedAnalyses DelugePass::run(Module &M, ModuleAnalysisManager &MAM) {
  Deluge D(M, MAM);
  D.run();
  return PreservedAnalyses::none();
}

