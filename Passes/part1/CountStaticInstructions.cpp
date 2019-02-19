#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <string>

using namespace llvm;

namespace {
struct CountStaticInstruction : public FunctionPass {
  static char ID;
  std::map <std::string,int> hash;
  CountStaticInstruction() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
  	for(Function::iterator B = F.begin(), BE = F.end(); B!=BE;++B){
  		for ( BasicBlock::iterator I = B->begin(), IE = B->end(); I!=IE; ++I){
			if(hash.find(I->getOpcodeName()) == hash.end()){
				hash[I->getOpcodeName()] = 1;
			}
			else{
  				hash[I->getOpcodeName()]++;
			}
  		}
  	}
  	for(std::map <std::string,int>::iterator it = hash.begin(); it!=hash.end(); ++it){
  		errs()<<it->first<<"\t"<<it->second<<"\n";
  	}
	//errs()<<"\n";
  	hash.clear();
  	return false;
    
  }
}; // end of struct submission_pt1
}  // end of anonymous namespace

char CountStaticInstruction::ID = 0;
static RegisterPass<CountStaticInstruction> X("cse231-csi", "count static instruction counts",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
