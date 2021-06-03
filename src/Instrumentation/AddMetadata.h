#pragma once

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace clam_prov {

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
struct AddMetadata : public llvm::PassInfoMixin<AddMetadata> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);
  bool runOnModule(llvm::Module &M);
};

//------------------------------------------------------------------------------
// Legacy PM interface
//------------------------------------------------------------------------------
struct LegacyAddMetadata : public llvm::ModulePass {
  static char ID;
  LegacyAddMetadata() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override;

  AddMetadata Impl;
};

} // end namespace clam_prov
