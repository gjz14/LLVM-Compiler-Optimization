#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Type.h"
#include "231DFA.h"

using namespace llvm;
using namespace std;

class ReachingInfo : public Info{
public:
	std::set<unsigned> Info_set;
	ReachingInfo(){}
	ReachingInfo(const ReachingInfo& other){
		Info_set  = other.Info_set;
	}
	~ReachingInfo() {};

	void print() {
		for(auto info:Info_set){
			errs()<<info<<"|";
		}
		errs()<<"\n";
	}

	static bool equals(ReachingInfo * info1, ReachingInfo * info2){
		return info1.Info_set == info2.Info_set;
	}

	static ReachingInfo* join(ReachingInfo * info1, ReachingInfo * info2, ReachingInfo * result){
		result.Info_set.insert(info1.Info_set.begin(), info1.Info_set.end());
		result.Info_set.insert(info2.Info_set.begin(), info2.Info_set.end());
		return result;
	}

}

class ReachingDefinitionAnalysis : public DataFlowAnalysis <ReachingInfo, true>{
private:
	
public:
	ReachingDefinitionAnalysis(ReachingInfo & bottom, ReachingInfo & initialState) :
    								 DataFlowAnalysis(bottom, initialState) {}

	void flowfunction(Instruction * I,std::vector<unsigned> & IncomingEdges,std::vector<unsigned> & OutgoingEdges,std::vector<ReachingInfo *> & Infos) {

	}
}


namespace {
    struct ReachingDefinitionAnalysisPass : public FunctionPass {
        static char ID;
        ReachingDefinitionAnalysisPass() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            ReachingInfo bottom;
            ReachingInfo initialState;
            ReachingDefinitionAnalysis * RDA = new ReachingDefinitionAnalysis(bottom, initialState);

            RDA->runWorklistAlgorithm(&F);
            RDA->print();

            return false;
        }
    }; // end of struct ReachingDefinitionAnalysisPass
}  // end of anonymous namespace

char ReachingDefinitionAnalysisPass::ID = 0;
static RegisterPass<ReachingDefinitionAnalysisPass> X("cse231-reaching", "Reaching Definition Analysis on CFG",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
