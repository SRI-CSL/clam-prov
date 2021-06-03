#include "./OutputResults.h"
#include "./ReadMetadata.h"

#include "llvm/ADT/Optional.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Type.h"

using namespace llvm;
using namespace clam;

namespace clam_prov {

ConstantAsMetadata *createConstant(Constant *C) {
  return ConstantAsMetadata::get(C);
}

static MDNode *createTags(LLVMContext &ctx,
                          clam::ClamQueryAPI::TagVector &tags) {
  SmallVector<Metadata *, 4> Ops;
  for (uint64_t t : tags) {
    IntegerType *int64Ty = Type::getInt64Ty(ctx);
    Constant *c = cast<Constant>(ConstantInt::get(int64Ty, t));
    Ops.push_back(createConstant(c));
  }
  return MDNode::get(ctx, Ops);
}

bool TagAnalysisResultsAsMetadata(Module &M, InterGlobalClam &tagAnalysis) {
  LLVMContext &ctx = M.getContext();
  bool Change = false;
  for (auto &F : M) {
    for (auto &B : F) {
      for (auto &I : B) {
        if (CallBase *CB = dyn_cast<CallBase>(&I)) {
          long long callId /*unused*/;
          unsigned long long callArg; // starts from 1
          bool isInput;
          if (extractProvenanceFromCallsite(*CB, callId, callArg, isInput)) {
            if (!isInput) {
              Value *Ptr = CB->getArgOperand(callArg - 1);
              llvm::Optional<clam::ClamQueryAPI::TagVector> tags =
                  tagAnalysis.tags(B, *Ptr);
              if (tags.hasValue()) {
                MDNode *Tags = createTags(ctx, tags.getValue());
                CB->setMetadata("clam-prov-tags", Tags);
                Change = true;
                // DEBUG
                // errs() << "Tags associated to " << *CB << "={";
                // for (auto t: tags.getValue()) {
                // errs() << t << ";";
                // }
                // errs() << "}\n";
                ////
              }
            }
          }
        }
      }
    }
  }
  return Change;
}
} // end namespace clam_prov
