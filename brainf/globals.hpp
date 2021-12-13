#pragma once
#include "jit.hpp"
#include "llvm.hpp"

namespace bf {
class globals {
  std::unique_ptr<llvm::LLVMContext> m_ctx;
  std::unique_ptr<llvm::Module> m_mod;
  llvm::legacy::FunctionPassManager m_fpm;
  llvm::legacy::PassManager m_mpm;

public:
  explicit globals(const char *name)
      : m_ctx{std::make_unique<llvm::LLVMContext>()},
        m_mod{std::make_unique<llvm::Module>(name, *m_ctx)}, m_fpm{m_mod.get()},
        m_mpm{} {
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

  [[nodiscard]] bool finish() noexcept {
    m_mpm.run(*m_mod);
    return llvm::verifyModule(*m_mod, &llvm::errs());
  }
  void dump() noexcept { m_mod->print(llvm::outs(), nullptr); }
  auto run(llvm::Function *fn) noexcept {
    return jit(std::move(m_mod), fn, {});
  }

  [[nodiscard]] auto &context() noexcept { return *m_ctx; }
  [[nodiscard]] auto &mod() noexcept { return *m_mod; }

  void optimise(llvm::Function &f) noexcept { m_fpm.run(f); }
};
} // namespace bf
