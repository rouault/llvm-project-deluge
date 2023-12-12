#include "llvm/Transforms/Instrumentation/Deluge.h"

#include <llvm/IR/DebugInfo.h>
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
    return Size == Other.size && Alignment == Other.Alignment && WordTypes == Other.WordTypes;
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

class Deluge {
  constexpr unsigned TargetAS = 0;
  
  LLVMContext& C;
  Module &M;
  DataLayout& DL;
  ModuleAnalysisManager &MAM;

  Type* VoidTy;
  Type* Int32Ty;
  Type* IntPtrTy;
  Type* LowRawPtrTy;
  Type* LowWidePtrTy;
  
  FunctionCallee GetHeap;
  FunctionCallee CheckAccessInt;
  FunctionCallee CheckAccessPtr;
  FunctionCallee Memset;
  FunctionCallee Memcpy;
  FunctionCallee Memmove;

  std::vector<GlobalVariable*> Globals;
  std::vector<Function*> Functions;
  std::vector<GlobalAlias*> Aliases;
  std::vector<GlobalIFunc*> IFuncs;

  std::map<Constant*, Value*> ForgedConstants;
  Instruction *ForgingInsertionPoint;

  Constant* typeRep(Type* T) {
    
  }

  Type* lowerType(Type* T) {
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
      if (T->getPointerAddressSpace() == TargetAS)
        return LowWidePtrTy;
      assert(T == LowRawPtrTy);
      return T;
    }

    if (StructType* ST = dyn_cast<StructType>(T)) {
      std::vector<Type*> Elements;
      for (Type* InnerT : ST->elements())
        Elements.push_back(lowerType(InnerT));
      return StructType::get(C, Elements, ST->getName(), ST->isPacked());
    }
      
    if (ArrayType* AT = dyn_cast<ArrayType>(T))
      return Array::get(lowerType(AT->getElementType()), AT->getNumElements());
      
    if (FixedVectorType* VT = dyn_cast<FixedVectorType>(T))
      return FixedVectorType::get(lowerType(VT->getElementType()), VT->getElementCount());

    if (ScalableVectorType* VT = dyn_cast<ScalableVectorType>(T))
      return ScalableVectorType::get(lowerType(VT->getElementType()), VT->getElementCount());
    
    return T;
  }

  void checkInt(Value *P, unsigned Size, Instruction *InsertBefore) {
    CallInst::Create(CheckAccessPtr, { P, ConstantInt::get(IntPtrTy, Size) }, "", InsertBefore)
      ->setDebugLoc(InsertBefore->getDebugLoc());
  }

  void checkPtr(Value *P, Instruction *InsertBefore) {
    CallInst::Create(CheckAccessPtr, { P }, "", InsertBefore)->setDebugLoc(InsertBefore->getDebugLoc());
  }

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

    if (isa<PointerType>(T)) {
      assert(T->getPointerAddressSpace() == TargetAS);
      checkPtr(P, InsertBefore);
      return;
    }

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
      for (unsigned Index = VT->getElementCount(); Index--;) {
        Value *InnerP = GetElementPtrInst::Create(VT, P, { 0, Index }, "deluge_InnerP_vector", InsertBefore);
        checkRecurse(VT->getElementType(), InnerP, InsertBefore);
      }
      return;
    }

    if (ScalableVectorType* VT = dyn_cast<ScalableVectorType>(T)) {
      llvm_unreachable("Shouldn't see scalable vector types in checkRecurse");
      return;
    }
  }

  Value* lowerPtr(Value *HighP) {
    return ExtractValue::Create(P, { 0 }, "deluge_getlowptr", InsertBefore);
  }

  // Insert whatever checks are needed to perform the access and then return the lowered pointer to
  // access.
  Value* prepareForAccess(Type *T, Value *HighP, Instruction *InsertBefore) {
    Value* LowP = lowerPtr(HighP);
    checkRecurse(T, LowP, InsertBefore);
    return LowP;
  }

  Value* forgeConstant(Constant* C) {
    if (!isa<PointerType>(C->getType()))
      return C;
    if (C->getType()->getPointerAddressSpace() != TargetAS)
      return C;
    
    auto iter = ForgedConstants.find(C);
    if (iter != ForgedConstants.end())
      return iter->second;

    // FIXME: Need to interpret the constant! That means possibly creating a pointer that's in an OOB
    // state.

    // FIXME: This means we need to have a way of forging a type. That means:
    // - Functions in this module must have a check at the top of them to see if the global state is
    //   initialized.
    // - There is a global state that has all of the type pointers we would use. We can load from this
    //   as needed.
    // - There is some module initializer that we call on the slow path if the global state isn't
    //   initialized.
    // NO! It looks like we don't even have to hash cons types.

    // FIXME: Eventually, global variables in a module will have some kind of checking associated with
    // them so that if one module uses a global variable defined in another, then you cannot get a type
    // confused wide pointer, with any combination of these things used as the enforcement mechanism:
    // - Accessing a global requires checking some side-state of the global that tells you the type
    //   that the global was really allocated with
    // - Extern globals are implemented as function calls that return a wide pointer with the right type.
    // - Runtime maintains a global registry of types associated with globals, and every module that
    //   knows of a global tells the runtime what it expects of the type. The runtime may then trap if
    //   it detects a contradiction.
  }

  // This lowers the instruction "in place", so all references to it are fixed up after this runs.
  void lowerInstruction(Instruction *I) {
    // FIXME: need to replace all uses of constants with *something*. Like, using a pointer to a global
    // must forge that pointer. FUUUUUCK
    
    if (AllocaInst* AI = dyn_cast<AllocaInst>(I)) {
      AI->setAllocatedType(lowerType(AI->getAllocatedType()));
      // FIXME: This now returns a wide pointer!! WTF!!
      return;
    }

    if (LoadInst* LI = dyn_cast<LoadInst>(I)) {
      // FUCK - if we're loading/storing aggregate type, then we need to emit checks for the
      // subcomponents! This is wack!
      ReplaceInstWithInst(
        LI, new LoadInst(
          lowerType(LI->getType()), prepareForAccess(LI->getType(), LI->getPointerOperand(), LI),
          LI->getName(), LI->isVolatile(), LI->getAlign(), LI->getOrdering(), LI->getSyncScoeID()));
      return;
    }

    if (StoreInst* SI = dyn_cast<StoreInst>(I)) {
      if (SI->getPointerAddressSpace() == TargetAS) {
        SI->getOperandUse(StoreInst::getPointerOperandIndex()) =
          prepareForAccess(SI->getValueOperand()->getType(), SI->getPointerOperand(), SI);
      }
      return;
    }

    if (FenceInst* FI = dyn_cast<FenceInst>(I)) {
      // We don't need to do anything because it doesn't take operands.
      return;
    }

    if (AtomicCmpXchInst* AI = dyn_cast<AtomicCmpXchInst>(I)) {
      if (AI->getPointerAddressSpace() == TargetAS) {
        AI->getOperandUse(AtomicCmpXchInst::getPointerOperandIndex()) =
          prepareForAccess(AI->getNewValOperand()->getType(), AI->getPointerOperand(), AI);
      }
      return;
    }

    if (AtomicRMWInst* AI = dyn_cast<AtomicRMWInst>(I)) {
      if (AI->getPointerAddressSpace() == TargetAS) {
        AI->getOperandUse(AtomicRMWInst::getPointerOperandIndex()) =
          prepareForAccess(AI->getValOperand()->getType(), AI->getPointerOperand(), AI);
      }
      return;
    }

    if (GetElementPtrInst* GI = dyn_cast<GetElementPtrInst>(I)) {
      GI->setSourceElementType(lowerType(GI->getSourceElementType()));
      GI->setResultElementType(lowerType(GI->getResultElementType()));
      return;
    }

    if (ICmpInst* CI = dyn_cast<ICmpInst>(I)) {
      if (isa<PointerType>(CI->getOperand(0)->getType()) &&
          CI->getOperand(0)->getType()->getPointerAddressSpace() == TargetAS) {
        CI->getOperandUse(0) = lowerPtr(CI->getOperand(0));
        CI->getOperandUse(1) = lowerPtr(CI->getOperand(1));
      }
      return;
    }

    if (FCmpInst* CI = dyn_cast<FCmpInst>(I)) {
      // We're gucci.
      return;
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

      
      return;
    }

    FIXME;
  }

