#pragma once
#include "llvm.hpp"

namespace bf {
class globals {
  llvm::LLVMContext m_ctx;
  llvm::Module m_mod;
  llvm::legacy::FunctionPassManager m_fpm;
  llvm::legacy::PassManager m_mpm;

public:
  explicit globals(const char *name)
      : m_ctx{}, m_mod{name, m_ctx}, m_fpm{&m_mod}, m_mpm{} {
    llvm::PassManagerBuilder pmb;
    pmb.OptLevel = 3;
    pmb.SizeLevel = 0;
    pmb.Inliner = llvm::createFunctionInliningPass(3, 0, false);
    pmb.LoopVectorize = true;
    pmb.SLPVectorize = true;

    pmb.populateFunctionPassManager(m_fpm);
    pmb.populateModulePassManager(m_mpm);
    m_fpm.doInitialization();
  }

  [[nodiscard]] int finish() noexcept {
    m_mpm.run(m_mod);
    m_mod.print(llvm::outs(), nullptr);
    return llvm::verifyModule(m_mod, &llvm::errs()) ? 1 : 0;
  }

  [[nodiscard]] auto &context() noexcept { return m_ctx; }
  [[nodiscard]] auto &mod() noexcept { return m_mod; }

  void optimise(llvm::Function &f) noexcept { m_fpm.run(f); }
};
} // namespace bf
