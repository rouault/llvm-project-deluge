#include "llvm/Transforms/Instrumentation/Deluge.h"

#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/TypedPointerType.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/TargetParser/Triple.h>
#include <vector>
#include <unordered_map>

using namespace llvm;

namespace {

// This has to match the Deluge runtime.
enum class DelugeWordType {
  Invalid = 0,
  Int = 1,
  PtrPart1 = 2,
  PtrPart2 = 3,
  PtrPart3 = 4,
  PtrPart4 = 5
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
};

struct DelugeType {
  CoreDelugeType Main;
  CoreDelugeType Trailing;

  DelugeType() = default;
  
  DelugeType(size_t Size, size_t Alignment)
    : Main(Size, Alignment) {
  }

  bool operator==(const DelugeType& Other) const {
    return Main == Other.Main && Trailing == Other.Trailing;
  }

  size_t hash() const {
    return Main.hash() + 11 * Trailing.hash();
  }
};

} // anonymous namespace

template<> struct std::hash<DelugeType>
{
  size_t operator()(const DelugeType& Key) const
  {
    return Key.hash();
  }
};

namespace {

struct DelugeTypeData {
  DelugeType Type;
  Constant* TypeRep { nullptr };
};

class Deluge {
  static constexpr unsigned TargetAS = 0;
  
  LLVMContext& C;
  Module &M;
  DataLayout& DL;
  ModuleAnalysisManager &MAM;

  unsigned PtrBits;
  Type* VoidTy;
  Type* Int32Ty;
  Type* IntPtrTy;
  PointerType* LowRawPtrTy;
  StructType* LowWidePtrTy;
  Constant* LowRawNull;
  Constant* LowWideNull;
  
  FunctionCallee GetHeap;
  FunctionCallee CheckAccessInt;
  FunctionCallee CheckAccessPtr;
  FunctionCallee Memset;
  FunctionCallee Memcpy;
  FunctionCallee Memmove;
  FunctionCallee Error;

  std::vector<GlobalVariable*> Globals;
  std::vector<Function*> Functions;
  std::vector<GlobalAlias*> Aliases;
  std::vector<GlobalIFunc*> IFuncs;
  std::unordered_map<GlobalValue*, Type*> GlobalLowTypes;
  std::unordered_map<GlobalValue*, Type*> GlobalHighTypes;

  std::unordered_map<Constant*, Constant*> ForgedConstants;

  std::unordered_map<Type*, DelugeTypeData*> TypeMap;
  std::unordered_map<DelugeType, std::unique_ptr<DelugeTypeData>> TypeDatas;
  DelugeTypeData Primitive;
  DelugeTypeData Invalid;

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
      assert(T->getPointerAddressSpace() == TargetAS);
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
    if (!hasPtrsForCheck(T))
      Data = &Primitive;
    else {
      DelugeType DT;
      // Deluge types derived from llvm types never have a trailing component.
      buildCoreTypeRecurse(DT.Main, T);

      // FIXME: Find repetitions?
      
      Data = dataForType(DT);
    }

    TypeMap[T] = Data;
    return Data;
  }

