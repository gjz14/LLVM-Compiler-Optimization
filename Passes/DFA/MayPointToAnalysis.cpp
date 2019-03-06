#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Type.h"
#include "231DFA.h"
#include <std::set>


using namespace llvm;
using namespace std;

class MayPointToInfo : public Info{
public:
	std::map<pair<char, unsigned>, std::set<unsigned>> InfoMap;
	MayPointToInfo(){}
	MayPointToInfo(const MayPointToInfo& other){
		 InfoMap  = other. InfoMap;
	}
	~MayPointToInfo() {};

	void print() {
		for (auto ri : InfoMap) {
			if (ri->second.size() == 0)
				continue;
			errs() << ri->first.first << ri->first.second << "->(";

			for (auto pointee : ri->second) {
				errs() << "M" << pointee << "/";
			}
			errs() << ")|";
		}
		errs() << "\n";
	}

	static bool equals(MayPointToInfo * info1, MayPointToInfo * info2){
		return info1-> InfoMap == info2-> InfoMap;
	}

	static MayPointToInfo* join(MayPointToInfo * info1, MayPointToInfo * info2, MayPointToInfo * result){
		result-> InfoMap.insert(info1-> InfoMap.begin(), info1-> InfoMap.end());
		result-> InfoMap.insert(info2-> InfoMap.begin(), info2-> InfoMap.end());
		return result;
	}

};

class MayPointToAnalysis : public DataFlowAnalysis <MayPointToInfo, true>{
private:
//	std::map<Edge, MayPointToInfo *> EdgeToInfo;
		// The bottom of the lattice
//	MayPointToInfo Bottom;
		// The initial state of the analysis
//	MayPointToInfo InitialState;

public:
	std::map<std::string, int> categorymap = {{"alloca",1},{"bitcast",2},{"getelementptr",3},{"load",4},{"store",5},{"select",6},{"phi",7}};
	MayPointToAnalysis(MayPointToInfo & bottom, MayPointToInfo & initialState) :
    								 DataFlowAnalysis(bottom, initialState) {}

	void flowfunction(Instruction * I,std::vector<unsigned> & IncomingEdges,std::vector<unsigned> & OutgoingEdges,std::vector<MayPointToInfo *> & Infos) {
		unsigned index = InstrToIndex[I];
		std::string opName = I->getOpcodeName();
		int category = 0;
		if(categorymap.contains(opName))
			category = categorymap[opName];
		
		MayPointToInfo * new_Info = new MayPointToInfo();
		
		// merge incoming edges
		for(unsigned i=0;i<IncomingEdges.size();i++)
			MayPointToInfo::join(EdgeToInfo[std::make_pair(IncomingEdges[i],index)], new_Info, new_Info);

		switch(category){
			case 1:{
				//std::map<pair<char, unsigned>, std::set<unsigned>> tempmap;
				new_Info->InfoMap[make_pair('R',index)].insert(index);
				
			}
			case 2:{
				//std::set<unsigned> X;
				unsigned v = InstrToIndex[(Instruction *) I->getOperand(0)];
				std::set<unsigned> X = new_Info->InfoMap[make_pair('R',v)];
				new_Info->InfoMap[make_pair('R',index)].insert(X.begin(),X.end());

				
			}
			case 3:{
				Value * rv = ((GetElementPtrInst*) I)->getPointerOperand();
				unsigned v = InstrToIndex[(Instruction*) rv];
				std::set<unsigned> X = new_Info->InfoMap[make_pair('R',v)];
				new_Info->InfoMap[make_pair('R',index)].insert(X.begin(),X.end());
				
			}
			case 4:{
				Value * rp = ((LoadInst *) I)->getPointerOperand();
				unsigned p = InstrToIndex[(Instruction*) rp];
				std::set<unsigned> X = new_Info->InfoMap[make_pair('R',p)];
				std::set<unsigned> Y;
				for(auto mi:X){
					std::set<unsigned> tempres = new_Info->InfoMap[make_pair('M',mi)];
					Y.insert(tempres.begin(),temp.end());
				}
				new_Info->InfoMap[make_pair('R'),index].insert(Y.begin(),Y.end());

			}
			case 5:{
				Value* rv = ((StoreInst*)I)->getValueOperand(); 
				Value* rp = ((StoreInst*)I)->getPointerOperand();
				unsigned v = InstrToIndex[(Instruction*) rv];
				unsigned p = InstrToIndex[(Instruction*) rp];
				std::set<unsigned> X = new_Info->InfoMap[make_pair('R',v)];
				std::set<unsigned> Y = new_Info->InfoMap[make_pair('R',p)];
				for(auto mi:Y){
					new_Info->InfoMap[make_pair('M',mi)].insert(X.begin(),X.end());
				}


			}
			case 6:{
				Value* r1 = ((SelectInst*)I)->getTrueValue(); 
				Value* r2 = ((SelectInst*)I)->getFalseValue();
				unsigned i1 =  InstrToIndex[(Instruction*) r1];
				unsigned i2 =  InstrToIndex[(Instruction*) r2];
				std::set<unsigned> X1 = new_Info->InfoMap[make_pair('R',i1)];
				std::set<unsigned> X2 = new_Info->InfoMap[make_pair('R',i2)];
				new_Info->InfoMap[make_pair('R',index)].insert(X1.begin(),X1.end());
				new_Info->InfoMap[make_pair('R',index)].insert(X2.begin(),X2.end());


			}
			case 7:{
				Instruction * firstNonPHI = I->getParent()->getFirstNonPHI();
				unsigned firstNonPHIindex = InstrToIndex[firstNonPHI];
				for(unsigned i=index;i<firstNonPHIindex;i++){
					Instruction * phiInstr_i = IndexToInstr[i];
					for(unsigned j=0;j < phiInstr_i->getNumIncomingValues();j++){
						Instruction * v_ij = (Instruction * ) phiInstr_i->getIncomingValue(j);
						unsigned ij = InstrToIndex[v_ij];
						std::set<unsigned> X  = new_Info->InfoMap[make_pair('R',ij)];
						new_Info->InfoMap[make_pair('R',i)].insert(X.begin(),X.end());
					}
				}

			}
			default:{

			}
		}
		for(unsigned i=0;i<OutgoingEdges.size();i++)
					Infos.push_back(new_Info);


	}
};


namespace {
    struct MayPointToAnalysisPass : public FunctionPass {
        static char ID;
        MayPointToAnalysisPass() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            MayPointToInfo bottom;
            MayPointToInfo initialState;
            MayPointToAnalysis * RDA = new MayPointToAnalysis(bottom, initialState);

            RDA->runWorklistAlgorithm(&F);
            RDA->print();

            return false;
        }
    }; // end of struct MayPointToAnalysisPass
}  // end of anonymous namespace

char MayPointToAnalysisPass::ID = 0;
static RegisterPass<MayPointToAnalysisPass> X("cse231-maypointto", "MayPointToAnalysis on CFG",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
