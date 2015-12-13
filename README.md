# LLVM OrcMCJIT sample

LLVM 3.7でしかおそらく動きません。llvm-configでバージョンを確認しましょう。

```
$ llvm-config --version
3.7.0

```

### 互換API(compatible_api)

ExecutionEngine経由で使う場合のサンプルです。

- runFunction
- getPointerToFunction
- getFunctionAddress

### 新API(new_api)

OrcMCJITのLayerを自分で構築する場合のサンプルです。

- addModuleSet
- findSymbol



