#pragma once

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

namespace llvm {
class Module;
class Function;
class Type;
class Value;
class CallBase;
} // end namespace llvm
namespace clam_prov {

/* Wrap all the sinks so that we can extract easily Clam tags */

class WrapSinks : public llvm::ModulePass {
  llvm::Type *m_int8PtrTy;
  llvm::FunctionCallee m_seadsaModified;
  llvm::DenseMap<llvm::CallBase *, llvm::SmallVector<unsigned, 4>> m_outputParamMap;

  bool runOnFunction(llvm::Function &F);
  llvm::FunctionCallee createWrapper(llvm::CallBase &CB);

public:
  static char ID;
  WrapSinks();
  virtual bool runOnModule(llvm::Module &M) override;
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  virtual llvm::StringRef getPassName() const override {
    return "Wrap special calls identified as sinks";
  }
};

} // namespace clam_prov
