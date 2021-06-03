#pragma once

/**
 * Extract results from the Tag Analysis and encode them as LLVM
 * metadata with name "clam-prov-tags"
 **/

#include "clam/Clam.hh"
#include "llvm/IR/Module.h"

namespace clam_prov {

bool TagAnalysisResultsAsMetadata(llvm::Module &M,
                                  clam::InterGlobalClam &tagAnalysis);

} // end namespace clam_prov
