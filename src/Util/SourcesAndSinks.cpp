#include "SourcesAndSinks.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Instructions.h"
#include <set>

using namespace llvm;

namespace clam_prov {

static std::set<std::string> Sources = {
    "read",
    "readv",
    "pread",
    "preadv",
    "recvfrom",
    "mmap"
  };

static std::set<std::string> Sinks = {
    "write",
    "writev",
    "pwrite",
    "pwritev",
    "sendto"
  };

class GetSourcesAndSinksImpl {
  SourceOrSinkMap m_sources;
  SourceOrSinkMap m_sinks;

  
  void addSourceOrSink(const Function *fn, const CallBase *CB,
		       SourceOrSinkMap &map) {
    auto it = map.find(fn);
    if (it != map.end()) {
      it->second.push_back(CB);
    } else {
      SmallVector<const CallBase*, 16> calls;
      calls.push_back(CB);
      map.insert(std::make_pair(fn, calls));
    }
  }
  
 public:

  GetSourcesAndSinksImpl(){}

  ~GetSourcesAndSinksImpl() = default;
  
  void runOnModule(llvm::Module &M) {
    for (Function &F: M) {
      for (BasicBlock &BB: F) {
	for (Instruction &I: BB) {
	  if (CallBase *CB = dyn_cast<CallBase>(&I)) {
	    if (Function *CalleeF = CB->getCalledFunction()) {
	      // FIXME: Expensive operation (string allocation)
	      std::string calleeN = CalleeF->getName().str();
	      if (Sources.count(calleeN) > 0) {
		//errs() << "Found source " << *CB << "\n";
		addSourceOrSink(CalleeF, CB, m_sources);
	      }
	      if (Sinks.count(calleeN) > 0) {
		//errs() << "Found sink " << *CB << "\n";		
		addSourceOrSink(CalleeF, CB, m_sinks);
	      }
	    }
	  }
	}
      }
    }
  }

  SourceOrSinkMap &getSources() {
    return m_sources;
  }
  
  const SourceOrSinkMap &getSources() const {
    return m_sources;
  }
  
  SourceOrSinkMap &getSinks() {
    return m_sinks;
  }

  const SourceOrSinkMap &getSinks() const {
    return m_sinks;
  }
  
};
  
  
GetSourcesAndSinks::GetSourcesAndSinks():
  m_impl(new GetSourcesAndSinksImpl()) {}

GetSourcesAndSinks::~GetSourcesAndSinks() {}
  
void GetSourcesAndSinks::runOnModule(Module &M) {
  m_impl->runOnModule(M);
}

SourceOrSinkMap &GetSourcesAndSinks::getSources() {
  return m_impl->getSources();
}
  
const SourceOrSinkMap &GetSourcesAndSinks::getSources() const {
  return m_impl->getSources();
}

SourceOrSinkMap &GetSourcesAndSinks::getSinks() {
  return m_impl->getSinks();
}

const SourceOrSinkMap &GetSourcesAndSinks::getSinks() const {
    return m_impl->getSinks();
  }
  
PrintSourcesAndSinks::PrintSourcesAndSinks() {}
  
void PrintSourcesAndSinks::runOnModule(Module &M) {
  GetSourcesAndSinks GSS;
  GSS.runOnModule(M);
  auto sourcesMap = GSS.getSources();
  auto sinksMap = GSS.getSinks();

  errs() << "===-------- Sources/Sinks Analysis --------=== \n";
  errs() << "Sources\n";
  for (auto &kv: sourcesMap) {
    errs() << "  " << kv.first->getName() << " " << kv.second.size() << " calls.\n";
  }
  errs() << "Sinks\n";
  for (auto &kv: sinksMap) {
    errs() << "  " << kv.first->getName() << " " << kv.second.size() << " calls.\n";
  }
  errs() << "===----------------------------------------=== \n";  
}  
  
} // end namespace clam_prov
