#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

#include <memory>

namespace clam_prov {
class GetSourcesAndSinksImpl;

using SourceOrSinkMap =
   llvm::DenseMap<const llvm::Function*,
   llvm::SmallVector<const llvm::CallBase*, 16>>;

 
class GetSourcesAndSinks {
  std::unique_ptr<GetSourcesAndSinksImpl> m_impl;
  
 public:
  
  GetSourcesAndSinks();
  ~GetSourcesAndSinks();
  void runOnModule(llvm::Module &M);
  
  SourceOrSinkMap& getSources();
  const SourceOrSinkMap& getSources() const;
  SourceOrSinkMap& getSinks();
  const SourceOrSinkMap& getSinks() const;
  
};

class PrintSourcesAndSinks {
 public:
  PrintSourcesAndSinks();
  ~PrintSourcesAndSinks() = default;
  void runOnModule(llvm::Module &M);
};
 
} // end namespace clam_prov
