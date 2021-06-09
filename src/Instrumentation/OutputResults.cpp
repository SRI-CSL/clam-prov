#include "./OutputResults.h"
#include "./ProvMetadata.h"

#include "llvm/ADT/Optional.h"

using namespace llvm;
using namespace clam;

namespace clam_prov {

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
          if (getCallSiteMetadataAndFirstArgumentType(*CB, callId, callArg, isInput)) {
            if (!isInput) {
              Value *Ptr = CB->getArgOperand(callArg - 1);
              llvm::Optional<clam::ClamQueryAPI::TagVector> tags =
                  tagAnalysis.tags(B, *Ptr);
              if (tags.hasValue()) {
                SmallVector<long long, 4> tagVector;
                for (uint64_t t : tags.getValue()) {
                  tagVector.push_back(t);
                }
                Change = setClamProvTags(ctx, *CB, tagVector);
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
