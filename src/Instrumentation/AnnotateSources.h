#pragma once

/**
 * Add/remove instrumentation to mark the sources for the Tag Analysis
 ***/

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace clam_prov {

class addSources : public llvm::ModulePass {
  llvm::FunctionCallee m_addTag;
  llvm::FunctionCallee m_seadsaModified;
  void insertClamAddTag(long long tag, llvm::Value &ptr,
                        llvm::Instruction *insertPt);
  bool runOnFunction(llvm::Function &F);

public:
  static char ID;
  addSources();
  virtual bool runOnModule(llvm::Module &M) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  virtual llvm::StringRef getPassName() const override {
    return "Add instrumentation to mark sources for the Crab Tag analysis";
  }
};

class removeSources : public llvm::FunctionPass {
public:
  static char ID;
  removeSources();
  virtual bool runOnFunction(llvm::Function &F) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  virtual llvm::StringRef getPassName() const override {
    return "Remove instrumentation added for addSources";
  }
};
} // end namespace clam_prov
