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
struct CountDynamicInstructions  : public FunctionPass {
  static char ID;
  
  CountDynamicInstructions () : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
  	Module *mod = F.getParent();
    LLVMContext &context = mod->getContext();
    Function * updateInstrInfo = cast<Function>(mod->getOrInsertFunction("updateInstrInfo",
                                                                          Type::getVoidTy(context),
                                                                          Type::getInt32Ty(context),
                                                                          Type::getInt32PtrTy(context),
                                                                          Type::getInt32PtrTy(context)));
    Function * printOutInstrInfo = cast<Function>(mod->getOrInsertFunction("printOutInstrInfo",
                                                                          Type::getVoidTy(context)
                                                                          ));
    for (Function::iterator B = F.begin(), BE = F.end(); B != BE; ++B) {
    // Here B is a pointer to a basic block
      // count staticly
      std::map <int,int> hash;
      for ( BasicBlock::iterator I = B->begin(), IE = B->end(); I!=IE; ++I){
        hash[I->getOpcode()]++;
      }
     
      std::vector<uint32_t> keys;
      std::vector<uint32_t> values;
      size_t num = 0;
      for(std::map<int,int>::iterator it = hash.begin(); it!=hash.end(); ++it){
        keys.push_back(it->first);
        values.push_back(it->second);
        num++;
      }
      //define arrayTy
      ArrayType* my_array = ArrayType::get(IntegerType::get(context, 32), num);

      // global variables for keys and values
      GlobalVariable* mykeys = new GlobalVariable(
        *mod,
        my_array,
        true,
        GlobalValue::InternalLinkage,
        ConstantDataArray::get(context, *(new ArrayRef<uint32_t>(keys))),
        "keys_global");

      GlobalVariable* myvalues = new GlobalVariable(
        *mod,
        my_array,
        true,
        GlobalValue::InternalLinkage,
        ConstantDataArray::get(context, *(new ArrayRef<uint32_t>(values))),
        "values_global");

      // create IRBuilder
      IRBuilder<> Builder(&(B->back()));
      

      //prepare arguments for the function call
      std::vector<Value*> args;
      Value* keyin = Builder.CreatePointerCast(mykeys, Type::getInt32PtrTy(context));
      Value* valuein = Builder.CreatePointerCast(myvalues, Type::getInt32PtrTy(context));
      args.push_back(ConstantInt::get(Type::getInt32Ty(context), num));
      args.push_back(keyin);
      args.push_back(valuein);

      //call updateInstrInfo function
      Builder.CreateCall(updateInstrInfo, args);

      //find where the return instruction is and call printOutInstrInfo function
      for(BasicBlock::iterator I = B->begin(), IE = B->end(); I!=IE; ++I){
        if(std::string(I->getOpcodeName()) == "ret"){
          Builder.SetInsertPoint(&*I);
          Builder.CreateCall(printOutInstrInfo);
        }
      }

    }
    return false;
    
  }
}; // end of struct CountDynamicInstructions 
}  // end of anonymous namespace

char CountDynamicInstructions ::ID = 0;
static RegisterPass<CountDynamicInstructions > X("cse231-cdi", "count dynamic instruction counts",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
