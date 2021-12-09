#include <iostream>
#include <string>

#include "globals.hpp"

using bf_globals = bf::context;

class bf_ops {
  bf_globals m_globals;
  llvm::IRBuilder<> m_builder;
  llvm::Value *m_data;
  llvm::Value *m_ptr;

  auto create_block_loop(auto blk) {
    // new_ptr = fn(data *, old_ptr)
    auto fn_tp = llvm::FunctionType::get(
        m_globals.i32(), {m_globals.data_ptr_type(), m_globals.i32()}, false);
    auto fn = m_globals.create_function(fn_tp);

    llvm::Value *data = fn->getArg(0);
    llvm::Value *ptr = fn->getArg(1);

    auto entry = m_globals.create_basic_block("entry", fn);
    auto header = m_globals.create_basic_block("head", fn);
    auto body = m_globals.create_basic_block("body", fn);
    auto exit = m_globals.create_basic_block("exit", fn);

    bf_ops e_ops{m_globals, entry, data, ptr};
    e_ops.create_icmp(exit, header);

    bf_ops h_ops{m_globals, header, data, ptr};
    auto h_ptr = h_ops.builder().CreatePHI(m_globals.i32(), 2);

    bf_ops b_ops{m_globals, body, data, h_ptr};
    blk(b_ops);
    b_ops.builder().CreateBr(header);

    h_ptr->addIncoming(ptr, entry);
    h_ptr->addIncoming(b_ops.ptr(), body);
    h_ops.create_icmp(exit, body);

    bf_ops x_ops{m_globals, exit, data, h_ptr};
    auto x_phi = x_ops.builder().CreatePHI(m_globals.i32(), 2);
    x_phi->addIncoming(ptr, entry);
    x_phi->addIncoming(h_ptr, header);
    x_ops.builder().CreateRet(x_phi);

    return fn;
  }

public:
  bf_ops(bf_globals g, llvm::BasicBlock *b, llvm::Value *d, llvm::Value *p)
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
    m_ptr = m_builder.CreateCall(create_block_loop(blk), {m_data, m_ptr});
  }

  void create_icmp(llvm::BasicBlock *t, llvm::BasicBlock *f) {
    auto cmp = m_builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                    load_data(), m_globals.zero());
    m_builder.CreateCondBr(cmp, t, f);
  }

  [[nodiscard]] llvm::Value *ptr() const { return m_ptr; }
  [[nodiscard]] llvm::IRBuilder<> &builder() { return m_builder; }
};

int main() {
  llvm::LLVMContext ctx;
  llvm::Module mod{"brainf", ctx};

  // Handcrafted IR builder for this code from http://www.brainfuck.org/short.b:
  //
  // ++++[>++++++++<-],[[>+.-<-]>.<,]
  // Show ASCII values of input in unary, separated by spaces.
  // (Useful for checking your implementation's newline behavior on input.)

  bf_globals g{&ctx, &mod};

  auto main_tp = llvm::FunctionType::get(g.i32(), false);
  auto main = llvm::Function::Create(main_tp, llvm::Function::ExternalLinkage,
                                     "main", mod);

  auto entry = g.create_basic_block("entry", main);
  auto data = g.create_data(entry);

  bf_ops ops{g, entry, data, g.zero()};
  // ++++
  for (auto i = 0; i < 4; i++) {
    ops.plus();
  }

  // [>++++++++<-]
  ops.loop([](bf_ops &ops) {
    ops.inc();
    for (auto i = 0; i < 8; i++) {
      ops.plus();
    }
    ops.dec();
    ops.minus();
  });

  // ,
  ops.in();

  // [[>+.-<-]>.<,]
  ops.loop([](bf_ops &ops) {
    ops.loop([](bf_ops &ops) {
      // [>+.-<-]
      ops.inc();
      ops.plus();
      ops.out();
      ops.minus();
      ops.dec();
      ops.minus();
    });

    // >.<,
    ops.inc();
    ops.out();
    ops.dec();
    ops.in();
  });

  ops.builder().CreateRet(g.zero());

  mod.print(llvm::outs(), nullptr);
  return llvm::verifyModule(mod, &llvm::errs()) ? 1 : 0;
}
