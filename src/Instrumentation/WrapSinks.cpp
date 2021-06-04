#include "./WrapSinks.h"
#include "./ReadMetadata.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"

using namespace llvm;

namespace clam_prov {

WrapSinks::WrapSinks()
    : ModulePass(ID), m_int8PtrTy(nullptr), m_seadsaModified(nullptr) {}

FunctionCallee WrapSinks::createWrapper(CallBase &CB) {
  assert(CB->getCalledFunction());

  Function &F = *(CB.getCalledFunction());
  Module &M = *(F.getParent());
  FunctionType *type = F.getFunctionType();

  // Don't use caching. We want one distinct wrapper per call.
  static unsigned n = 0;
  FunctionCallee wrapperFC =
      M.getOrInsertFunction("clam_prov_sink." + std::to_string(n++), type);

  // Body of the wrapper. It has a weird shape but needed to extract
  // invariants at the right place.
  LLVMContext &ctx = M.getContext();
  IRBuilder<> Builder(ctx);
  Function *wrapperF = cast<Function>(wrapperFC.getCallee());
  BasicBlock *EntryBB = BasicBlock::Create(ctx, "entry", wrapperF);
  BasicBlock *ExitBB = BasicBlock::Create(ctx, "exit", wrapperF);
  Builder.SetInsertPoint(EntryBB);
  Builder.CreateBr(ExitBB);
  Builder.SetInsertPoint(ExitBB); // at the end of the block
  std::vector<Value *> Params;
  Params.reserve(wrapperF->arg_size());
  for (Argument &Arg : wrapperF->args()) {
    Arg.setName("param");
    Params.push_back(&Arg);
  }
  CallInst *OrigCall = Builder.CreateCall(type, &F, Params,
					  (type->getReturnType()->isVoidTy() ? "" : "res"));
  OrigCall->copyMetadata(CB);
  Builder.SetInsertPoint(OrigCall); // before OrigCall
  Value *castedPtr = Builder.CreateBitOrPointerCast(
      wrapperF->getArg(m_outputParamMap[&CB]), m_int8PtrTy);
  Builder.CreateCall(m_seadsaModified.getFunctionType(),
                     m_seadsaModified.getCallee(), {castedPtr});

  Builder.SetInsertPoint(ExitBB); // at the end of the block
  if (type->getReturnType()->isVoidTy()) {
    Builder.CreateRetVoid();
  } else {
    Builder.CreateRet(OrigCall);
  }

  return wrapperFC;
}

bool WrapSinks::runOnModule(Module &M) {

  LLVMContext &ctx = M.getContext();
  Type *voidTy = Type::getVoidTy(ctx);
  m_int8PtrTy = cast<Type>(Type::getInt8PtrTy(ctx));
  m_seadsaModified =
      M.getOrInsertFunction("sea_dsa_set_modified", voidTy, m_int8PtrTy);
  if (!m_seadsaModified) {
    return false;
  }

  bool Changed = false;
  for (auto &F : M) {
    Changed |= runOnFunction(F);
  }
  return Changed;
}

bool WrapSinks::runOnFunction(Function &F) {
  Module &M = *(F.getParent());
  LLVMContext &ctx = M.getContext();
  IRBuilder<> Builder(ctx);
  bool Change = false;
  // worklist with calls to be wrapped
  std::vector<CallBase *> callsToWrap;
  for (auto &B : F) {
    if (F.getName().startswith("clam_prov_sink")) {
      // important to skip these
      continue;
    }
    for (auto &I : B) {
      if (CallBase *CB = dyn_cast<CallBase>(&I)) {
        if (!CB->getCalledFunction()) {
          continue;
        }
        long long callId /*unused*/;
        unsigned long long callArg;
        bool isInput;
        if (extractProvenanceFromCallsite(*CB, callId, callArg, isInput)) {
          if (!isInput) {
            callsToWrap.push_back(CB);
            m_outputParamMap[CB] = callArg - 1;
            Change = true;
          }
        }
      }
    }
  }

  while (!callsToWrap.empty()) {
    CallBase *CB = callsToWrap.back();
    callsToWrap.pop_back();

    // Replace original call with wrapper
    FunctionCallee FC = createWrapper(*CB);
    std::vector<Value *> Params;
    Params.reserve(CB->getNumArgOperands());
    for (auto &Param : CB->args()) {
      Params.push_back(Param.get());
    }
    Builder.SetInsertPoint(CB);
    CallInst *Wrapper =
        Builder.CreateCall(FC.getFunctionType(), FC.getCallee(), Params);
    Wrapper->setName(CB->getName());
    CB->replaceAllUsesWith(Wrapper);

    // Remove original call
    if (CB->use_empty()) {
      CB->eraseFromParent();
    }
  }
  return Change;
}

void WrapSinks::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char WrapSinks::ID = 0;

} // end namespace clam_prov
