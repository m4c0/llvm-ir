#include <iostream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <string>

static constexpr const auto max_elements_in_data = 30000;

struct bf_globals {
  llvm::LLVMContext *ctx;
  llvm::Module *mod;

  llvm::IntegerType *i32;
  llvm::Type *data_tp;

  llvm::Value *zero;
  llvm::Value *one;
  llvm::FunctionCallee getchar;
  llvm::FunctionCallee putchar;
  llvm::Function *main;
};

static auto create_block_loop(const bf_globals &g, auto blk);

class bf_ops {
  bf_globals m_globals;
  llvm::IRBuilder<> m_builder;
  llvm::Value *m_data;
  llvm::Value *m_ptr;

  static auto create_data(bf_globals g, llvm::IRBuilder<> *b) {
    b->SetInsertPoint(llvm::BasicBlock::Create(*g.ctx, "entry", g.main));
    auto data = b->CreateAlloca(g.data_tp, nullptr, "data");
    auto data_init = llvm::ConstantAggregateZero::get(g.data_tp);
    b->CreateStore(data_init, data);
    return data;
  }

public:
  explicit bf_ops(bf_globals g)
      : m_globals(g), m_builder(*g.ctx), m_data(create_data(g, &m_builder)),
        m_ptr(g.zero) {}
  bf_ops(bf_globals g, llvm::BasicBlock *b, llvm::Value *d, llvm::Value *p)
      : m_globals(g), m_builder(*g.ctx), m_data(d), m_ptr(p) {
    m_builder.SetInsertPoint(b);
  }

  [[nodiscard]] auto load_data() {
    auto gep = m_builder.CreateInBoundsGEP(m_data, {m_globals.zero, m_ptr});
    return m_builder.CreateLoad(gep);
  }
  void store_data(auto v) {
    auto gep = m_builder.CreateInBoundsGEP(m_data, {m_globals.zero, m_ptr});
    m_builder.CreateStore(v, gep);
  }

  void plus() { store_data(m_builder.CreateAdd(load_data(), m_globals.one)); }
  void minus() { store_data(m_builder.CreateSub(load_data(), m_globals.one)); }

  void inc() { m_ptr = m_builder.CreateAdd(m_ptr, m_globals.one); }
  void dec() { m_ptr = m_builder.CreateSub(m_ptr, m_globals.one); }

  void in() { store_data(m_builder.CreateCall(m_globals.getchar)); }
  void out() { m_builder.CreateCall(m_globals.putchar, {load_data()}); }

  void loop(auto blk) {
    m_ptr = m_builder.CreateCall(create_block_loop(m_globals, blk),
                                 {m_data, m_ptr});
  }

  void create_icmp(llvm::BasicBlock *t, llvm::BasicBlock *f) {
    auto cmp = m_builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                    load_data(), m_globals.zero);
    m_builder.CreateCondBr(cmp, t, f);
  }

  [[nodiscard]] auto ptr() const { return m_ptr; }
  [[nodiscard]] auto &builder() { return m_builder; }
};

static auto create_block_loop(const bf_globals &g, auto blk) {
  auto data_tp = g.data_tp->getPointerTo();

  // new_ptr = fn(data *, old_ptr)
  auto fn_tp = llvm::FunctionType::get(g.i32, {data_tp, g.i32}, false);
  auto fn =
      llvm::Function::Create(fn_tp, llvm::Function::InternalLinkage, "", g.mod);

  llvm::Value *data = fn->getArg(0);
  llvm::Value *ptr = fn->getArg(1);

  auto entry = llvm::BasicBlock::Create(*g.ctx, "entry", fn);
  auto header = llvm::BasicBlock::Create(*g.ctx, "head", fn);
  auto body = llvm::BasicBlock::Create(*g.ctx, "body", fn);
  auto exit = llvm::BasicBlock::Create(*g.ctx, "exit", fn);

  bf_ops e_ops{g, entry, data, ptr};
  e_ops.create_icmp(exit, header);

  bf_ops h_ops{g, header, data, ptr};
  auto h_ptr = h_ops.builder().CreatePHI(g.i32, 2);

  bf_ops b_ops{g, body, data, h_ptr};
  blk(b_ops);
  b_ops.builder().CreateBr(header);

  h_ptr->addIncoming(ptr, entry);
  h_ptr->addIncoming(b_ops.ptr(), body);
  h_ops.create_icmp(exit, body);

  bf_ops x_ops{g, exit, data, h_ptr};
  auto x_phi = x_ops.builder().CreatePHI(g.i32, 2);
  x_phi->addIncoming(ptr, entry);
  x_phi->addIncoming(h_ptr, header);
  x_ops.builder().CreateRet(x_phi);

  return fn;
}

int main() {
  llvm::LLVMContext ctx;
  llvm::Module m{"brainf", ctx};

  // Handcrafted IR builder for this code from http://www.brainfuck.org/short.b:
  //
  // ++++[>++++++++<-],[[>+.-<-]>.<,]
  // Show ASCII values of input in unary, separated by spaces.
  // (Useful for checking your implementation's newline behavior on input.)

  auto i32 = llvm::Type::getInt32Ty(ctx);

  auto zero = llvm::ConstantInt::get(i32, 0);
  auto one = llvm::ConstantInt::get(i32, 1);

  auto getchar = m.getOrInsertFunction("getchar", i32);
  auto putchar = m.getOrInsertFunction("putchar", i32, i32);

  auto main_tp = llvm::FunctionType::get(i32, false);
  auto main = llvm::Function::Create(main_tp, llvm::Function::ExternalLinkage,
                                     "main", m);
  auto data_tp = llvm::ArrayType::get(i32, max_elements_in_data);

  bf_globals g{
      .ctx = &ctx,
      .mod = &m,
      .i32 = i32,
      .data_tp = data_tp,
      .zero = zero,
      .one = one,
      .getchar = getchar,
      .putchar = putchar,
      .main = main,
  };
  bf_ops ops{g};

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

  ops.builder().CreateRet(zero);

  m.print(llvm::outs(), nullptr);
  return llvm::verifyModule(m, &llvm::errs()) ? 1 : 0;
}
