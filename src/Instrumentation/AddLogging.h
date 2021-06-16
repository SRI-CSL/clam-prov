#pragma once

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace clam_prov {

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
struct AddLogging : public llvm::PassInfoMixin<AddLogging> {
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &);
  bool runOnModule(llvm::Module &M);
};

//------------------------------------------------------------------------------
// Legacy PM interface
//------------------------------------------------------------------------------
struct LegacyAddLogging : public llvm::ModulePass {
  static char ID;
  LegacyAddLogging() : ModulePass(ID) {}
  bool runOnModule(llvm::Module &M) override;

  AddLogging Impl;
};

} // end namespace clam_prov
