#pragma once

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace clam_prov {

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
struct OutputDependencyMap : public llvm::PassInfoMixin<OutputDependencyMap> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);
  bool runOnModule(llvm::Module &M);
};

//------------------------------------------------------------------------------
// Legacy PM interface
//------------------------------------------------------------------------------
struct LegacyOutputDependencyMap : public llvm::ModulePass {
  static char ID;
  LegacyOutputDependencyMap() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override;

  OutputDependencyMap Impl;
};

} // end namespace clam_prov
