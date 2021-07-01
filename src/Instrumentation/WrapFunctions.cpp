#include "WrapFunctions.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

namespace clam_prov {

/** Modify this function to add more wrappers **/
static void createWrapperMap(Module &M, DenseMap<Function*,FunctionCallee> &map) {
  LLVMContext &ctx = M.getContext();  
  Type *int8PtrTy = cast<Type>(Type::getInt8PtrTy(ctx));
  Type *i32Ty = Type::getInt32Ty(ctx);  
  Type *i64Ty = Type::getInt64Ty(ctx);

  auto readF = M.getOrInsertFunction("read", i64Ty, i32Ty, int8PtrTy, i64Ty);
  auto wrap_readF = M.getOrInsertFunction("clam_prov_read", i64Ty, i32Ty, int8PtrTy, i64Ty);
  map[cast<Function>(readF.getCallee())] = wrap_readF;    
  auto writeF = M.getOrInsertFunction("write", i64Ty, i32Ty, int8PtrTy, i64Ty);
  auto wrap_writeF = M.getOrInsertFunction("clam_prov_write", i64Ty, i32Ty, int8PtrTy, i64Ty);
  map[cast<Function>(writeF.getCallee())] = wrap_writeF;
}

WrapFunctions::WrapFunctions(): ModulePass(ID) {}

bool WrapFunctions::runOnModule(Module &M) {
  createWrapperMap(M, m_wrappers);
  
  bool Changed = false;
  for (auto &F : M) {
    if (F.empty()) {
      continue;
    }
    if (F.getName().startswith("clam_prov")) {
      continue;
    }
    Changed |= runOnFunction(F);
  }
  llvm::errs() << M << "\n";
  return Changed;
}

bool WrapFunctions::runOnFunction(Function &F) {
  LLVMContext &ctx = F.getParent()->getContext();
  IRBuilder<> Builder(ctx);
  bool Changed = false;
  
  DenseMap<Instruction*,Instruction*> replaceMap;
  for (auto &B : F) {
    for (auto &I : B) {
      if (CallBase *CB = dyn_cast<CallBase>(&I)) {
	if (Function* calleeF = CB->getCalledFunction()) {
	  auto it = m_wrappers.find(calleeF);
	  if (it != m_wrappers.end()) {
	    FunctionCallee wrapperF = it->second;
	    // Replace call to calleeF with wrapperF
	    Builder.SetInsertPoint(CB);
	    assert(calleeF->getFunctionType() == wrapperF.getFunctionType());
	    std::vector<Value*> args(CB->args().begin(), CB->args().end());
	    CallInst *newCI = Builder.CreateCall(wrapperF.getFunctionType(), wrapperF.getCallee(), args);
	    newCI->setName(CB->getName());
	    replaceMap[CB] = newCI;
	    Changed = true;
	  }
	}
      }
    }
  }

  // Do the actual replacement
  for (auto &kv: replaceMap) {
    kv.first->replaceAllUsesWith(kv.second);
    kv.first->eraseFromParent();
  }
  
  return Changed;
}

void WrapFunctions::getAnalysisUsage(AnalysisUsage &AU) const {
  // TODO: update callgraph so that we can claim that we preserve all
  // AU.setPreservesAll();
}

char WrapFunctions::ID = 0;
} // namespace clam_prov


static RegisterPass<clam_prov::WrapFunctions>
    X("wrap-functions", "Replace some calls with special wrappers");

