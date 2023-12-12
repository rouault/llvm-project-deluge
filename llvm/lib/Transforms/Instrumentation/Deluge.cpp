#include "llvm/Transforms/Instrumentation/Deluge.h"

#include <llvm/IR/DebugInfo.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/TargetParser/Triple.h>
#include <vector>

using namespace llvm;

namespace {

class Deluge {
  constexpr unsigned TargetAS = 0;
  
  LLVMContext& C;
  Module &M;
  DataLayout& DL;
  ModuleAnalysisManager &MAM;

  Type* VoidTy;
  Type* IntPtrTy;
  Type* LowRawPtrTy;
  Type* LowWidePtrTy;
  
  FunctionCallee CheckAccessInt;
  FunctionCallee CheckAccessPtr;

  std::vector<GlobalVariable*> Globals;
  std::vector<Function*> Functions;
  std::vector<GlobalAlias*> Aliases;
  std::vector<GlobalIFunc*> IFuncs;

  // This turns ptrs into structs.
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

  // Insert whatever checks are needed to perform the access and then return the lowered pointer to
  // access.
  Value* prepareForAccess(Type *T, Value *HighP, Instruction *InsertBefore) {
    Value* LowP = ExtractValue::Create(P, { 0 }, "deluge_getlowptr", InsertBefore);
    checkRecurse(T, LowP, InsertBefore);
    return LowP;
  }

  // This lowers the instruction "in place", so all references to it are fixed up after this runs.
  void lowerInstruction(Instruction *I) {
    if (AllocaInst* AI = dyn_cast<AllocaInst>(I)) {
      AI->setAllocatedType(lowerType(AI->getAllocatedType()));
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

    //FIXME: Next up: ICmpInst
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
    IntPtrTy = Type::getIntNTy(C, DL.getPointerSizeInBits(TargetAS));
    LowRawPtrTy = PointerType::get(C, TargetAS);
    LowWidePtrTy = StructType::create(
      {LowRawPtrTy, LowRawPtrTy, LowRawPtrTy, LowRawPtrTy}, "deluge_wide_ptr");

    CheckAccessInt = M.getOrInsertFunction("deluge_check_access_int", VoidTy, LowWidePtrTy, IntPtrTy);
    CheckAccessPtr = M.getOrInsertFunction("deluge_check_access_ptr", VoidTy, LowWidePtrTy);
    
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

