#include "./AddLogging.h"
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
    addLoggingConfig("add-logging-config",
                         cl::desc("Configuration file for logging"),
			 cl::init(""),
                         cl::cat(ClamProvOpts));

namespace clam_prov {

static llvm::StringMap<Value *> functionNameToGlobal;

static int outputMode = -1;
static int maxRecords = -1;
static const StringRef functionNameInit("clam_prov_logging_init");
static const StringRef functionNameShutdown("clam_prov_logging_shutdown");
static const StringRef functionNameBuffer("clam_prov_logging_buffer");

static bool loadConfiguration(Module &M, std::string filePath);

static bool loadConfiguration(Module &M, std::string filePath) {
  if (filePath.empty()) {
    //errs() << "Empty configuration file path\n";
    return false;
  }
  std::ifstream file(filePath);
  if (!file.good()) {
    errs() << "Failed to read file '" << filePath << "'\n";
    return false;
  }
  std::string line;
  while (std::getline(file, line)) {
    StringRef lineRef(line);
    lineRef = lineRef.ltrim(' ');
    if (lineRef.startswith("#") || lineRef.empty()) {
      continue;
    }
    SmallVector<StringRef, 2> tokens;
    lineRef.split(tokens, '=', 2, true);
    if (tokens.size() == 2) {
      StringRef key;
      key = tokens[0].trim();
      if (key == "output_mode") {
        APInt valueInt;
        StringRef value;
        value = tokens[1].trim();
        if (value.getAsInteger(10, valueInt)) {
          errs() << "Skipped line '" << line << "' with non-numeric output_mode value\n";
          continue;
        }
        outputMode = valueInt.getSExtValue();
      }else if (key == "max_records") {
        APInt valueInt;
        StringRef value;
        value = tokens[1].trim();
        if (value.getAsInteger(10, valueInt)) {
          errs() << "Skipped line '" << line << "' with non-numeric max_records value\n";
          continue;
        }
        maxRecords = valueInt.getSExtValue();
      }
    }
  }

  if (outputMode < 0) {
    errs() << "Invalid value for output_mode '" << outputMode << "'\n";
    return false;
  }
  if (maxRecords < 0) {
    errs() << "Invalid value for max_records '" << maxRecords << "'\n";
    return false;
  }

  return true;
}

static bool insertLoggerInitInMain(Module &module, Function &function){
  //int clam_prov_logger_init(int control, ...)
  bool updated = false;
  LLVMContext &llvmContext = module.getContext();

  IntegerType *loggerInitResultType = IntegerType::getInt32Ty(llvmContext);
  IntegerType *loggerInitArg0Type = IntegerType::getInt64Ty(llvmContext);
  bool loggerInitIsVarArg = true;
  FunctionType *loggerInitFunctionType = FunctionType::get(loggerInitResultType, loggerInitArg0Type, loggerInitIsVarArg);
  FunctionCallee loggerInitFunctionCallee = module.getOrInsertFunction(functionNameInit, loggerInitFunctionType);
  Function *loggerInitFunction = dyn_cast<Function>(loggerInitFunctionCallee.getCallee());
  loggerInitFunction->setDoesNotThrow();

  BasicBlock &entryBlock = function.getEntryBlock();
  BasicBlock::iterator entryBlockIterator = entryBlock.getFirstInsertionPt();
  Instruction *instruction = &*entryBlockIterator;

  IRBuilder<> instructionBuilder(instruction);

  ConstantInt *constantArg0 = instructionBuilder.getInt64(0);
  ConstantInt *constantArg1 = instructionBuilder.getInt64(maxRecords);
  ConstantInt *constantArg2 = instructionBuilder.getInt64(outputMode);

  CallInst *loggerInitCall = instructionBuilder.CreateCall(loggerInitFunction, {constantArg0, constantArg1, constantArg2});
  updated = true;
  return updated;
}

static bool insertLoggerShutdownInMain(Instruction *current, Module &module){
  //int clam_prov_logger_shutdown(int control, ...)
  bool updated = false;
  LLVMContext &llvmContext = module.getContext();

  IntegerType *loggerShutdownResultType = IntegerType::getInt32Ty(llvmContext);
  IntegerType *loggerShutdownArg0Type = IntegerType::getInt64Ty(llvmContext);
  bool loggerShutdownIsVarArg = true;
  FunctionType *loggerShutdownFunctionType = FunctionType::get(loggerShutdownResultType, loggerShutdownArg0Type, loggerShutdownIsVarArg);
  FunctionCallee loggerShutdownFunctionCallee = module.getOrInsertFunction(functionNameShutdown, loggerShutdownFunctionType);
  Function *loggerShutdownFunction = dyn_cast<Function>(loggerShutdownFunctionCallee.getCallee());
  loggerShutdownFunction->setDoesNotThrow();

  IRBuilder<> instructionBuilder(current);

  ConstantInt *constantArg0 = instructionBuilder.getInt64(0); // unused

  CallInst *loggerShutdownCall = instructionBuilder.CreateCall(loggerShutdownFunction, {constantArg0});
  updated = true;
  return updated;
}

static Value* getFunctionNameVariable(StringRef functionName,
    IRBuilder<> &instructionBuilder, LLVMContext &llvmContext, Module &module) {
  if (functionNameToGlobal.find(functionName) == functionNameToGlobal.end()) {
    PointerType *typeCharPointer = PointerType::getUnqual(Type::getInt8Ty(llvmContext));
    Constant *constantString = ConstantDataArray::getString(llvmContext, functionName);
    Constant *constantVariable = module.getOrInsertGlobal(functionName, constantString->getType());
    dyn_cast<GlobalVariable>(constantVariable)->setInitializer(constantString);
    Value *constantValue = instructionBuilder.CreatePointerCast(constantVariable, typeCharPointer, functionName);
    functionNameToGlobal[functionName] = constantValue;
  }
  return functionNameToGlobal[functionName];
}

static bool insertBufferLoggerCall(Instruction *previous, Instruction *current, Function *bufferLoggerFunction, Module &module){
  bool updated = false;
  if (previous == nullptr || current == nullptr) {
    return updated;
  }
  if (CallBase *callBase = dyn_cast<CallBase>(previous)) {
    Function *function = callBase->getCalledFunction();
    if (function == nullptr || !function->hasName()) {
      return updated;
    }
    StringRef functionName = function->getName();
    if (functionName == functionNameInit || functionName == functionNameBuffer) {
      return updated;
    }

    MDNode *callSiteNode = nullptr;
    long long callSiteId;
    if (!getCallSiteMetadata(*callBase, callSiteNode, callSiteId)) {
      return updated;
    }
    LLVMContext &llvmContext = module.getContext();
    IRBuilder<> instructionBuilder(current);

    ConstantInt *controlConstant = instructionBuilder.getInt32(0); // unused
    ConstantInt *callSiteIdConstant = instructionBuilder.getInt64(callSiteId);
    Value *functionNameConstant = getFunctionNameVariable(functionName, instructionBuilder, llvmContext, module);
    CallInst *bufferLoggerCall = instructionBuilder.CreateCall(bufferLoggerFunction, {controlConstant, callSiteIdConstant, previous, functionNameConstant});
    updated = true;
  }
  return updated;
}

bool AddLogging::runOnModule(Module &module) {
  bool updated = false;

  std::string inputFilePath = addLoggingConfig == "" ? "" : addLoggingConfig.getValue().c_str();
  if (!loadConfiguration(module, inputFilePath)) {
    return updated;
  }

  LLVMContext &llvmContext = module.getContext();

  IntegerType *bufferLoggerArg0Type = IntegerType::getInt32Ty(llvmContext);
  IntegerType *bufferLoggerResultType = IntegerType::getInt32Ty(llvmContext);
  bool bufferLoggerIsVarArg = true;
  FunctionType *bufferLoggerFunctionType = FunctionType::get(bufferLoggerResultType, bufferLoggerArg0Type, bufferLoggerIsVarArg);
  FunctionCallee bufferLoggerFunctionCallee = module.getOrInsertFunction(functionNameBuffer, bufferLoggerFunctionType);
  Function *bufferLoggerFunction = dyn_cast<Function>(bufferLoggerFunctionCallee.getCallee());
  bufferLoggerFunction->setDoesNotThrow();

  bool isMainFunction = false;;

  for (Function &function : module) {

    if(function.hasName() && function.getName() == "main"){
      isMainFunction = true;
      updated = insertLoggerInitInMain(module, function);
    }else{
      isMainFunction = false;
    }

    for (BasicBlock &basicBlock : function) {
      Instruction *previous = nullptr;
      for (Instruction &current : basicBlock) {
        // Insert buffer calls conditionally
        bool inserted = insertBufferLoggerCall(previous, &current, bufferLoggerFunction, module);
        updated = updated || inserted;
        previous = &current;

        // Insert shutdown call before all return instructions in the main function.
        if (isMainFunction) {
          if (ReturnInst *returnInst = dyn_cast<ReturnInst>(&current)) {
            updated = insertLoggerShutdownInMain(&current, module);
          }
        }
      }
    }
  }

  return updated;
}

//////////////////////////////////////////////////////////////////////////////

PreservedAnalyses AddLogging::run(llvm::Module &M,
                                   llvm::ModuleAnalysisManager &) {
  bool Changed = runOnModule(M);
  return (Changed ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}

bool LegacyAddLogging::runOnModule(llvm::Module &M) {
  bool Changed = Impl.runOnModule(M);
  return Changed;
}

char LegacyAddLogging::ID = 0;

} // end namespace clam_prov

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getAddLoggingPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "add-logging", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "add-logging") {
                    MPM.addPass(clam_prov::AddLogging());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getAddLoggingPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------

// Register the pass - required for (among others) opt
static RegisterPass<clam_prov::LegacyAddLogging> X(/*PassArg=*/"legacy-add-logging",
						    /*Name=*/"LegacyAddLogging",
						    /*CFGOnly=*/false,
						    /*is_analysis=*/false);