public:
  Deluge(Module &M, ModuleAnalysisManager &MAM)
    : C(M.getContext()), M(M), DL(const_cast<DataLayout&>(M.getDataLout())), MAM(MAM) {
  }

  void run() {
    // Capture the set of things that need conversion, before we start adding functions and globals.
    for (GlobalVariable *G : M.globals())
      Globals.push_back(G);
    for (Function *F : M.functions())
      Functions.push_back(F);
    for (GlobalAlias *G : M.aliases())
      Aliases.push_back(G);
    for (GlobalIFunc *G : M.ifuncs())
      IFuncs.push_back(G);

    VoidTy = Type::getVoidTy(C);
    Int32Ty = Type::getInt32Ty(C);
    IntPtrTy = Type::getIntNTy(C, DL.getPointerSizeInBits(TargetAS));
    LowRawPtrTy = PointerType::get(C, TargetAS);
    LowWidePtrTy = StructType::create(
      {LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, LowRawPtrTy}, "deluge_wide_ptr");

    GetHeap = M.getOrInsertFunction("deluge_get_heap", LowRawPtrTy, LowRawPtrTy);
    CheckAccessInt = M.getOrInsertFunction("deluge_check_access_int_impl", VoidTy, LowWidePtrTy, IntPtrTy);
    CheckAccessPtr = M.getOrInsertFunction("deluge_check_access_ptr_impl", VoidTy, LowWidePtrTy);
    Memset = M.getOrInsertFunction("deluge_memset_impl", VoidTy, LowWidePtrTy, Int32Ty, IntPtrTy);
    Memcpy = M.getOrInsertFunction("deluge_memcpy_impl", VoidTy, LowWidePtrTy, LowWidePtrTy, IntPtrTy);
    Memmove = M.getOrInsertFunction("deluge_memmove_impl", VoidTy, LowWidePtrTy, LowWidePtrTy, IntPtrTy);
    
    // Need to rewrite all things that use pointers.
    // - All uses of pointers need to be transformed to calls to wide pointer functions.
    // - All pointer data flow needs to be replaced with data flow of wide pointer records.
    // - All types everywhere must now mention the wide pointer records.
  }
};

} // anonymous namespace

PreservedAnalyses DelugePass::run(Module &M, ModuleAnalysisManager &MAM) {
  Deluge D(M, MAM);
  D.run();
  return PreservedAnalyses::none();
}

