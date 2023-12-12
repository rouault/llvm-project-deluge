#ifndef LLVM_TRANSFORMS_INSTRUMENTATION_DELUGE_H
#define LLVM_TRANSFORMS_INSTRUMENTATION_DELUGE_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class Module;

class DelugePass : public PassInfoMixin<DelugePass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
  static bool isRequired() { return true; }
};

} // namespace llvm

#endif /* LLVM_TRANSFORMS_INSTRUMENTATION_DELUGE_H */

