
/** Insert dummy main function if one does not exist */

#include "DummyMainFunction.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/Debug.h"

#include "boost/format.hpp"

using namespace llvm;

namespace clam_prov {

FunctionCallee DummyMainFunction::makeNewNondetFn(Module &m, Type &type, unsigned num, std::string prefix) {
  std::string name;
  unsigned c = num;
  do
    name = boost::str(boost::format(prefix + "%d") % (c++));
  while (m.getNamedValue(name));
  return m.getOrInsertFunction(name, &type);
}
  
FunctionCallee DummyMainFunction::getNondetFn(Type *type, Module& M) {
  auto it = m_ndfn.find(type);
  if (it == m_ndfn.end()) {
    FunctionCallee fn = makeNewNondetFn(M, *type, m_ndfn.size(), "verifier.nondet.");
    m_ndfn.insert(std::make_pair(type, fn));
    return fn;
  } else {
    return it->second;
  }
}
  

DummyMainFunction::DummyMainFunction(std::string entryPoint)
  : m_entryPoint(entryPoint) {
}

bool DummyMainFunction::runOnModule(Module &M) {
      
  if (M.getFunction("main")) { 
    errs() << "DummyMainFunction: Main already exists.\n";
    return false;
  }      
  
  Function* entry = nullptr;
  if (m_entryPoint != "") {
    entry = M.getFunction(m_entryPoint);
  }
  
  // --- Create main
  LLVMContext &ctx = M.getContext();
  Type* intTy = Type::getInt32Ty(ctx);
  
  ArrayRef <Type*> params;
  Function *main = Function::Create(FunctionType::get(intTy, params, false), 
				    GlobalValue::LinkageTypes::ExternalLinkage, 
				    "main", &M);
  
  IRBuilder<> B(ctx);
  BasicBlock *BB = BasicBlock::Create(ctx, "", main);
  B.SetInsertPoint(BB, BB->begin());
  
  std::vector<Function*> FunctionsToCall;
  if (entry) {  
    FunctionsToCall.push_back(entry);
  } else { 
    // --- if no selected entry found then we call to all
    //     non-declaration functions.
    for (auto &F: M) {
      if(F.getName() == "main") { // avoid recursive call to main
	continue;
      } 
      if (F.isDeclaration()) {
	continue;
      }
      FunctionsToCall.push_back(&F);
    }
  }
  
  for (auto F: FunctionsToCall) {
    // -- create a call with non-deterministic actual parameters
    SmallVector<Value*, 16> Args;
    for (auto &A : F->args()) {
      FunctionCallee ndf = getNondetFn(A.getType(), M);
      Args.push_back(B.CreateCall(ndf));
    }
    CallInst* CI = B.CreateCall(F, Args);
    errs() << "DummyMainFunction: created a call " << *CI << "\n";
  }
  
  // -- return of main
  B.CreateRet(ConstantInt::get(intTy, 0));
  return true;
}

} // end namespace clam_prov
      
   
