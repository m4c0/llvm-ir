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
};

static auto create_block_loop(const bf_globals &g, auto blk);

class bf_ops {
  llvm::IRBuilder<> *m_builder;
  bf_globals m_globals;
  llvm::Value *m_data;

public:
  bf_ops(llvm::IRBuilder<> *b, bf_globals g, llvm::Value *d)
      : m_builder(b), m_globals(g), m_data(d) {}

  [[nodiscard]] auto load_data(auto ptr) {
    auto gep = m_builder->CreateInBoundsGEP(m_data, {m_globals.zero, ptr});
    return m_builder->CreateLoad(gep);
  }
  void store_data(auto ptr, auto v) {
    auto gep = m_builder->CreateInBoundsGEP(m_data, {m_globals.zero, ptr});
    m_builder->CreateStore(v, gep);
  }

  void plus(auto ptr) {
    store_data(ptr, m_builder->CreateAdd(load_data(ptr), m_globals.one));
  }
  void minus(auto ptr) {
    store_data(ptr, m_builder->CreateSub(load_data(ptr), m_globals.one));
  }

  [[nodiscard]] auto inc(auto ptr) {
    return m_builder->CreateAdd(ptr, m_globals.one);
  }
  [[nodiscard]] auto dec(auto ptr) {
    return m_builder->CreateSub(ptr, m_globals.one);
  }

  void in(auto ptr) {
    store_data(ptr, m_builder->CreateCall(m_globals.getchar));
  }
  void out(auto ptr) {
    m_builder->CreateCall(m_globals.putchar, {load_data(ptr)});
  }

  auto loop(auto ptr, auto blk) {
    return m_builder->CreateCall(create_block_loop(m_globals, blk),
                                 {m_data, ptr});
  }
};

static auto create_block(const bf_globals &g, auto blk) {
  auto data_tp = g.data_tp->getPointerTo();

  // new_ptr = fn(data *, old_ptr)
  auto fn_tp = llvm::FunctionType::get(g.i32, {data_tp, g.i32}, false);
  auto fn =
      llvm::Function::Create(fn_tp, llvm::Function::InternalLinkage, "", g.mod);

  llvm::Value *data = fn->getArg(0);
  llvm::Value *ptr = fn->getArg(1);

  llvm::IRBuilder<> builder{*g.ctx};
  builder.SetInsertPoint(llvm::BasicBlock::Create(*g.ctx, "entry", fn));

  const bf_ops ops{&builder, g, data};
  builder.CreateRet(blk(ops, ptr));

  return fn;
}

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

  llvm::IRBuilder<> builder{*g.ctx};

  builder.SetInsertPoint(entry);
  bf_ops ops{&builder, g, data};
  auto cmp = builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                ops.load_data(ptr), g.zero);
  builder.CreateCondBr(cmp, exit, header);

  builder.SetInsertPoint(header);
  auto h_ptr = builder.CreatePHI(g.i32, 2);

  builder.SetInsertPoint(body);
  auto b_ptr = builder.CreateCall(create_block(g, blk), {data, h_ptr});
  builder.CreateBr(header);

  h_ptr->addIncoming(ptr, entry);
  h_ptr->addIncoming(b_ptr, body);

  builder.SetInsertPoint(header);

  cmp = builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                           ops.load_data(h_ptr), g.zero);
  builder.CreateCondBr(cmp, exit, body);

  builder.SetInsertPoint(exit);
  auto x_phi = builder.CreatePHI(g.i32, 2);
  x_phi->addIncoming(ptr, entry);
  x_phi->addIncoming(h_ptr, header);
  builder.CreateRet(x_phi);

  return fn;
}

int main() {
  llvm::LLVMContext ctx;
  llvm::Module m{"brainf", ctx};
  llvm::IRBuilder<> builder{ctx};

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
  builder.SetInsertPoint(llvm::BasicBlock::Create(ctx, "entry", main));

  auto data_tp = llvm::ArrayType::get(i32, max_elements_in_data);
  auto data = builder.CreateAlloca(data_tp, nullptr, "data");
  llvm::Value *ptr = zero;

  auto data_init = llvm::ConstantAggregateZero::get(data_tp);
  builder.CreateStore(data_init, data);

  bf_globals g{.ctx = &ctx,
               .mod = &m,
               .i32 = i32,
               .data_tp = data_tp,
               .zero = zero,
               .one = one,
               .getchar = getchar,
               .putchar = putchar};
  bf_ops ops{&builder, g, data};

  // ++++
  for (auto i = 0; i < 4; i++) {
    ops.plus(ptr);
  }

  // [>++++++++<-]
  ptr = ops.loop(ptr, [](bf_ops ops, auto b_ptr) {
    b_ptr = ops.inc(b_ptr);
    for (auto i = 0; i < 8; i++) {
      ops.plus(b_ptr);
    }
    b_ptr = ops.dec(b_ptr);
    ops.minus(b_ptr);
    return b_ptr;
  });

  // ,
  ops.store_data(ptr, builder.CreateCall(getchar));

  // [[>+.-<-]>.<,]
  ptr = ops.loop(ptr, [](bf_ops ops, auto b_ptr_outer) {
    b_ptr_outer = ops.loop(b_ptr_outer, [](bf_ops ops, auto b_ptr) {
      // [>+.-<-]
      b_ptr = ops.inc(b_ptr);
      ops.plus(b_ptr);
      ops.out(b_ptr);
      ops.minus(b_ptr);
      b_ptr = ops.dec(b_ptr);
      ops.minus(b_ptr);
      return b_ptr;
    });

    // >.<,
    b_ptr_outer = ops.inc(b_ptr_outer);
    ops.out(b_ptr_outer);
    b_ptr_outer = ops.dec(b_ptr_outer);
    ops.in(b_ptr_outer);
    return b_ptr_outer;
  });

  builder.CreateRet(zero);

  m.print(llvm::outs(), nullptr);
  return llvm::verifyModule(m, &llvm::errs()) ? 1 : 0;
}
