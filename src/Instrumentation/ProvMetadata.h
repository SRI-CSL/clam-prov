#pragma once

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

/**
 * Read provenance metadata at callsites.
 ***/

namespace llvm {
class CallBase;
class MDTuple;
class MDNode;
class LLVMContext;
} // end namespace llvm

namespace clam_prov {
/*
  A.
  Call-site metadata format:
  An MDNode attached to an instruction with the name 'call-site-metadata'.
  The first element in the MDNode is the call-site identifier. Call-site identifier is a numerical identifier for the call-site.
  The second (or the Nth element) is an MDTuple.
  If an argument at the call-site was given a label (according to AddMetadata.config) then there would be an MDTuple for it in the call-site MDNode.
  The first element in the MDTuple is the argument index (starting at 1), and the rest are labels.

  B.
  Clam prov metadata format:
  An MDNode attached to an instruction with the name 'clam-prov-tags' with one or more operands.
  Each operand is a tag identifier.
*/

/*
  Returns the count of argument MDTuples that exist at the given call-site.
  An argument MDTuple is: {<argument index>, <label 1>, ... <label N>}.

  If no argument tuples found or it is not a call-site with metadata then returns '0'.
*/
unsigned int getCallSiteArgumentMetadataCount (const llvm::CallBase &CB);

/*
  Gets the root MDNode (in 'callSiteMetadata') for the call-site (if present) along with the call-site identifier (in 'callSiteId').

  Returns 'false' if failed to get or none existed. Otherwise 'true'.
*/
bool getCallSiteMetadata(const llvm::CallBase &CB, llvm::MDNode *&callSiteMetadata, long long &callSiteId);

/*
  For the given index of argument MDTuple ('argumentMetadataIndex'), returns the:
    1) call-site identifier ('callSiteId')
    2) argument index in the argument MDTuple ('argumentIndex'). Starts at 1. Can be used to access the argument in the instruction.
    3) argument MDTuple

  Returns 'false' if failed to get or none existed. Otherwise 'true'.
*/
bool getCallSiteArgumentMetadata (const unsigned int argumentMetadataIndex, const llvm::CallBase &CB,
                                      long long &callSiteId, unsigned long long &argumentIndex,
                                      llvm::MDTuple *&argumentMetadata);

/*
  Checks in the argument MDTuple ('argumentMetadata') if any of the labels is 'input', or 'output'.
  If only 'input' label exists then 'isInput' set to 'true', and returns 'true'.
  If only 'output' label exists then 'isInput' set to 'false', and returns 'true'.
  Otherwise, doesn't set 'isInput', and returns 'false'.
*/
bool getArgumentMetadataType (llvm::MDTuple *argumentMetadata, bool &isInput);

/*
  Convenience function.

  Checks if there is call-site metadata. If yes, then iterates all the argument MDTuples.
  For each argument MDTuple, checks if it is of type 'input' or 'output' according to function 'getArgumentMetadataType'.
  If the argument MDTuple is of type 'input', or 'output' then gets the argument index ('argumentIndex') in it, and doesn't check the rest of the argument MDTuples.
  'callSiteId' is populated from the call-site metadata, and 'isInput' is set by the function 'getArgumentMetadataType'.

  'argumentIndex' is the call-site's parameter starting at 1.
  'isInput' is true if the call should be considered as a source, otherwise it should be a sink.

  Returns 'false' if failed to get or none existed. Otherwise 'true'.
*/
bool getCallSiteMetadataAndFirstArgumentType (const llvm::CallBase &CB,
                                         long long &callSiteId, unsigned long long &argumentIndex, bool &isInput);

/*
  Sets the call-site metadata.
  'callSiteId' - The call-site identifier
  'paramToLabel' - A map where the key is the argument index, and the value is the vector of labels for that argument

  Always returns 'true' i.e. overwrites if any metadata previously existed.
*/
bool setCallSiteMetadata(llvm::LLVMContext &ctx, llvm::CallBase &CB,
                          long long callSiteId, llvm::StringMap<llvm::SmallVector<std::string *, 4>> paramToLabel);

/*
  Gets the clam prov tags (if any) in the vector 'tags'

  Returns 'false' if failed to get or none existed. Otherwise 'true'.
*/
bool getClamProvTags (const llvm::CallBase &CB, llvm::SmallVectorImpl<long long> &tags);

/*
  Sets the clam prov tags in the vector 'tags'.

  Always returns 'true' i.e. overwrites if any metadata previously existed.
*/
bool setClamProvTags (llvm::LLVMContext &ctx, llvm::CallBase &CB, llvm::SmallVectorImpl<long long> &tags);
} // end namespace clam_prov
