#include "./AnnotateSources.h"
#include "./ProvMetadata.h"

#include "llvm/IR/IRBuilder.h"

#include <forward_list>

using namespace llvm;

namespace clam_prov {

void addSources::insertClamAddTag(long long tag, Value &ptr,
                                  Instruction *insertPt) {
  LLVMContext &ctx = insertPt->getContext();
  Type *int8PtrTy = cast<Type>(Type::getInt8PtrTy(ctx));
  IntegerType *int64Ty = Type::getInt64Ty(ctx);
  IRBuilder<> Builder(ctx);

  Builder.SetInsertPoint(insertPt);
  Value *castedPtr = Builder.CreateBitOrPointerCast(&ptr, int8PtrTy);
  if (castedPtr != &ptr) {
    Builder.SetInsertPoint(insertPt);
  }

  Builder.CreateCall(m_seadsaModified.getFunctionType(),
                     m_seadsaModified.getCallee(), {castedPtr});

  Builder.CreateCall(m_addTag.getFunctionType(), m_addTag.getCallee(),
                     {castedPtr, ConstantInt::get(int64Ty, tag)});
}

addSources::addSources()
    : ModulePass(ID), m_addTag(nullptr), m_seadsaModified(nullptr) {}

bool addSources::runOnModule(Module &M) {
  // errs() << M << "\n";

  LLVMContext &ctx = M.getContext();
  Type *int8PtrTy = cast<Type>(Type::getInt8PtrTy(ctx));
  Type *tagTy = Type::getInt64Ty(ctx);
  Type *voidTy = Type::getVoidTy(ctx);
  m_addTag = M.getOrInsertFunction("__CRAB_intrinsic_add_tag", voidTy,
                                   int8PtrTy, tagTy);
  m_seadsaModified =
      M.getOrInsertFunction("sea_dsa_set_modified", voidTy, int8PtrTy);

  if (!m_addTag || !m_seadsaModified) {
    return false;
  }

  bool Changed = false;
  for (auto &F : M) {
    Changed |= runOnFunction(F);
  }
  return Changed;
}

bool addSources::runOnFunction(Function &F) {
  bool Changed = false;
  for (auto &B : F) {
    for (auto &I : B) {
      if (CallBase *CB = dyn_cast<CallBase>(&I)) {
        long long callId;
        unsigned long long callArg; // starts from 1
        bool isInput;
        if (getCallSiteMetadataAndFirstArgumentType(*CB, callId, callArg, isInput)) {
          if (isInput) {
            Changed = true;
            insertClamAddTag(callId, *(CB->getArgOperand(callArg - 1)), CB);
          }
        }
      }
    }
  }
  return Changed;
}

void addSources::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char addSources::ID = 0;
} // namespace clam_prov

namespace clam_prov {

removeSources::removeSources() : FunctionPass(ID) {}

bool removeSources::runOnFunction(Function &F) {
  std::forward_list<CallInst *> toerase;

  for (Function::iterator b = F.begin(), be = F.end(); b != be; ++b) {
    for (BasicBlock::iterator it = b->begin(), ie = b->end(); it != ie; ++it) {
      User *u = &(*it);
      // -- looking for empty users
      if (!u->use_empty())
        continue;

      if (CallInst *ci = dyn_cast<CallInst>(u)) {
        Function *f = ci->getCalledFunction();
        if (f == NULL)
          continue;

        if (f->getName() == "__CRAB_intrinsic_add_tag" || f->getName() == "sea_dsa_set_modified") {
          toerase.push_front(ci);
        }
      }
    }
  }

  for (CallInst *ci : toerase) {
    ci->eraseFromParent();
  }

  return !toerase.empty();
}

void removeSources::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char removeSources::ID = 0;

} // namespace clam_prov

static RegisterPass<clam_prov::addSources>
    X("insert-clam-tag-instr", "Mark sources for the Clam Tag analysis");

static RegisterPass<clam_prov::removeSources>
    Y("remove-clam-tag-instr",
      "Remove instrumentation added by insert-crab-tag-instr");
