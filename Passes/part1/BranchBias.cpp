#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"
#include <map>
#include <vector>

using namespace llvm;

namespace {
struct BranchBias  : public FunctionPass {
  static char ID;
  
  BranchBias () : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
  	Module *mod = F.getParent();
    LLVMContext &context = mod->getContext();
    Function * updateBranchInfo = cast<Function>(mod->getOrInsertFunction("updateBranchInfo",
                                                                          Type::getVoidTy(context),
                                                                          Type::getInt1Ty(context)
                                                                          ));
    Function * printOutBranchInfo = cast<Function>(mod->getOrInsertFunction("printOutBranchInfo",
                                                                          Type::getVoidTy(context)
                                                                          ));
    for (Function::iterator B = F.begin(), BE = F.end(); B != BE; ++B) {
    // Here B is a pointer to a basic block
      // count staticly
      for(BasicBlock::iterator I = B->begin(), IE = B->end(); I!=IE; ++I){

        if(std::string(I->getOpcodeName()) == "br"){
          BranchInst* brInst = cast<BranchInst>(I);
          if(brInst->isConditional()){
            IRBuilder<> Builder(brInst);
            std::vector<Value*> args;
            args.push_back(brInst->getCondition());
            Builder.CreateCall(updateBranchInfo, args); 
          }
        }
      }
      

      //find where the return instruction is and call printOutInstrInfo function
      for(BasicBlock::iterator I = B->begin(), IE = B->end(); I!=IE; ++I){
        if(std::string(I->getOpcodeName()) == "ret"){
          IRBuilder<> Builder(&*I);
          Builder.CreateCall(printOutBranchInfo);
        }
      }

    }
    return false;
    
  }
}; // end of struct CountDynamicInstructions 
}  // end of anonymous namespace

char BranchBias::ID = 0;
static RegisterPass<BranchBias> X("cse231-bb", "count branches counts",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
