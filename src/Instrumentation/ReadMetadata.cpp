#include "./ReadMetadata.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"

using namespace llvm;

namespace clam_prov {

bool extractProvenanceFromCallsite(const CallBase &CB, long long &callId,
                                   unsigned long long &callArg, bool &isInput) {
  MDString *ms0, *ms1, *ms2;
  if (MDNode *md = CB.getMetadata("call-site-metadata")) {
    ms0 = dyn_cast<MDString>(md->getOperand(0));
    if (!ms0) {
      return false;
    }
    if (MDNode *mdNext = dyn_cast<MDNode>(md->getOperand(1))) {
      ms1 = dyn_cast<MDString>(mdNext->getOperand(0));
      if (!ms1) {
        return false;
      }
      ms2 = dyn_cast<MDString>(mdNext->getOperand(1));
      if (!ms2) {
        return false;
      }
    }
  } else {
    return false;
  }

  assert(ms0);
  assert(ms1);
  assert(ms2);

  bool ok = getAsSignedInteger(ms0->getString(), 10, callId);
  if (ok) {
    // error: empty or overflow
    return false;
  }
  ok = getAsUnsignedInteger(ms1->getString(), 10, callArg);
  if (ok) {
    // error: empty or overflow
    return false;
  }
  if (ms2->getString() != "input" && ms2->getString() != "output") {
    return false;
  }

  isInput = ms2->getString() == "input";
  return true;
}

} // end namespace clam_prov
