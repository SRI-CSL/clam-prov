/**
 * Main entry point
 *
 * Run Clam (https://github.com/seahorn/clam) to perform tag analysis
 * (https://github.com/seahorn/clam/wiki/TagAnalysis).
 **/
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"

// Seadsa
#include "seadsa/AllocWrapInfo.hh"
#include "seadsa/DsaLibFuncInfo.hh"
#include "seadsa/support/RemovePtrToInt.hh"

// Clam
#include "clam/CfgBuilder.hh"
#include "clam/Clam.hh"
#include "clam/ClamQueryAPI.hh"
#include "clam/HeapAbstraction.hh"
#include "clam/Passes.hh"
#include "clam/RegisterAnalysis.hh"
#include "clam/SeaDsaHeapAbstraction.hh"
#include "clam/Support/NameValues.hh"

#include "crab/domains/flat_boolean_domain.hpp"
#include "crab/domains/region_domain.hpp"

#include "./Instrumentation/AddMetadata.h"
#include "./Instrumentation/AnnotateSources.h"
#include "./Instrumentation/OutputResults.h"
#include "./Instrumentation/WrapSinks.h"
#include "./Instrumentation/OutputDependencyMap.h"

using namespace clam;
using namespace llvm;

llvm::cl::OptionCategory ClamProvOpts("clam-prov Options");

static llvm::cl::opt<std::string>
    InputFilename(llvm::cl::Positional,
                  llvm::cl::desc("<input LLVM bitcode file>"),
                  llvm::cl::Required, llvm::cl::value_desc("filename"),
                  llvm::cl::cat(ClamProvOpts));

static llvm::cl::opt<std::string>
    OutputFilename("o", llvm::cl::desc("Override output filename"),
                   llvm::cl::init(""), llvm::cl::value_desc("filename"),
                   llvm::cl::cat(ClamProvOpts));

// -- Set up an abstract domain specialized to perform tag analysis
namespace clam {
namespace CrabDomain {
constexpr Type TAG_INTERVALS(1, "intervals", "intervals", false, false);
} // namespace CrabDomain

/* Configuration of the region domain to perform tag analysis */
using domvar_allocator = crab::var_factory_impl::str_var_alloc_col;
using dom_varname_t = domvar_allocator::varname_t;
template <class BaseAbsDom> class RegionParams {
public:
  using number_t = clam::number_t;
  using varname_t = clam::varname_t;
  using varname_allocator_t = domvar_allocator;
  using base_abstract_domain_t = BaseAbsDom;
  using base_varname_t = typename BaseAbsDom::varname_t;
  enum { allocation_sites = 0 };
  enum { deallocation = 0 };
  enum { refine_uninitialized_regions = 0 };
  enum { tag_analysis = 1 };
};

using base_interval_domain_t = crab::domains::flat_boolean_numerical_domain<
    ikos::interval_domain<clam::number_t, dom_varname_t>>;
using tag_analysis_with_interval_domain_t =
    crab::domains::region_domain<RegionParams<base_interval_domain_t>>;

REGISTER_DOMAIN(CrabDomain::TAG_INTERVALS, tag_analysis_with_interval_domain_t)
} // namespace clam

static void preTagAnalysis(Module &M) {
  llvm::legacy::PassManager pm;

  /// === Generic passes ==== ///

  // kill unused internal global
  pm.add(llvm::createGlobalDCEPass());
  pm.add(clam::createRemoveUnreachableBlocksPass());
  // -- promote alloca's to registers
  pm.add(llvm::createPromoteMemoryToRegisterPass());
  // -- ensure one single exit point per function
  pm.add(llvm::createUnifyFunctionExitNodesPass());
  // -- remove unreachable blocks
  pm.add(clam::createRemoveUnreachableBlocksPass());
  // -- remove switch constructions
  pm.add(llvm::createLowerSwitchPass());
  // cleanup after lowering switches
  pm.add(llvm::createCFGSimplificationPass());
  // -- lower constant expressions to instructions
  pm.add(clam::createLowerCstExprPass());
  // cleanup after lowering constant expressions
  pm.add(llvm::createDeadCodeEliminationPass());
  // -- lower ULT and ULE instructions
  pm.add(clam::createLowerUnsignedICmpPass());
  // cleanup unnecessary and unreachable blocks
  pm.add(llvm::createCFGSimplificationPass());
  pm.add(clam::createRemoveUnreachableBlocksPass());
  // -- remove ptrtoint and inttoptr instructions
  pm.add(seadsa::createRemovePtrToIntPass());
  // -- must be the last ones before running crab.
  //pm.add(clam::createLowerSelectPass());
  // -- ensure one single exit point per function
  //    LowerUnsignedICmpPass and LowerSelect can add multiple
  //    returns.
  pm.add(llvm::createUnifyFunctionExitNodesPass());
  pm.add(new clam::NameValues());

  /// === Specific passes for the Tag analysis ==== ///
  pm.add(new clam_prov::LegacyAddMetadata());
  pm.add(new clam_prov::addSources());
  pm.add(new clam_prov::WrapSinks());

  pm.run(M);
}

