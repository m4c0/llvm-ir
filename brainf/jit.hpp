#pragma once

#include "llvm.hpp"

#include <memory>

namespace bf {
static llvm::Expected<llvm::GenericValue>
jit(std::unique_ptr<llvm::Module> m, llvm::Function *fn,
    llvm::ArrayRef<llvm::GenericValue> args) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  auto ee = llvm::EngineBuilder{std::move(m)}.create();
  if (!ee) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "Failure creating JIT engine");
  }
  return ee->runFunction(fn, args);
}
} // namespace bf
