#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Type.h"
#include "231DFA.h"
#include <set>


using namespace llvm;
using namespace std;

class LivenessInfo : public Info{
public:
	std::set<unsigned> Info_set;
	LivenessInfo(){}
	LivenessInfo(const LivenessInfo& other){
		Info_set  = other.Info_set;
	}
	~LivenessInfo() {};

	void print() {
		for(auto info:Info_set){
			errs()<<info<<"|";
		}
		errs()<<"\n";
	}

	static bool equals(LivenessInfo * info1, LivenessInfo * info2){
		return info1->Info_set == info2->Info_set;
	}

	static LivenessInfo* join(LivenessInfo * info1, LivenessInfo * info2, LivenessInfo * result){
		result->Info_set.insert(info1->Info_set.begin(), info1->Info_set.end());
		result->Info_set.insert(info2->Info_set.begin(), info2->Info_set.end());
		return result;
	}

};

class  LivenessAnalysis : public DataFlowAnalysis <LivenessInfo, false>{
private:
//	std::map<Edge, LivenessInfo *> EdgeToInfo;
		// The bottom of the lattice
//	LivenessInfo Bottom;
		// The initial state of the analysis
//	LivenessInfo InitialState;

public:
	std::map<std::string, int> categorymap = {{"br",2},{"switch",2},{"alloca",1},{"load",1},{"store",2},{"getelementptr",1},{"icmp",1},{"fcmp",1},{"phi",3},{"select",1}};
	LivenessAnalysis(LivenessInfo & bottom, LivenessInfo & initialState) :
    								 DataFlowAnalysis(bottom, initialState) {}
	void flowfunction(Instruction * I,std::vector<unsigned> & IncomingEdges,std::vector<unsigned> & OutgoingEdges,std::vector<LivenessInfo *> & Infos){
		unsigned index = InstrToIndex[I];
		std::string opName = I->getOpcodeName();
		int category;
		// if it is a binary operation, it belongs to category I
		if(I->isBinaryOp())
			category = 1;
		else{
			category = categorymap[opName];
		}
		LivenessInfo * new_Info = new LivenessInfo();
		for(unsigned i=0;i<IncomingEdges.size();i++)
			LivenessInfo::join(EdgeToInfo[std::make_pair(IncomingEdges[i],index)], new_Info, new_Info);

		std::set<unsigned> operands;
		for(unsigned i=0;i<I->getNumOperands();i++){
			Instruction * ins = (Instruction *) I->getOperand(i);
			if(InstrToIndex.count(ins)>0)
				operands.insert(InstrToIndex[ins]);
		}
		switch(category){
			case 1:{
				//new_Info->Info_set.insert(index);
				new_Info->Info_set.insert(operands.begin(),operands.end());
				new_Info->Info_set.erase(index);

				for(unsigned i=0;i<OutgoingEdges.size();i++)
					Infos.push_back(new_Info);
				break;
			}
			case 2:{
				new_Info->Info_set.insert(operands.begin(),operands.end());	

				for(unsigned i=0;i<OutgoingEdges.size();i++)
					Infos.push_back(new_Info);
				break;
			}
			case 3:{
				
				Instruction * firstNonPHI = I->getParent()->getFirstNonPHI();
				unsigned firstNonPHIindex = InstrToIndex[firstNonPHI];

				for (unsigned k=0; k<OutgoingEdges.size(); k++){
					// join the incoming
					LivenessInfo * out_k = new LivenessInfo();
					LivenessInfo::join(new_Info,  out_k, out_k);
					
					for(unsigned i=index;i<firstNonPHIindex;i++){
						Instruction * phiInstr_i = IndexToInstr[i];

						// exclude variables defined at each phi instruction
						out_k->Info_set.erase(i);

						// iterate over values of the phi instruction

						for(unsigned j=0;j < ((PHINode *) phiInstr_i)->getNumIncomingValues();j++){

							// caculate ValuetoInstr(v_ij)
							Instruction * v_ij =  (Instruction *) ( ((PHINode *)  phiInstr_i)->getIncomingValue(j) );
							if(InstrToIndex.count(v_ij)==0)
								continue;

							// get the basic block where v_ij is defined
							BasicBlock* label_ij = ((PHINode *) phiInstr_i)->getIncomingBlock(j);
		                	 	
							//unsigned out_k = OutgoingEdges[k];    

							// if v_ij is defined in its matching basic block, join v_ij  	
		 	           		if(IndexToInstr[OutgoingEdges[k]]->getParent() == label_ij) 
								out_k->Info_set.insert(InstrToIndex[v_ij]);	
		            		
						}
					}
					Infos.push_back(out_k);
				}
				break;

			}
		}
		
	}
	
};


namespace {
    struct LivenessAnalysisPass : public FunctionPass {
        static char ID;
        LivenessAnalysisPass () : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            LivenessInfo bottom;
            LivenessInfo initialState;
            LivenessAnalysis * RDA = new LivenessAnalysis(bottom, initialState);

            RDA->runWorklistAlgorithm(&F);
            RDA->print();

            return false;
        }
    }; // end of struct ReachingDefinitionAnalysisPass
}  // end of anonymous namespace

char LivenessAnalysisPass::ID = 0;
static RegisterPass<LivenessAnalysisPass> X("cse231-liveness", "Liveness Analysis on CFG",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
