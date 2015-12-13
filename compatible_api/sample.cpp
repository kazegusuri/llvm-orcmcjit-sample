#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/OrcMCJITReplacement.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>

using namespace llvm;

std::unique_ptr<Module> createTestModule(LLVMContext &context) {
  // `test` モジュールを作成
  std::unique_ptr<Module> M = make_unique<Module>("test", context);

  // `add1` 関数をモジュールに追加
  // 戻り値: int32 引数: int32
  // 0は引数の終端を意味する
  Function *Add1 =
      cast<Function>(M->getOrInsertFunction("add1", Type::getInt32Ty(context),
                                            Type::getInt32Ty(context),
                                            (Type *)0));

  BasicBlock *BB = BasicBlock::Create(context, "EntryBlock", Add1);
  IRBuilder<> builder(BB);

  Value *One = builder.getInt32(1);          // 定数1を取得
  Argument *ArgX = Add1->arg_begin();        // 引数を取得
  Value *Add = builder.CreateAdd(One, ArgX); // 引数と定数1を加算、その結果を取得
  builder.CreateRet(Add);                    // 加算結果をreturnする
  return M;
}

void runFunction_with_mcjit(std::unique_ptr<Module> Owner) {
  auto M = Owner.get();
  EngineBuilder EB(std::move(Owner));
  EB.setMCJITMemoryManager(make_unique<SectionMemoryManager>());
  ExecutionEngine* EE = EB.create();

  Function *F = M->getFunction("add1");
  std::vector<GenericValue> args(1);
  args[0].IntVal = APInt(32, 10);  // 32bit定数10
  GenericValue result = EE->runFunction(F, args);
  outs() << "Result: " << result.IntVal << "\n";
}

void getFunctionAddress_with_mcjit(std::unique_ptr<Module> Owner) {
  auto M = Owner.get();
  EngineBuilder EB(std::move(Owner));
  EB.setMCJITMemoryManager(make_unique<SectionMemoryManager>());
  ExecutionEngine* EE = EB.create();

  Function *F = M->getFunction("add1");
  auto f = reinterpret_cast<int(*)(int)>(EE->getFunctionAddress(F->getName().str()));
  int result = f(10);
  outs() << "Result: " << result << "\n";
}

void getPointerToFunction_with_mcjit(std::unique_ptr<Module> Owner) {
  auto M = Owner.get();
  EngineBuilder EB(std::move(Owner));
  EB.setMCJITMemoryManager(make_unique<SectionMemoryManager>());
  ExecutionEngine* EE = EB.create();

  Function *F = M->getFunction("add1");
  auto f = reinterpret_cast<int(*)(int)>(EE->getPointerToFunction(F));
  int result = f(10);
  outs() << "Result: " << result << "\n";
}

void runFunction_with_orcmcjit(std::unique_ptr<Module> Owner) {
  auto M = Owner.get();
  EngineBuilder EB(std::move(Owner));
  EB.setMCJITMemoryManager(make_unique<SectionMemoryManager>());
  EB.setUseOrcMCJITReplacement(true); // use OrcMCJIT
  ExecutionEngine* EE = EB.create();

  Function *F = M->getFunction("add1");
  std::vector<GenericValue> args(1);
  args[0].IntVal = APInt(32, 10);  // 32bit定数10
  GenericValue result = EE->runFunction(F, args);
  outs() << "Result: " << result.IntVal << "\n";
}

void getFunctionAddress_with_orcmcjit(std::unique_ptr<Module> Owner) {
  auto M = Owner.get();
  EngineBuilder EB(std::move(Owner));
  EB.setMCJITMemoryManager(make_unique<SectionMemoryManager>());
  EB.setUseOrcMCJITReplacement(true);
  ExecutionEngine* EE = EB.create();

  Function *F = M->getFunction("add1");
  auto f = reinterpret_cast<int(*)(int)>(EE->getFunctionAddress(F->getName().str()));
  int result = f(10);
  outs() << "Result: " << result << "\n";
}

void getPointerToFunction_with_orcmcjit(std::unique_ptr<Module> Owner) {
  auto M = Owner.get();
  EngineBuilder EB(std::move(Owner));
  EB.setMCJITMemoryManager(make_unique<SectionMemoryManager>());
  EB.setUseOrcMCJITReplacement(true);
  ExecutionEngine* EE = EB.create();

  Function *F = M->getFunction("add1");
  auto f = reinterpret_cast<int(*)(int)>(EE->getPointerToFunction(F));
  int result = f(10);
  outs() << "Result: " << result << "\n";
}


void usage() {
  outs() << "./sample [mcjit|orc] [run|addr|ptr]\n";
  exit(1);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    usage();
  }

  LLVMContext context;
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  auto M = createTestModule(context);
  outs() << "> created LLVM module:\n" << *M << "\n";

  if (strcmp(argv[1], "mcjit") == 0) {
    if (strcmp(argv[2], "run") == 0) {
      outs() << "> call add1(10) by runFunction with MCJIT\n";
      runFunction_with_mcjit(std::move(M));
    } else if (strcmp(argv[2], "addr") == 0) {
      outs() << "> call add1(10) by getFunctionAddress with MCJIT\n";
      getFunctionAddress_with_mcjit(std::move(M));
    } else if (strcmp(argv[2], "ptr") == 0) {
      outs() << "> call add1(10) by getPointerToFunction with MCJIT\n";
      getPointerToFunction_with_mcjit(std::move(M));
    } else {
      usage();
    }
  } else if (strcmp(argv[1], "orc") == 0) {
    if (strcmp(argv[2], "run") == 0) {
      outs() << "> call add1(10) by runFunction with OrcMCJIT\n";
      runFunction_with_orcmcjit(std::move(M));
    } else if (strcmp(argv[2], "addr") == 0) {
      outs() << "> call add1(10) by getFunctionAddress with OrcMCJIT\n";
      getFunctionAddress_with_orcmcjit(std::move(M));
    } else if (strcmp(argv[2], "ptr") == 0) {
      outs() << "> call add1(10) by getPointerToFunction with OrcMCJIT\n";
      getPointerToFunction_with_orcmcjit(std::move(M));
    } else {
      usage();
    }
  }

  llvm_shutdown();
  return 0;
}
