
/** 
 * Insert main function if one does not exist.
 */

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

namespace clam_prov {
  
  class DummyMainFunction {
    std::string m_entryPoint;
    llvm::DenseMap<const llvm::Type*, llvm::FunctionCallee> m_ndfn;
    
    llvm::FunctionCallee makeNewNondetFn(llvm::Module &m, llvm::Type &type,
					 unsigned num, std::string prefix);
    llvm::FunctionCallee getNondetFn(llvm::Type *type, llvm::Module& M);
    
   public:
    DummyMainFunction(std::string EntryPoint = "");
    bool runOnModule(llvm::Module &M);
  };

} // end namespace clam_prov
      
   
