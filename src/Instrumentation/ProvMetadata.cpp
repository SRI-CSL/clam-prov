#include "./ProvMetadata.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"

using namespace llvm;

namespace clam_prov {

static StringRef metadataKeyCallSite("call-site-metadata");
static StringRef keyMetadataClamProv("clam-prov-tags");

static ConstantAsMetadata* getIntegerAsMetadata(llvm::LLVMContext &ctx, long long value){
  IntegerType *int64Ty = Type::getInt64Ty(ctx);
  Constant *c = cast<Constant>(ConstantInt::get(int64Ty, value));
  ConstantAsMetadata *cMetadata = ConstantAsMetadata::get(c);
  return cMetadata;
}

static bool getIntegerFromMetadata(const Metadata *metadata, long long &value){
  if (const ConstantAsMetadata *constantMetadata = dyn_cast<ConstantAsMetadata>(metadata)) {
    if (ConstantInt *constantInt = dyn_cast<ConstantInt>(constantMetadata->getValue())) {
      value = constantInt->getValue().getZExtValue();
      return true;
    }
  }
  return false;
}

unsigned int getCallSiteArgumentMetadataCount (const llvm::CallBase &CB) {
  if (MDNode *callSiteNode = CB.getMetadata(metadataKeyCallSite)) {
    unsigned int result = callSiteNode->getNumOperands();
    /*
       In the `metadataKeyCallSite` metadata node:
         1) There is one operand for call-site identifier, and
         2) One MDTuple operand per argument
    */
    if(result > 0){ // Sanity check for the case when even the operand for call-site identifier didn't exist
      return result - 1;
    }
  }
  return 0;
}

bool getCallSiteMetadata(const llvm::CallBase &CB, llvm::MDNode *&callSiteMetadata, long long &callSiteId){
  MDNode *callSiteNode = CB.getMetadata(metadataKeyCallSite);
  if (callSiteNode != nullptr && callSiteNode->getNumOperands() > 0) {
    if (getIntegerFromMetadata(callSiteNode->getOperand(0), callSiteId)) {
      callSiteMetadata = callSiteNode;
      return true;
    }
  }
  return false;
}

bool getCallSiteArgumentMetadata (const unsigned int argumentMetadataIndex, const llvm::CallBase &CB,
                                      long long &callSiteId, unsigned long long &argumentIndex,
                                      llvm::MDTuple *&argumentMetadata) {
  MDNode *callSiteNode = nullptr;
  if (getCallSiteMetadata(CB, callSiteNode, callSiteId) && (argumentMetadataIndex + 1) < callSiteNode->getNumOperands()) {
      // Confirm that the operand at index+1 (+1 because first is MDString) is MDTuple - argument tuple
    MDTuple *argTuple = dyn_cast<MDTuple>(callSiteNode->getOperand(argumentMetadataIndex + 1));
    if (argTuple != nullptr && argTuple->getNumOperands() > 0) {
      MDString *argumentIndexString = dyn_cast<MDString>(argTuple->getOperand(0));
      if (argumentIndexString != nullptr && !getAsUnsignedInteger(argumentIndexString->getString(), 10, argumentIndex)) {
        argumentMetadata = argTuple;
        return true;
      }
    }
  }
  return false;
}

// input or output
bool getArgumentMetadataType (llvm::MDTuple *argumentMetadata, bool &isInput) {
  if (argumentMetadata != nullptr){
    int indexInput = -1, indexOutput = -1;
    unsigned int argTupleSize = argumentMetadata->getNumOperands();
    for (unsigned int i = 1; i < argTupleSize; i++) { // start from 1 because 0 is argument index
      if (MDString *labelString = dyn_cast<MDString>(argumentMetadata->getOperand(i))) {
        if (labelString->getString() == "input") {
          indexInput = i;
        }
        if (labelString->getString() == "output") {
          indexOutput = i;
        }
      }
    }

    if (indexInput > -1 && indexOutput == -1) { // only had input
      isInput = true;
      return true;
    } else if (indexInput == -1 && indexOutput > -1) { // only had output
      isInput = false;
      return true;
    }
  }
  return false;
}

bool getCallSiteMetadataAndFirstArgumentType (const llvm::CallBase &CB,
                                         long long &callSiteId, unsigned long long &argumentIndex, bool &isInput) {
  unsigned int argumentMetadataCount = getCallSiteArgumentMetadataCount(CB);
  for (unsigned int i = 0; i < argumentMetadataCount; i++) {
    MDTuple *argumentTuple = nullptr;
    if (getCallSiteArgumentMetadata(i, CB, callSiteId, argumentIndex, argumentTuple)) {
      if (getArgumentMetadataType(argumentTuple, isInput)) {
        // got the first valid one
        return true;
      }
    }
  }
  return false;
}

bool setCallSiteMetadata(llvm::LLVMContext &ctx, llvm::CallBase &CB,
                          long long callSiteId, llvm::StringMap<llvm::SmallVector<char *, 4>> paramToLabel){
  std::vector<Metadata *> callSiteTuple;
  callSiteTuple.push_back(getIntegerAsMetadata(ctx, callSiteId));

  for (auto const &entry : paramToLabel) {
    std::vector<Metadata *> mdList;
    llvm::SmallVectorImpl<char *> *list = &paramToLabel[entry.first()];
    mdList.push_back(MDString::get(ctx, entry.first()));
    for (char *strPtr : *list) {
      mdList.push_back(MDString::get(ctx, strPtr));
    }
    MDTuple *paramTuple = MDTuple::get(ctx, mdList);
    callSiteTuple.push_back(paramTuple);
  }

  MDNode *callSiteNode = MDNode::get(ctx, callSiteTuple);
  CB.setMetadata(metadataKeyCallSite, callSiteNode);
  return true;
}

bool getClamProvTags (const llvm::CallBase &CB, llvm::SmallVectorImpl<long long> &tags) {
  MDNode *mdNodeClamProv = CB.getMetadata(keyMetadataClamProv);
  if (mdNodeClamProv != nullptr) {
    if (mdNodeClamProv->getNumOperands() > 0) {
      MDNode::op_iterator clamProvIterator = mdNodeClamProv->op_begin();
      MDNode::op_iterator clamProvIteratorEnd = mdNodeClamProv->op_end();
      while (clamProvIterator != clamProvIteratorEnd) {
        long long value;
        if (getIntegerFromMetadata((&*clamProvIterator)->get(), value)) {
            tags.push_back(value);
        }
        clamProvIterator++;
      }
      return tags.size() > 0;
    }
  }
  return false;
}

bool setClamProvTags (llvm::LLVMContext &ctx, llvm::CallBase &CB, llvm::SmallVectorImpl<long long> &tags) {
  SmallVector<Metadata *, 4> tagOperands;
  for (long long t : tags) {
    ConstantAsMetadata *cMetadata = getIntegerAsMetadata(ctx, t);
    tagOperands.push_back(cMetadata);
  }
  MDNode *tagNode = MDNode::get(ctx, tagOperands);
  CB.setMetadata(keyMetadataClamProv, tagNode);
  return true;
}

}
