#pragma once
#include "globals.hpp"
#include "llvm.hpp"

namespace bf {
class context {
  static constexpr const auto max_elements_in_data = 30000;

  globals *m_g;

  llvm::IntegerType *m_i32;

  llvm::Value *m_zero;
  llvm::Value *m_one;
  llvm::FunctionCallee m_getchar;
  llvm::FunctionCallee m_putchar;

  llvm::Function *m_main;
  llvm::BasicBlock *m_main_entry;
  llvm::BasicBlock *m_main_exit;
  llvm::Value *m_data;

  [[nodiscard]] auto create_data() {
    auto b = new_builder();

    auto data_type = llvm::ArrayType::get(m_i32, max_elements_in_data);
    auto data_init = llvm::ConstantAggregateZero::get(data_type);
    b.SetInsertPoint(m_main_entry);
    auto data = b.CreateAlloca(data_type, nullptr, "data");
    b.CreateStore(data_init, data);
    return data;
  }

public:
  context(globals *g)
      : m_g{g},

        m_i32{llvm::Type::getInt32Ty(g->context())},

        m_zero{llvm::ConstantInt::get(m_i32, 0)}, m_one{llvm::ConstantInt::get(
                                                      m_i32, 1)},

        m_getchar{g->mod().getOrInsertFunction("getchar", m_i32)},
        m_putchar{g->mod().getOrInsertFunction("putchar", m_i32, m_i32)},

        m_main{llvm::Function::Create(llvm::FunctionType::get(m_i32, false),
                                      llvm::Function::ExternalLinkage, "main",
                                      g->mod())},
        m_main_entry{create_basic_block("entry")}, m_data{create_data()},
        m_main_exit{create_basic_block("exit")} {}

  [[nodiscard]] llvm::BasicBlock *
  create_basic_block(const char *name) const noexcept {
    return llvm::BasicBlock::Create(m_g->context(), name, m_main);
  }

  [[nodiscard]] auto i32() const noexcept { return m_i32; }

  [[nodiscard]] auto one() const noexcept { return m_one; }
  [[nodiscard]] auto zero() const noexcept { return m_zero; }

  [[nodiscard]] auto getchar() const noexcept { return m_getchar; }
  [[nodiscard]] auto putchar() const noexcept { return m_putchar; }

  [[nodiscard]] auto main_entry() const noexcept { return m_main_entry; }
  [[nodiscard]] auto main_exit() const noexcept { return m_main_exit; }
  [[nodiscard]] auto data() const noexcept { return m_data; }

  [[nodiscard]] llvm::IRBuilder<> new_builder() const noexcept {
    return llvm::IRBuilder<>{m_g->context()};
  }

  [[nodiscard]] auto finish() const noexcept {
    llvm::IRBuilder<> builder{m_g->context()};
    builder.SetInsertPoint(m_main_exit);
    builder.CreateRet(m_zero);
    m_g->optimise(*m_main);
    return m_g->finish();
  }

  [[nodiscard]] bool finish_and_run() const noexcept {
    if (finish()) {
      return true;
    }
    m_g->run("main");
    return false;
  }
};
} // namespace bf
