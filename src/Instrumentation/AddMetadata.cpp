#include "./AddMetadata.h"
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

#define DEBUG_TYPE "add-metadata"
#define STR_ARG_MAX 256

extern cl::OptionCategory ClamProvOpts;

static cl::opt<std::string>
    configFilePathOption("add-metadata-config", cl::Required,
                         cl::desc("Input file for the pass"), cl::ValueRequired,
                         cl::cat(ClamProvOpts));

static cl::opt<std::string>
    configOutputOption("add-metadata-output", cl::init(""), cl::Optional,
                       cl::desc("Output specifier for the pass"),
                       cl::ValueRequired, cl::cat(ClamProvOpts));

namespace clam_prov {

static int outputMode;
static std::ofstream outputFile;
static void writeOutput(long counter, StringRef functionName,
                        std::string *paramKey, char *paramDescription);
static void closeOutput();
static bool openOutput();
static bool loadInputFile(Module &M, std::string inputFilePath);

struct ParamInfo {
  bool useIndex;
  APInt index;
  char name[STR_ARG_MAX];
  char description[STR_ARG_MAX];
};

struct FunctionInfo {
  std::vector<struct ParamInfo> paramInfos;
};

static StringMap<FunctionInfo> functionInfos;

static long callSiteCounter;

static struct FunctionInfo *getFunctionInfo(StringRef functionName) {
  auto it = functionInfos.find(functionName);
  if (it == functionInfos.end()) {
    /// DEBUG
    // errs() << functionName << " not found " << "(" << (void*)functionName.data() << ")\n";
    // for (auto &k: functionInfos.keys()) {
    //   errs() << "\t" << k << "\n";
    // }
    //////
    return nullptr;
  } else {
    return &(it->second);
  }
}

static struct FunctionInfo *initFunctionInfo(StringRef functionName) {
  struct FunctionInfo functionInfo;
  functionInfos[functionName] = functionInfo;
  /// DEBUG 
  // errs() << "Inserted  " << functionName << " (" << (void*) functionName.data() << ")\n";
  ////
  return &functionInfos[functionName];
}

static long getNextCallSiteCounter() {
  return callSiteCounter++;
}

static int copy_str_arg(char *dst, StringRef *src, StringRef msg) {
  const char *tempSrc = src->data();
  int tempI = 0;
  for (; tempI < src->size(); tempI++) {
    *dst++ = *tempSrc++;
    if (tempI >= STR_ARG_MAX - 1) {
      errs() << msg << " length  must be smaller than " << STR_ARG_MAX << ".\n";
      return 0;
    }
  }
  *dst = '\0';
  return 1;
}

static bool loadInputFile(Module &M, std::string inputFilePath) {
  std::ifstream inputFile(inputFilePath);
  if (!inputFile.good()) {
    errs() << "Failed to read file '" << inputFilePath << "'\n";
    return false;
  }

  std::string line;

  while (std::getline(inputFile, line)) {
    StringRef lineRef(line);
    lineRef = lineRef.ltrim(' ');
    if (lineRef.startswith("#") || lineRef.empty()) {
      continue;
    }
    SmallVector<StringRef, 3> tokens;
    lineRef.split(tokens, ',', 3, true);

    if (tokens.size() == 3) {
      StringRef functionName;

      functionName = tokens[0].trim();
      if (functionName.empty()) {
        errs() << "Skipped line '" << line << "' with empty function name\n";
        continue;
      }

      Function *function = M.getFunction(functionName);
      if (!function) {
	errs() << "Ignored " << functionName << " because not defined in module.\n";
	//// DEBUG
	// errs() << "Functions:\n";
	// for (auto &f: M) {
	//   errs() << "\t" << f.getName() << "\n";
	// }
	/////
	// function does not exist in the module
	continue;
      }

      // IMPORTANT: use function->getName instead of functionName
      struct FunctionInfo *functionInfo = getFunctionInfo(function->getName());
      if (functionInfo == nullptr) {
        functionInfo = initFunctionInfo(function->getName());
      }

      struct ParamInfo paramInfo;
      StringRef functionParamName;

      functionParamName = tokens[1].trim();
      if (functionParamName.empty()) {
        errs() << "Skipped line '" << line
               << "' with empty function parameter name/index\n";
        continue;
      }

      if (isdigit(functionParamName
                      .front())) { // it means that we are given an index
        if (functionParamName.getAsInteger(10, paramInfo.index)) {
          errs() << "Skipped line '" << line
                 << "' with non-numeric function parameter index\n";
          continue;
        }
        if (paramInfo.index.sle(0)) {
          errs() << "Skipped line '" << line
                 << "' with non-positive function parameter index\n";
          continue;
        }
        paramInfo.index -= 1;
        paramInfo.useIndex = true;
      } else {
        if (copy_str_arg(&(paramInfo.name[0]), &functionParamName,
                         StringRef("Function parameter name's")) == 0) {
          continue;
        }
        paramInfo.useIndex = false;
      }

      StringRef functionDescription;
      functionDescription = tokens[2].trim();
      if (copy_str_arg(&(paramInfo.description[0]), &functionDescription,
                       StringRef("Function description's")) == 0) {
        continue;
      }
      /* // Let me be empty if i want
      if(functionDescription.empty()){
              errs() << "Skipped line '" << line << "' with empty function
      description\n"; continue;
      }
      */

      functionInfo->paramInfos.push_back(paramInfo);
    } else {
      errs() << "Skipped unexpected line: '" << line
             << "'. Expected format: '<function name>, <parameter name/index> "
                "<description>'\n";
    }
  }
  return true;
}

static bool conditionalUpdate(Instruction *current, Module &module) {
  bool updated = false;
  if (current == nullptr) {
    return updated;
  }

  LLVMContext &llvmContext = module.getContext();

  if (CallBase *callBase = dyn_cast<CallBase>(current)) {
    Function *function = callBase->getCalledFunction();
    if (function == nullptr || !function->hasName()) {
      return updated;
    }
    StringRef functionName = function->getName();
    FunctionInfo *functionInfo = getFunctionInfo(functionName);
    if (functionInfo == nullptr) {
      return updated;
    }

    long counter = getNextCallSiteCounter();

    StringMap<SmallVector<char *, 4>> paramToLabels;

    for (struct ParamInfo &paramInfo : functionInfo->paramInfos) {
      Value *operand = nullptr;
      if (paramInfo.useIndex == true) {
        if (paramInfo.index.uge(callBase->arg_size())) {
          continue; // Skip
        }
        operand = callBase->getArgOperand(paramInfo.index.getZExtValue());
      } else {
        // Use debug info to get the right operand later
      }
      if (operand == nullptr) {
        continue; // skip
      }

      std::string paramKey;

      if (paramInfo.useIndex == true) {
        paramKey = std::to_string(paramInfo.index.getZExtValue() + 1);
        // get param name from debug info later. Using index for now.
      } else {
        paramKey = std::string(&paramInfo.name[0]);
      }

      writeOutput(counter, functionName, &paramKey, &paramInfo.description[0]);

      if (paramToLabels.find(paramKey.c_str()) == paramToLabels.end()) {
        SmallVector<char *, 4> list;
        paramToLabels[paramKey.c_str()] = list;
      }

      SmallVectorImpl<char *> *list = &paramToLabels[paramKey.c_str()];
      list->push_back(&paramInfo.description[0]);
    }

    updated = setCallSiteMetadata(llvmContext, *callBase, counter, paramToLabels);
  }
  return updated;
}

static bool openOutput() {
  std::string configOutput =
      configOutputOption == "" ? "" : configOutputOption.getValue().c_str();
  if (configOutput.empty()) {
    // None
    outputMode = 0;
  } else if (configOutput == "stdout") {
    outputMode = 1;
  } else { // file
    outputMode = 2;
    outputFile.open(configOutput);
    if (!outputFile.good()) {
      errs() << "Invalid output file\n";
      return false;
    }
  }
  return true;
}

static void writeOutput(long counter, StringRef functionName,
                        std::string *paramKey, char *paramDescription) {
  if (outputMode == 0) {
  } else if (outputMode == 1) {
    outs() << counter << "," << functionName << "," << paramKey->c_str() << ","
           << paramDescription << "\n";
  } else if (outputMode == 2) {
    outputFile << counter << "," << functionName.data() << ","
               << paramKey->c_str() << "," << paramDescription << "\n";
  }
}

static void closeOutput() {
  if (outputMode == 0) {
  } else if (outputMode == 1) {
  } else if (outputMode == 2) {
    outputFile.close();
  }
}

bool AddMetadata::runOnModule(Module &module) {
  std::string inputFilePath =
      configFilePathOption == "" ? "" : configFilePathOption.getValue().c_str();
  if (inputFilePath.empty()) {
    errs() << "Must specify argument '-add-metadata-config\n";
    return false;
  }

  if (!loadInputFile(module, inputFilePath)) {
    return false;
  }

  if (!openOutput()) {
    return false;
  }

  bool updated = false;
  LLVMContext &llvmContext = module.getContext();

  for (Function &function : module) {
    for (BasicBlock &basicBlock : function) {
      for (Instruction &current : basicBlock) {
        bool currentUpdate = conditionalUpdate(&current, module);
        updated = updated || currentUpdate;
      }
    }
  }

  closeOutput();

  return updated;
}

//////////////////////////////////////////////////////////////////////////////

PreservedAnalyses AddMetadata::run(llvm::Module &M,
                                   llvm::ModuleAnalysisManager &) {
  bool Changed = runOnModule(M);
  return (Changed ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}

bool LegacyAddMetadata::runOnModule(llvm::Module &M) {
  bool Changed = Impl.runOnModule(M);
  return Changed;
}

char LegacyAddMetadata::ID = 0;

} // end namespace clam_prov

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getAddMetadataPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "add-metadata", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "add-metadata") {
                    MPM.addPass(clam_prov::AddMetadata());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getAddMetadataPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------

// Register the pass - required for (among others) opt
static RegisterPass<clam_prov::LegacyAddMetadata> X(/*PassArg=*/"legacy-add-metadata",
						    /*Name=*/"LegacyAddMetadata",
						    /*CFGOnly=*/false,
						    /*is_analysis=*/false);
