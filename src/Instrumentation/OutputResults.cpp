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
          SmallVector<long long, 4> tagVector;

          unsigned int argumentMetadataCount = getCallSiteArgumentMetadataCount(*CB);
          for (int i = 0; i < argumentMetadataCount; i++) {
            MDTuple *argumentMetadata;
            if (getCallSiteArgumentMetadata(i, *CB, callId, callArg, argumentMetadata)) {
              if (getArgumentMetadataType(argumentMetadata, isInput)) {
                if (!isInput) {
                  Value *Ptr = CB->getArgOperand(callArg - 1);
                  llvm::Optional<clam::ClamQueryAPI::TagVector> tags = tagAnalysis.tags(B, *Ptr);
                  if (tags.hasValue()) {
                    for (uint64_t t : tags.getValue()) {
                      tagVector.push_back(t);
                    }
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

          if (tagVector.size() > 0) {
            Change = setClamProvTags(ctx, *CB, tagVector);
          }

        }
      }
    }
  }
  return Change;
}
} // end namespace clam_prov
