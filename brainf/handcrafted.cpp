#include <iostream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <string>

static constexpr const auto max_elements_in_data = 30000;

struct bf_globals {
  llvm::Value *data;
  llvm::Value *zero;
  llvm::Value *one;
  llvm::FunctionCallee getchar;
  llvm::FunctionCallee putchar;
};

class bf_ops {
  llvm::IRBuilder<> &m_builder;
  bf_globals m_globals;

public:
  bf_ops(llvm::IRBuilder<> &b, bf_globals g) : m_builder(b), m_globals(g) {}

  [[nodiscard]] auto load_data(auto ptr) {
    auto gep =
        m_builder.CreateInBoundsGEP(m_globals.data, {m_globals.zero, ptr});
    return m_builder.CreateLoad(gep);
  }
  void store_data(auto ptr, auto v) {
    auto gep =
        m_builder.CreateInBoundsGEP(m_globals.data, {m_globals.zero, ptr});
    m_builder.CreateStore(v, gep);
  }

  void plus(auto ptr) {
    store_data(ptr, m_builder.CreateAdd(load_data(ptr), m_globals.one));
  }
  void minus(auto ptr) {
    store_data(ptr, m_builder.CreateSub(load_data(ptr), m_globals.one));
  }

  [[nodiscard]] auto inc(auto ptr) {
    return m_builder.CreateAdd(ptr, m_globals.one, ".ptr.");
  }
  [[nodiscard]] auto dec(auto ptr) {
    return m_builder.CreateSub(ptr, m_globals.one, ".ptr.");
  }

  void in(auto ptr) {
    store_data(ptr, m_builder.CreateCall(m_globals.getchar));
  }
  void out(auto ptr) {
    m_builder.CreateCall(m_globals.putchar, {load_data(ptr)});
  }
};

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

  bf_globals g{.one = one,
               .data = data,
               .zero = zero,
               .getchar = getchar,
               .putchar = putchar};

  auto data_init = llvm::ConstantAggregateZero::get(data_tp);
  builder.CreateStore(data_init, data);

  const auto load_data = [&](auto l_ptr) {
    auto gep = builder.CreateInBoundsGEP(data, {zero, l_ptr});
    return builder.CreateLoad(gep);
  };
  const auto store_data = [&](auto l_ptr, auto v) {
    auto gep = builder.CreateInBoundsGEP(data, {zero, l_ptr});
    return builder.CreateStore(v, gep);
  };

  const auto plus = [&](auto l_ptr) {
    store_data(l_ptr, builder.CreateAdd(load_data(l_ptr), one));
  };
  const auto minus = [&](auto l_ptr) {
    store_data(l_ptr, builder.CreateSub(load_data(l_ptr), one));
  };

  const auto loop = [&](auto l_ptr, auto blk) {
    auto block = builder.GetInsertBlock();

    auto header = llvm::BasicBlock::Create(ctx, "head", main);
    auto body = llvm::BasicBlock::Create(ctx, "body", main);
    auto exit = llvm::BasicBlock::Create(ctx, "exit", main);

    builder.CreateBr(header);

    builder.SetInsertPoint(header);
    auto h_ptr = builder.CreatePHI(i32, 2, ".ptr.");

    builder.SetInsertPoint(body);
    auto b_ptr = blk(bf_ops{builder, g}, static_cast<llvm::Value *>(h_ptr));
    builder.CreateBr(header);

    { // header
      h_ptr->addIncoming(l_ptr, block);
      h_ptr->addIncoming(b_ptr, builder.GetInsertBlock());

      builder.SetInsertPoint(header);

      auto d = load_data(h_ptr);
      auto cmp = builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_EQ, d, zero);
      builder.CreateCondBr(cmp, exit, body);
    }

    builder.SetInsertPoint(exit);
    return h_ptr;
  };

  // ++++
  for (auto i = 0; i < 4; i++) {
    plus(ptr);
  }

  // [>++++++++<-]
  ptr = loop(ptr, [](bf_ops ops, auto b_ptr) {
    b_ptr = ops.inc(b_ptr);
    for (auto i = 0; i < 8; i++) {
      ops.plus(b_ptr);
    }
    b_ptr = ops.dec(b_ptr);
    ops.minus(b_ptr);
    return b_ptr;
  });

  // ,
  store_data(ptr, builder.CreateCall(getchar));

  // [[>+.-<-]>.<,]
  ptr = loop(ptr, [loop](bf_ops ops, auto b_ptr_outer) {
    b_ptr_outer = loop(b_ptr_outer, [](bf_ops ops, auto b_ptr) {
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

  if (!llvm::verifyModule(m, &llvm::errs())) {
    m.print(llvm::outs(), nullptr);
  }
}