static void postTagAnalysis(Module &M) {
  llvm::legacy::PassManager pm;
  // -- remove special calls to __CRAB_intrinsic_add_tag, and sea_dsa_set_modified
  pm.add(new clam_prov::removeSources());
  pm.add(new clam_prov::LegacyOutputDependencyMap());
  pm.run(M);
}

// -- Run seadsa: pointer analysis
std::unique_ptr<HeapAbstraction> runSeaDsa(Module &M,
                                           TargetLibraryInfoWrapperPass &TLIW) {
  CallGraph cg(M);
  seadsa::AllocWrapInfo allocWrapInfo(&TLIW);
  allocWrapInfo.initialize(M, nullptr);
  seadsa::DsaLibFuncInfo dsaLibFuncInfo;
  dsaLibFuncInfo.initialize(M);
  std::unique_ptr<HeapAbstraction> mem(new SeaDsaHeapAbstraction(
      M, cg, TLIW, allocWrapInfo, dsaLibFuncInfo, true));
  return std::move(mem);
}

int main(int argc, char *argv[]) {

  llvm::llvm_shutdown_obj shutdown; // calls llvm_shutdown() on exit

  llvm::cl::HideUnrelatedOptions(ClamProvOpts);
  llvm::cl::ParseCommandLineOptions(
      argc, argv, "clam-prov -- Provenance Tracking using Clam\n");

  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);
  llvm::PrettyStackTraceProgram PSTP(argc, argv);
  llvm::EnableDebugBuffering = true;
  std::unique_ptr<llvm::ToolOutputFile> output;

  // Get module from LLVM file
  LLVMContext Context;
  std::error_code error_code;
  SMDiagnostic err;
  std::unique_ptr<Module> module = parseIRFile(InputFilename, err, Context);
  if (!module) {
    if (llvm::errs().has_colors()) {
      llvm::errs().changeColor(llvm::raw_ostream::RED);
    }
    llvm::errs() << "error: "
                 << "Bitcode was not properly read; " << err.getMessage()
                 << "\n";
    if (llvm::errs().has_colors()) {
      llvm::errs().resetColor();
    }
    return 1;
  }

  if (!OutputFilename.empty()) {
    output = std::make_unique<llvm::ToolOutputFile>(
        OutputFilename.c_str(), error_code, llvm::sys::fs::F_None);
  }

  if (error_code) {
    if (llvm::errs().has_colors()) {
      llvm::errs().changeColor(llvm::raw_ostream::RED);
    }
    llvm::errs() << "error: "
                 << "Could not open " << OutputFilename << ": "
                 << error_code.message() << "\n";
    if (llvm::errs().has_colors()) {
      llvm::errs().resetColor();
    }
    return 1;
  }

  const auto &tripleS = module->getTargetTriple();
  Twine tripleT(tripleS);
  Triple triple(tripleT);
  TargetLibraryInfoWrapperPass TLIW(triple);

  /// 1. Optimize and add special instrumentation for the Tag analysis.
  preTagAnalysis(*module);

  /// 2. Translation from LLVM to CrabIR
  std::unique_ptr<HeapAbstraction> mem = runSeaDsa(*module, TLIW);
  CrabBuilderParams cparams;
  cparams.setPrecision(clam::CrabBuilderPrecision::MEM);
  CrabBuilderManager man(cparams, TLIW, std::move(mem));
  /// Set Crab parameters
  AnalysisParams aparams;
  aparams.dom = CrabDomain::TAG_INTERVALS;
  aparams.run_inter = true;
  aparams.store_invariants = true;
  aparams.print_invars = true;
  // disable Clam/Crab warnings
  crab::CrabEnableWarningMsg(false);
  // for debugging only
  // crab::CrabEnableVerbosity(1);
  // to print tags
  crab::CrabEnableLog("region-print");
  /// Create an inter-analysis instance
  std::unique_ptr<InterGlobalClam> clam(new InterGlobalClam(*module, man));

  /// 3. Run the Crab analysis
  ClamGlobalAnalysis::abs_dom_map_t assumptions;
  clam->analyze(aparams, assumptions);

  /// 4. Dump the analysis results as metadata in the bitcode
  clam_prov::TagAnalysisResultsAsMetadata(*module, *clam);

  /// 5. Remove instrumentation added at step 1.
  /// TODO:
  postTagAnalysis(*module);

  if (!OutputFilename.empty()) {
    llvm::legacy::PassManager pm;
    pm.add(createBitcodeWriterPass(output->os()));
    pm.run(*module);
    output->keep();
  }

  return 0;
}
