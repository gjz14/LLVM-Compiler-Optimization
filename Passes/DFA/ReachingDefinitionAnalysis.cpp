#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Type.h"
#include "231DFA.h"
#include <set>


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
		return info1->Info_set == info2->Info_set;
	}

	static ReachingInfo* join(ReachingInfo * info1, ReachingInfo * info2, ReachingInfo * result){
		result->Info_set.insert(info1->Info_set.begin(), info1->Info_set.end());
		result->Info_set.insert(info2->Info_set.begin(), info2->Info_set.end());
		return result;
	}

};

class ReachingDefinitionAnalysis : public DataFlowAnalysis <ReachingInfo, true>{
private:
//	std::map<Edge, ReachingInfo *> EdgeToInfo;
		// The bottom of the lattice
//	ReachingInfo Bottom;
		// The initial state of the analysis
//	ReachingInfo InitialState;

public:
	std::map<std::string, int> categorymap = {{"br",2},{"switch",2},{"alloca",1},{"load",1},{"store",2},{"getelementptr",1},{"icmp",1},{"fcmp",1},{"phi",3},{"select",1}};
	ReachingDefinitionAnalysis(ReachingInfo & bottom, ReachingInfo & initialState) :
    								 DataFlowAnalysis(bottom, initialState) {}

	void flowfunction(Instruction * I,std::vector<unsigned> & IncomingEdges,std::vector<unsigned> & OutgoingEdges,std::vector<ReachingInfo *> & Infos) {
		unsigned index = InstrToIndex[I];
		std::string opName = I->getOpcodeName();
		int category;
		if(I->isBinaryOp())
			category = 1;
		else{
			category = categorymap[opName];
		}
		ReachingInfo * new_Info = new ReachingInfo();
		for(unsigned i=0;i<IncomingEdges.size();i++)
			ReachingInfo::join(EdgeToInfo[std::make_pair(IncomingEdges[i],index)], new_Info, new_Info);
		switch(category){
			case 1:{
				new_Info->Info_set.insert(index);
				break;
			}
			case 2:
			
			case 3:{
				Instruction * firstNonPHI = I->getParent()->getFirstNonPHI();
				unsigned firstNonPHIindex = InstrToIndex[firstNonPHI];
				for(unsigned i=index;i<firstNonPHIindex;i++){
					new_Info->Info_set.insert(i);
				}
				break;
			}
		}
		for(unsigned i=0;i<OutgoingEdges.size();i++)
					Infos.push_back(new_Info);


	}
};


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
