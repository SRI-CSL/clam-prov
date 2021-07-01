#pragma once

/**
 * Replace some calls with special wrappers
 ***/

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace clam_prov {

class WrapFunctions : public llvm::ModulePass {
  llvm::DenseMap<llvm::Function*, llvm::FunctionCallee> m_wrappers;
  bool runOnFunction(llvm::Function &F);  
public:
  static char ID;
  WrapFunctions();
  virtual bool runOnModule(llvm::Module &M) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  virtual llvm::StringRef getPassName() const override {
    return "Replace some calls with special wrappers";
  }
};

} // end namespace clam_prov
