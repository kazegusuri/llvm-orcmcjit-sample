#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/LambdaResolver.h>
#include <llvm/ExecutionEngine/Orc/LazyEmittingLayer.h>
#include <llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h>
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
using namespace llvm::orc;

class MyJIT {
public:
  typedef ObjectLinkingLayer<> ObjLayerT;
  typedef IRCompileLayer<ObjLayerT> CompileLayerT;
  typedef LazyEmittingLayer<CompileLayerT> LazyEmitLayerT;
  typedef LazyEmitLayerT::ModuleSetHandleT ModuleHandleT;

  MyJIT(TargetMachine& _TM)
      :  CompileLayer(ObjectLayer, SimpleCompiler(_TM)),
         LazyEmitLayer(CompileLayer) {}

  ModuleHandleT addModule(std::unique_ptr<Module> M) {
    std::vector<std::unique_ptr<Module>> Ms;
    Ms.push_back(std::move(M));
    auto Resolver = createLambdaResolver(
                      [&](const std::string &Name) {
                        if (auto Sym = findSymbol(Name))
                          return RuntimeDyld::SymbolInfo(Sym.getAddress(),
                                                         Sym.getFlags());
                        return RuntimeDyld::SymbolInfo(nullptr);
                      },
                      [](const std::string &S) { return nullptr; }
                    );
    return LazyEmitLayer.addModuleSet(std::move(Ms),
                                      make_unique<SectionMemoryManager>(),
                                      std::move(Resolver));
  }

  void removeModule(ModuleHandleT H) { LazyEmitLayer.removeModuleSet(H); }

  JITSymbol findSymbol(const std::string &Name) {
    return LazyEmitLayer.findSymbol(Name, true);
  }

  JITSymbol findSymbolIn(ModuleHandleT H, const std::string &Name) {
    return LazyEmitLayer.findSymbolIn(H, Name, true);
  }

private:
  ObjLayerT ObjectLayer;
  CompileLayerT CompileLayer;
  LazyEmitLayerT LazyEmitLayer;
};

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


int main(int argc, char **argv) {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  LLVMContext context;
  std::unique_ptr<TargetMachine> TM(EngineBuilder().selectTarget());
  auto M = createTestModule(context);
  MyJIT jit(*TM);
  jit.addModule(std::move(M));
  auto sym = jit.findSymbol("add1");
  if (sym) {
    auto add1 = reinterpret_cast<int(*)(int)>(sym.getAddress());
    int result = add1(10);
    outs() << "result: " << result << "\n";
  } else {
    outs() << "symbol not found\n";
  }
  llvm_shutdown();
  return 0;
}
