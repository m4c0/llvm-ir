#pragma once

#include "context.hpp"

namespace bf {
class ops {
  context m_globals;
  llvm::IRBuilder<> m_builder;
  llvm::Value *m_data;
  llvm::Value *m_ptr;

public:
  ops(context g, llvm::BasicBlock *b, llvm::Value *d, llvm::Value *p)
      : m_globals(g), m_builder(g.new_builder()), m_data(d), m_ptr(p) {
    m_builder.SetInsertPoint(b);
  }

  [[nodiscard]] auto load_data() {
    auto gep = m_builder.CreateInBoundsGEP(m_data, {m_globals.zero(), m_ptr});
    return m_builder.CreateLoad(gep);
  }
  void store_data(auto v) {
    auto gep = m_builder.CreateInBoundsGEP(m_data, {m_globals.zero(), m_ptr});
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
    auto fn = m_globals.create_block_function();
    llvm::Value *data = fn->getArg(0);
    llvm::Value *ptr = fn->getArg(1);

    auto entry = m_globals.create_basic_block("entry", fn);
    auto header = m_globals.create_basic_block("head", fn);
    auto body = m_globals.create_basic_block("body", fn);
    auto exit = m_globals.create_basic_block("exit", fn);

    ops e_ops{m_globals, entry, data, ptr};
    e_ops.builder().CreateBr(header);

    ops h_ops{m_globals, header, data, ptr};
    auto h_ptr = h_ops.builder().CreatePHI(m_globals.i32(), 2);

    ops b_ops{m_globals, body, data, h_ptr};
    blk(b_ops);
    b_ops.builder().CreateBr(header);

    h_ptr->addIncoming(ptr, entry);
    h_ptr->addIncoming(b_ops.ptr(), body);
    h_ops.create_icmp(exit, body);

    ops x_ops{m_globals, exit, data, h_ptr};
    x_ops.builder().CreateRet(h_ptr);

    m_ptr = m_builder.CreateCall(fn, {m_data, m_ptr});
  }

  void create_icmp(llvm::BasicBlock *t, llvm::BasicBlock *f) {
    auto cmp = m_builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                    load_data(), m_globals.zero());
    m_builder.CreateCondBr(cmp, t, f);
  }

  [[nodiscard]] llvm::Value *ptr() const { return m_ptr; }
  [[nodiscard]] llvm::IRBuilder<> &builder() { return m_builder; }
};
} // namespace bf
