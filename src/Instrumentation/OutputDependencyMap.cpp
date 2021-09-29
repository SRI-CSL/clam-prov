#include "./OutputDependencyMap.h"
#include "./ProvMetadata.h"

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

  outputFile << "digraph clam_prov_dependency_map{\n";

  LLVMContext &llvmContext = module.getContext();
  for (Function &function : module) {
    for (BasicBlock &basicBlock : function) {
      for (Instruction &instruction : basicBlock) {

        if (CallBase *callBase = dyn_cast<CallBase>(&instruction)) {
          Function *function = callBase->getCalledFunction();
          if (function != nullptr && function->hasName()) {
            // input
            MDNode *callSiteNode = nullptr;
            long long callSiteId;
            if (getCallSiteMetadata(*callBase, callSiteNode, callSiteId)) {
              outputFile << "\"" << callSiteId << "\" [label=\"function name:" << function->getName().data() << "\\ncall site:" << callSiteId << "\"];\n";
            }

            // output
            SmallVector<long long, 16> tagVector;
            if (getClamProvTags(*callBase, tagVector)) {
              for (long long tag : tagVector) {
                outputFile << "\"" << callSiteId << "\" -> \"" << tag << "\" [label=\"WasDependentOn\"];\n";
              }
            }

          }
        }

      }
    }
  }

  outputFile << "}\n";

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
