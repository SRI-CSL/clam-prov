#include "OutputDependencyMap.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/CommandLine.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace llvm;

extern cl::OptionCategory ClamProvOpts;

static cl::opt<std::string>
    dependencyMapFile("dependency-map-file",
                         cl::desc("Output file to write the dependency map to"),
			 cl::init(""),
                         cl::cat(ClamProvOpts));

namespace clam_prov {

static StringRef keyMetadataCallSite("call-site-metadata");
static StringRef keyMetadataClamProv("clam-prov-tags");

bool OutputDependencyMap::runOnModule(Module &module) {
  if (dependencyMapFile.empty()) {
    return false;
  }

  std::ofstream outputFile;
  outputFile.open(dependencyMapFile);
  if(!outputFile.good()){
    errs() << "Failed to open output dependency map file '" << dependencyMapFile << "'\n";
    return false;
  }

  LLVMContext &llvmContext = module.getContext();
  for (Function &function : module) {
    for (BasicBlock &basicBlock : function) {
      for (Instruction &instruction : basicBlock) {

        // input
        MDNode *mdNodeCallSite = instruction.getMetadata(keyMetadataCallSite);
        if (mdNodeCallSite != nullptr) {
          if (mdNodeCallSite->getNumOperands() > 0) {
            if (CallBase *callBase = dyn_cast<CallBase>(&instruction)) {
              Function *function = callBase->getCalledFunction();
              if (function != nullptr && function->hasName()) {
                StringRef functionName = function->getName();
                MDNode::op_iterator callSiteIterator = mdNodeCallSite->op_begin();
                if (MDString *callSiteString = dyn_cast<MDString>(*callSiteIterator)) {
                  APInt callSite;
                  if (!callSiteString->getString().getAsInteger(10, callSite)) {
                    outputFile << "call-site," << functionName.data() << "," << callSite.getZExtValue() << "\n";
                  }
                }
              }
            }
          }
        }

        // output
        MDNode *mdNodeClamProv = instruction.getMetadata(keyMetadataClamProv);
        if (mdNodeClamProv != nullptr) {
          if (mdNodeClamProv->getNumOperands() > 0) {
            if (CallBase *callBase = dyn_cast<CallBase>(&instruction)) {
              Function *function = callBase->getCalledFunction();
              if (function != nullptr && function->hasName()) {
                StringRef functionName = function->getName();
                MDNode::op_iterator clamProvIterator = mdNodeClamProv->op_begin();
                MDNode::op_iterator clamProvIteratorEnd = mdNodeClamProv->op_end();
                outputFile << "tags," << functionName.data();
                while (clamProvIterator != clamProvIteratorEnd) {
                  if (ConstantAsMetadata *mdTag = dyn_cast<ConstantAsMetadata>(*clamProvIterator)) {
                    if (ConstantInt *constantIntTag = dyn_cast<ConstantInt>(mdTag->getValue())) {
                      outputFile << "," << constantIntTag->getValue().getZExtValue();
                    }
                  }
                  clamProvIterator++;
                }
                outputFile << "\n";
              }
            }
          }
        }

      }
    }
  }

  outputFile.close();

  return false;
}

//////////////////////////////////////////////////////////////////////////////

PreservedAnalyses OutputDependencyMap::run(llvm::Module &M,
                                   llvm::ModuleAnalysisManager &) {
  bool Changed = runOnModule(M);
  return (Changed ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}

bool LegacyOutputDependencyMap::runOnModule(llvm::Module &M) {
  bool Changed = Impl.runOnModule(M);
  return Changed;
}

char LegacyOutputDependencyMap::ID = 0;

} // end namespace clam_prov

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getOutputDependencyMapPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "output-dependency-map", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "output-dependency-map") {
                    MPM.addPass(clam_prov::OutputDependencyMap());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getOutputDependencyMapPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------

// Register the pass - required for (among others) opt
static RegisterPass<clam_prov::LegacyOutputDependencyMap> X(/*PassArg=*/"legacy-output-dependency-map",
						    /*Name=*/"LegacyOutputDependencyMap",
						    /*CFGOnly=*/false,
						    /*is_analysis=*/false);
