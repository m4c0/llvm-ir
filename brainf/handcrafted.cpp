#include <iostream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <string>

static constexpr const auto max_elements_in_data = 30000;

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

  auto zero = llvm::Constant::getIntegerValue(i32, llvm::APInt(32, 0));
  auto one = llvm::Constant::getIntegerValue(i32, llvm::APInt(32, 1));

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
    auto b_ptr = blk(static_cast<llvm::Value *>(h_ptr));
    builder.CreateBr(header);

    {                                            // header
      auto body_exit = builder.GetInsertBlock(); // Handles nested loops
      builder.SetInsertPoint(header);
      h_ptr->addIncoming(l_ptr, block);
      h_ptr->addIncoming(b_ptr, body_exit);

      auto d = load_data(h_ptr);
      auto cmp = builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_EQ, d, zero);
      builder.CreateCondBr(cmp, exit, body);
    }

    builder.SetInsertPoint(exit);
    auto x_phi = builder.CreatePHI(i32, 1, ".xptr");
    x_phi->addIncoming(h_ptr, header);

    return x_phi;
  };

  // ++++
  for (auto i = 0; i < 4; i++) {
    plus(ptr);
  }

  // [>++++++++<-]
  ptr = loop(ptr, [&](auto b_ptr) {
    b_ptr = builder.CreateAdd(b_ptr, one, ".ptr.");
    for (auto i = 0; i < 8; i++) {
      plus(b_ptr);
    }
    b_ptr = builder.CreateSub(b_ptr, one, ".ptr.");
    minus(b_ptr);
    return b_ptr;
  });

  // ,
  store_data(ptr, builder.CreateCall(getchar));

  // [[>+.-<-]>.<,]
  ptr = loop(ptr, [&](auto b_ptr_outer) {
    b_ptr_outer = loop(b_ptr_outer, [&](auto b_ptr) {
      // [>+.-<-]
      b_ptr = builder.CreateAdd(b_ptr, one, ".ptr.");
      plus(b_ptr);
      builder.CreateCall(putchar, {load_data(b_ptr)});
      minus(b_ptr);
      b_ptr = builder.CreateSub(b_ptr, one, ".ptr.");
      minus(b_ptr);
      return b_ptr;
    });

    // >.<,
    b_ptr_outer = builder.CreateAdd(b_ptr_outer, one, ".ptr.");
    builder.CreateCall(putchar, {load_data(b_ptr_outer)});
    b_ptr_outer = builder.CreateSub(b_ptr_outer, one, ".ptr.");
    store_data(ptr, builder.CreateCall(getchar));
    return b_ptr_outer;
  });

  builder.CreateRet(zero);

  if (!llvm::verifyModule(m, &llvm::errs())) {
    m.print(llvm::outs(), nullptr);
  }
}
