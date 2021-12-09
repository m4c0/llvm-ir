#pragma once
#include "llvm.hpp"

namespace bf {
static constexpr const auto max_elements_in_data = 30000;

class context {
  llvm::LLVMContext *m_ctx;
  llvm::Module *m_mod;

  llvm::IntegerType *m_i32;
  llvm::Type *m_data_type;

  llvm::Value *m_zero;
  llvm::Value *m_one;
  llvm::FunctionCallee m_getchar;
  llvm::FunctionCallee m_putchar;

public:
  context(llvm::LLVMContext *ctx, llvm::Module *mod)
      : m_ctx{ctx}, m_mod{mod}, m_i32{llvm::Type::getInt32Ty(*m_ctx)},
        m_data_type{llvm::ArrayType::get(m_i32, bf::max_elements_in_data)},
        m_zero{llvm::ConstantInt::get(m_i32, 0)}, m_one{llvm::ConstantInt::get(
                                                      m_i32, 1)},
        m_getchar{m_mod->getOrInsertFunction("getchar", m_i32)},
        m_putchar{m_mod->getOrInsertFunction("putchar", m_i32, m_i32)} {}

  [[nodiscard]] auto create_basic_block(const char *name,
                                        llvm::Function *fn) const noexcept {
    return llvm::BasicBlock::Create(*m_ctx, name, fn);
  }
  [[nodiscard]] auto create_data(llvm::BasicBlock *block) const noexcept {
    auto b = new_builder();

    auto data_init = llvm::ConstantAggregateZero::get(m_data_type);
    b.SetInsertPoint(block);
    auto data = b.CreateAlloca(m_data_type, nullptr, "data");
    b.CreateStore(data_init, data);

    return data;
  }
  [[nodiscard]] auto create_function(llvm::FunctionType *fn_tp) const noexcept {
    return llvm::Function::Create(fn_tp, llvm::Function::InternalLinkage, "",
                                  m_mod);
  }

  [[nodiscard]] auto data_ptr_type() const noexcept {
    return m_data_type->getPointerTo();
  }
  [[nodiscard]] auto i32() const noexcept { return m_i32; }

  [[nodiscard]] auto one() const noexcept { return m_one; }
  [[nodiscard]] auto zero() const noexcept { return m_zero; }

  [[nodiscard]] auto getchar() const noexcept { return m_getchar; }
  [[nodiscard]] auto putchar() const noexcept { return m_putchar; }

  [[nodiscard]] llvm::IRBuilder<> new_builder() const noexcept {
    return llvm::IRBuilder<>{*m_ctx};
  }
};
} // namespace bf
