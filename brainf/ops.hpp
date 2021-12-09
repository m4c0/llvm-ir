#pragma once

#include "context.hpp"

namespace bf {
class ops {
  context m_globals;
  llvm::IRBuilder<> m_builder;
  llvm::Value *m_ptr;

public:
  ops(context g, llvm::BasicBlock *b, llvm::Value *p)
      : m_globals(g), m_builder(g.new_builder()), m_ptr(p) {
    m_builder.SetInsertPoint(b);
  }

  [[nodiscard]] auto load_data() {
    auto gep = m_builder.CreateInBoundsGEP(m_globals.data(),
                                           {m_globals.zero(), m_ptr});
    return m_builder.CreateLoad(gep);
  }
  void store_data(auto v) {
    auto gep = m_builder.CreateInBoundsGEP(m_globals.data(),
                                           {m_globals.zero(), m_ptr});
    m_builder.CreateStore(v, gep);
  }

  void plus() { store_data(m_builder.CreateAdd(load_data(), m_globals.one())); }
  void minus() {
    store_data(m_builder.CreateSub(load_data(), m_globals.one()));
  }

  void inc() { m_ptr = m_builder.CreateAdd(m_ptr, m_globals.one()); }
  void dec() { m_ptr = m_builder.CreateSub(m_ptr, m_globals.one()); }

  void in() { store_data(m_builder.CreateCall(m_globals.getchar())); }
  void out() { m_builder.CreateCall(m_globals.putchar(), {load_data()}); }

  void loop(auto blk) {
    auto entry = m_builder.GetInsertBlock();
    auto header = m_globals.create_basic_block("head");
    auto body = m_globals.create_basic_block("body");
    auto exit = m_globals.create_basic_block("exit");

    m_builder.CreateBr(header);

    m_builder.SetInsertPoint(header);
    auto h_ptr = m_builder.CreatePHI(m_globals.i32(), 2);
    h_ptr->addIncoming(m_ptr, entry);

    m_ptr = h_ptr;
    auto cmp = m_builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                    load_data(), m_globals.zero());
    m_builder.CreateCondBr(cmp, exit, body);

    ops b_ops{m_globals, body, h_ptr};
    blk(b_ops);
    b_ops.builder().CreateBr(header);

    h_ptr->addIncoming(b_ops.ptr(), b_ops.builder().GetInsertBlock());

    m_builder.SetInsertPoint(exit);
  }

  [[nodiscard]] llvm::Value *ptr() const { return m_ptr; }
  [[nodiscard]] llvm::IRBuilder<> &builder() { return m_builder; }
};
} // namespace bf