  Type* lowerType(Type* T) {
    assert(T != LowWidePtrTy);
    
    if (FunctionType* FT = dyn_cast<FunctionType>(T)) {
      std::vector<Type*> Params;
      for (Type* InnerT : FT->params())
        Params.push_back(lowerType(InnerT));
      // This works on arm64, but I dunno if it'll work on other platforms!!!
      return FunctionType::get(lowerType(FT->getReturnType()), Params, FT->isVarArg());
    }

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

  void checkInt(Value *P, unsigned Size, Instruction *InsertBefore) {
    CallInst::Create(CheckAccessPtr, { P, ConstantInt::get(IntPtrTy, Size) }, "", InsertBefore)
      ->setDebugLoc(InsertBefore->getDebugLoc());
  }

  void checkPtr(Value *P, Instruction *InsertBefore) {
    CallInst::Create(CheckAccessPtr, { P }, "", InsertBefore)->setDebugLoc(InsertBefore->getDebugLoc());
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

    if (ScalableVectorType* VT = cast<ScalableVectorType>(T)) {
      llvm_unreachable("Shouldn't ever see scalable vectors in hasPtrsForCheck");
      return false;
    }
    
    return false;
  }

  void checkRecurse(Type *T, Value *P, Instruction *InsertBefore) {
    if (!hasPtrsForCheck(T)) {
      checkInt(P, DL.getTypeStoreSize(T), InsertBefore);
      return;
    }
    
    if (FunctionType* FT = dyn_cast<FunctionType>(T)) {
      llvm_unreachable("shouldn't see function types in checkRecurse");
      return;
    }

    if (isa<TypedPointerType>(T)) {
      llvm_unreachable("Shouldn't ever see typed pointers");
      return;
    }

    if (isa<PointerType>(T))
      assert(T->getPointerAddressSpace() == TargetAS);

    // We might see either low or high types!!!
    if (T == LowRawPtrTy || T == LowWidePtrTy) {
      checkPtr(P, InsertBefore);
      return;
    }

    assert(!isa<PointerType>(T));

    if (StructType* ST = dyn_cast<StructType>(T)) {
      for (unsigned Index = ST->getNumElements(); Index--;) {
        Type* InnerT = ST->getElementType(Index);
        Value *InnerP = GetElementPtrInst::Create(ST, P, { 0, Index }, "deluge_InnerP_struct", InsertBefore);
        checkRecurse(InnerT, InnerP, InsertBefore);
      }
      return;
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(T)) {
      for (uint64_t Index = AT->getNumElements(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(AT, P, { 0, Index }, "deluge_InnerP_array", InsertBefore);
        checkRecurse(AT->getElementType(), InnerP, InsertBefore);
      }
      return;
    }
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(T)) {
      for (unsigned Index = VT->getElementCount().getFixedValue(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(VT, P, { 0, Index }, "deluge_InnerP_vector", InsertBefore);
        checkRecurse(VT->getElementType(), InnerP, InsertBefore);
      }
      return;
    }

    if (ScalableVectorType* VT = dyn_cast<ScalableVectorType>(T)) {
      llvm_unreachable("Shouldn't see scalable vector types in checkRecurse");
      return;
    }

    llvm_unreachable("Should not get here.");
  }

  Value* lowerPtr(Value *HighP, Instruction* InsertBefore) {
    Instruction* Result = ExtractValueInst::Create(HighP, { 0 }, "deluge_getlowptr", InsertBefore);
    Result->setDebugLoc(InsertBefore->getDebugLoc());
    return Result;
  }

  Constant* lowerPtrConstant(Constant* HighP) {
    ConstantStruct* CS = cast<ConstantStruct>(HighP);
    assert(CS->getNumOperands() == 4);
    assert(CS->getType() == LowWidePtrTy);
    return CS->getOperand(0);
  }

  // Insert whatever checks are needed to perform the access and then return the lowered pointer to
  // access.
  Value* prepareForAccess(Type *T, Value *HighP, Instruction *InsertBefore) {
    Value* LowP = lowerPtr(HighP, InsertBefore);
    checkRecurse(T, LowP, InsertBefore);
    return LowP;
  }

  Constant* forgePtrConstant(Constant* Ptr, Constant* Lower, Constant* Upper, Constant* TypeRep) {
    return ConstantStruct::get(LowWidePtrTy, { Ptr, Lower, Upper, TypeRep });
  }

  Constant* forgePtrConstantWithLowType(Constant* Ptr, Constant* Lower, Constant* Upper, Type* LowT) {
    return forgePtrConstant(Ptr, Lower, Upper, dataForLowType(LowT)->TypeRep);
  }

  Constant* forgePtrConstantWithLowType(Constant* Ptr, Type* LowT) {
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
    Instruction* Upper = GetElementPtrInst::Create(LowT, Ptr, { ConstantInt::get(IntPtrTy, 1) }, "deluge_upper", InsertionPoint);
    Upper->setDebugLoc(InsertionPoint->getDebugLoc());
    return forgePtrWithLowType(Ptr, Ptr, Upper, LowT, InsertionPoint);
  }

  Constant* reforgePtrConstant(Constant* LowWidePtr, Constant* NewLowRawPtr) {
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

  Constant* lowerConstantImpl(Constant* C) {
    if (isa<ConstantPointerNull>(C))
      return LowWideNull;

    if (GlobalValue* G = dyn_cast<GlobalValue>(C)) {
      Type* LowT = GlobalLowTypes[G];
      return forgePtrConstantWithLowType(G, LowT);
    }

    assert(isa<ConstantExpr>(C));
    ConstantExpr* CE = cast<ConstantExpr>(C);

    switch (CE->getOpcode()) {
    case Instruction::GetElementPtr:
    case Instruction::BitCast:
      return reforgePtrConstant(lowerConstant(CE->getOperand(0)), C);
    case Instruction::IntToPtr:
    case Instruction::AddrSpaceCast:
      return reforgePtrConstant(LowWideNull, C);
    default:
      llvm_unreachable("Invalid ConstExpr returning ptr");
      return nullptr;
    }
  }

  Constant* lowerConstant(Constant* C) {
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
    
    if (C->getType() != LowRawPtrTy)
      return C;

    auto iter = ForgedConstants.find(C);
    if (iter != ForgedConstants.end())
      return iter->second;

    Constant* LowC = lowerConstantImpl(C);
    ForgedConstants[C] = LowC;
    return LowC;
  }

  Value* makeIntPtr(Value* V, Instruction *InsertionPoint) {
    if (V->getType() == IntPtrTy)
      return V;
    Instruction* Result =
      CastInst::CreateIntegerCast(V, IntPtrTy, false, "deluge_makeintptr", InsertionPoint);
    Result->setDebugLoc(InsertionPoint->getDebugLoc());
    return Result;
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
  void lowerInstruction(Instruction *I, Function* NewF) {
    for (unsigned Index = I->getNumOperands(); Index--;) {
      Use& U = I->getOperandUse(Index);
      if (Constant* C = dyn_cast<Constant>(U))
        U = lowerConstant(C);
      if (Argument* A = dyn_cast<Argument>(U))
        U = NewF->getArg(A->getArgNo());
    }
    
    if (AllocaInst* AI = dyn_cast<AllocaInst>(I)) {
      Type* LowT = lowerType(AI->getAllocatedType());
      AI->setAllocatedType(LowT);
      AI->replaceAllUsesWith(forgePtrWithLowType(AI, LowT, AI->getNextNode()));
      return;
    }

    if (LoadInst* LI = dyn_cast<LoadInst>(I)) {
      ReplaceInstWithInst(
        LI, new LoadInst(
          lowerType(LI->getType()), prepareForAccess(LI->getType(), LI->getPointerOperand(), LI),
          LI->getName(), LI->isVolatile(), LI->getAlign(), LI->getOrdering(), LI->getSyncScopeID()));
      return;
    }

    if (StoreInst* SI = dyn_cast<StoreInst>(I)) {
      if (hasPtrsForCheck(SI->getPointerOperand()->getType())) {
        SI->getOperandUse(StoreInst::getPointerOperandIndex()) =
          prepareForAccess(SI->getValueOperand()->getType(), SI->getPointerOperand(), SI);
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
          prepareForAccess(AI->getNewValOperand()->getType(), AI->getPointerOperand(), AI);
      }
      if (hasPtrsForCheck(AI->getNewValOperand()->getType()))
        llvm_unreachable("Cannot handle CAS on pointer field, sorry");
      return;
    }

    if (AtomicRMWInst* AI = dyn_cast<AtomicRMWInst>(I)) {
      if (hasPtrsForCheck(AI->getPointerOperand()->getType())) {
        AI->getOperandUse(AtomicRMWInst::getPointerOperandIndex()) =
          prepareForAccess(AI->getValOperand()->getType(), AI->getPointerOperand(), AI);
      }
      if (hasPtrsForCheck(AI->getValOperand()->getType()))
        llvm_unreachable("Cannot handle CAS on pointer field, sorry");
      return;
    }

    if (GetElementPtrInst* GI = dyn_cast<GetElementPtrInst>(I)) {
      GI->setSourceElementType(lowerType(GI->getSourceElementType()));
      GI->setResultElementType(lowerType(GI->getResultElementType()));
      GI->getOperandUse(0) = lowerPtr(GI->getOperand(0), GI);
      GI->replaceAllUsesWith(reforgePtr(GI->getOperand(0), GI, GI->getNextNode()));
      return;
    }

    if (ICmpInst* CI = dyn_cast<ICmpInst>(I)) {
      if (isa<PointerType>(CI->getOperand(0)->getType()) &&
          CI->getOperand(0)->getType()->getPointerAddressSpace() == TargetAS) {
        CI->getOperandUse(0) = lowerPtr(CI->getOperand(0), CI);
        CI->getOperandUse(1) = lowerPtr(CI->getOperand(1), CI);
      }
      return;
    }

    if (isa<FCmpInst>(I) ||
        isa<ReturnInst>(I) ||
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

    if (InvokeInst* II = dyn_cast<InvokeInst>(I)) {
      llvm_unreachable("Don't support InvokeInst yet");
      return;
    }
    
    if (IntrinsicInst* II = dyn_cast<IntrinsicInst>(I)) {
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
            Memcpy, { II->getArgOperand(0), II->getArgOperand(1), makeIntPtr(II->getArgOperand(2), II) },
            "deluge_memcpy", II);
          ReplaceInstWithInst(II, CI);
        }
        return;
      case Intrinsic::memset:
      case Intrinsic::memset_inline:
        if (hasPtrsForCheck(II->getArgOperand(0)->getType())) {
          Instruction* CI = CallInst::Create(
            Memset, { II->getArgOperand(0), II->getArgOperand(1), makeIntPtr(II->getArgOperand(2), II) },
            "deluge_memset", II);
          ReplaceInstWithInst(II, CI);
        }
        return;
      case Intrinsic::memmove:
        if (hasPtrsForCheck(II->getArgOperand(0)->getType())) {
          assert(hasPtrsForCheck(II->getArgOperand(1)->getType()));
          Instruction* CI = CallInst::Create(
            Memmove, { II->getArgOperand(0), II->getArgOperand(1), makeIntPtr(II->getArgOperand(2), II) },
            "deluge_memmove", II);
          ReplaceInstWithInst(II, CI);
        }
        return;

      default:
        // Intrinsics that take pointers but do not access memory are handled by simply giving them the
        // raw pointers. It's possible that this is the null set, but it's nice to have this carveout.
        if (!II->getCalledFunction()->doesNotAccessMemory()) {
          llvm::errs() << "Unhandled intrinsic: " << *II << "\n";
          CallInst::Create(Error, "", II)->setDebugLoc(II->getDebugLoc());
        }
        for (Use& U : II->data_ops()) {
          if (hasPtrsForCheck(U->getType()))
            U = lowerPtr(U, II);
        }
        if (hasPtrsForCheck(II->getType()))
          II->replaceAllUsesWith(forgeBadPtr(II, II->getNextNode()));
        return;
      }
    }
    
    if (CallInst* CI = dyn_cast<CallInst>(I)) {
      // FIXME: We should totally do this, but not now:
      // Maybe we should lower calls to something that passes all arguments and returns all results
      // on the stack, which would then enable us to do very smart type checks?
      // We're fucking the ABI anyway, so why not?
      // But what does that mean for callers?
      // -> Callers must create an "outgoing arguments" alloca
      // -> all calls just place their arguments into that alloca
      // -> that alloca can also serve as the place that results get janked into
      // What does it mean for callees?
      // -> all args get RAUW to loads from the passed-in ptr

      if (CI->isInlineAsm())
        llvm_unreachable("Don't support InlineAsm");

      // FIXME: This doesn't do _any_ of the checking that needs to happen here.
      CI->mutateFunctionType(cast<FunctionType>(lowerType(CI->getFunctionType())));
      CI->getCalledOperandUse() = lowerPtr(CI->getCalledOperand(), CI);
      return;
    }

    if (VAArgInst* VI = dyn_cast<VAArgInst>(I)) {
      // FIXME: This could totally do smart checking if we accept more ABI carnage. See FIXME under
      // CallInst.
      VAArgInst* NewVI = new VAArgInst(
        lowerPtr(VI->getPointerOperand(), VI), lowerType(VI->getType()), "deluge_vaarg", VI);
      ReplaceInstWithInst(VI, NewVI);
      return;
    }

    if (isa<ExtractElementInst>(I) ||
        isa<InsertElementInst>(I) ||
        isa<ShuffleVectorInst>(I) ||
        isa<ExtractValueInst>(I) ||
        isa<InsertValueInst>(I) ||
        isa<PHINode>(I)) {
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
      CallInst::Create(Error, "", I)->setDebugLoc(I->getDebugLoc());
      return;
    }

    if (isa<IntToPtrInst>(I)) {
      I->replaceAllUsesWith(forgeBadPtr(I, I->getNextNode()));
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
          I->replaceAllUsesWith(forgeBadPtr(I, I->getNextNode()));
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

    PtrBits = DL.getPointerSizeInBits(TargetAS);
    VoidTy = Type::getVoidTy(C);
    Int32Ty = Type::getInt32Ty(C);
    IntPtrTy = Type::getIntNTy(C, PtrBits);
    LowRawPtrTy = PointerType::get(C, TargetAS);
    LowWidePtrTy = StructType::create(
      {LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, LowRawPtrTy}, "deluge_wide_ptr");
    LowRawNull = ConstantPointerNull::get(LowRawPtrTy);

    GetHeap = M.getOrInsertFunction("deluge_get_heap", LowRawPtrTy, LowRawPtrTy);
    CheckAccessInt = M.getOrInsertFunction("deluge_check_access_int_impl", VoidTy, LowWidePtrTy, IntPtrTy);
    CheckAccessPtr = M.getOrInsertFunction("deluge_check_access_ptr_impl", VoidTy, LowWidePtrTy);
    Memset = M.getOrInsertFunction("deluge_memset_impl", VoidTy, LowWidePtrTy, Int32Ty, IntPtrTy);
    Memcpy = M.getOrInsertFunction("deluge_memcpy_impl", VoidTy, LowWidePtrTy, LowWidePtrTy, IntPtrTy);
    Memmove = M.getOrInsertFunction("deluge_memmove_impl", VoidTy, LowWidePtrTy, LowWidePtrTy, IntPtrTy);
    Error = M.getOrInsertFunction("deluge_error", VoidTy);

    Primitive.Type = DelugeType(1, 1);
    buildTypeRep(Primitive);
    Invalid.Type = DelugeType(0, 0);
    Invalid.TypeRep = LowRawNull;
    
    LowWideNull = forgePtrConstant(LowRawNull, LowRawNull, LowRawNull, Invalid.TypeRep);

    auto FixupTypes = [&] (GlobalValue* G, GlobalValue* NewG) {
      GlobalHighTypes[NewG] = GlobalHighTypes[G];
      GlobalLowTypes[NewG] = GlobalLowTypes[G];
    };
    
    for (GlobalVariable* G : Globals) {
      GlobalVariable* NewG = new GlobalVariable(
        M, lowerType(G->getValueType()), G->isConstant(), G->getLinkage(),
        G->hasInitializer() ? lowerConstant(G->getInitializer()) : nullptr,
        "deluged_" + G->getName(), nullptr, G->getThreadLocalMode(), G->getAddressSpace(),
        G->isExternallyInitialized());
      FixupTypes(G, NewG);
      NewG->copyAttributesFrom(G);
      G->replaceAllUsesWith(NewG);
      G->eraseFromParent();

    }
    for (Function* F : Functions) {
      Function* NewF = Function::Create(cast<FunctionType>(lowerType(F->getFunctionType())),
                                        F->getLinkage(), F->getAddressSpace(),
                                        "deluded_" + F->getName(), &M);
      FixupTypes(F, NewF);
      std::vector<BasicBlock*> Blocks;
      for (BasicBlock& BB : *F)
        Blocks.push_back(&BB);
      for (BasicBlock* BB : Blocks) {
        BB->removeFromParent();
        BB->insertInto(NewF);
        for (Instruction& I : *BB)
          lowerInstruction(&I, NewF);
      }
      NewF->copyAttributesFrom(F);
      F->replaceAllUsesWith(NewF);
      F->eraseFromParent();
    }
    for (GlobalAlias* G : Aliases) {
      // FIXME: The GlobalAlias constant expression may produce something that is not at all a valid
      // pointer. It's not at all clear that we get the right behavior here. Probably, we want there to
      // be a compile-time or runtime check that we're producing a pointer that makes sense with a type
      // that makes sense.
      GlobalAlias* NewG = GlobalAlias::create(lowerType(G->getValueType()), G->getAddressSpace(),
                                              G->getLinkage(), "deluged_" + G->getName(),
                                              G->getAliasee(), &M);
      FixupTypes(G, NewG);
      NewG->copyAttributesFrom(G);
      G->replaceAllUsesWith(NewG);
      G->eraseFromParent();
    }
    for (GlobalIFunc* G : IFuncs) {
      GlobalIFunc* NewG = GlobalIFunc::create(lowerType(G->getValueType()), G->getAddressSpace(),
                                              G->getLinkage(), "deluged_" + G->getName(),
                                              G->getResolver(), &M);
      FixupTypes(G, NewG);
      NewG->copyAttributesFrom(G);
      G->replaceAllUsesWith(NewG);
      G->eraseFromParent();
    }
  }
};

} // anonymous namespace

PreservedAnalyses DelugePass::run(Module &M, ModuleAnalysisManager &MAM) {
  Deluge D(M, MAM);
  D.run();
  return PreservedAnalyses::none();
}

