#pragma once

/**
 * Read provenance metadata at callsites.
 ***/

namespace llvm {
class CallBase;
} // end namespace llvm

namespace clam_prov {
// callId is a numerical identifier for the callsite.
// callArg is the callsite's parameter starting at 1.
// isInput is true if the call should be considered as a source,
// otherwise it should be a sink.
bool extractProvenanceFromCallsite(const llvm::CallBase &CB, long long &callId,
                                   unsigned long long &callArg, bool &isInput);
} // end namespace clam_prov
