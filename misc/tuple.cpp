#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"

int main() {
  llvm::LLVMContext ctx;
  llvm::Module m { "tuple", ctx };

  llvm::IRBuilder<> b { ctx };

  auto i128 = llvm::IntegerType::getInt128Ty(ctx);
  auto i64 = llvm::IntegerType::getInt64Ty(ctx);
  auto i32 = llvm::IntegerType::getInt32Ty(ctx);
  auto i8 = llvm::IntegerType::getInt8Ty(ctx);

  auto meta_t0 = llvm::StructType::create("fake_list", i8);
  auto meta_t1 = llvm::StructType::create("fake_vector", i8);

  auto t0 = llvm::StructType::get(ctx, { meta_t0, i128, i64, i32 });
  auto t1 = llvm::StructType::get(ctx, { meta_t1, i128, i64, i32 });

  auto t0p = t0->getPointerTo();
  auto t1p = t1->getPointerTo();

  // static t1 meh(t0 i) { return { {}, i.i128, i.i64, i.i32 }; }
  auto fn_tp = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), { t1p, t0p }, false);
  auto meh = llvm::Function::Create(fn_tp, llvm::Function::InternalLinkage, "meh", m);
  b.SetInsertPoint(llvm::BasicBlock::Create(ctx, "entry", meh));

  auto arg = meh->getArg(1);
  auto v128 = b.CreateLoad(i128, b.CreateStructGEP(t0, arg, 1));
  auto v64 = b.CreateLoad(i64, b.CreateStructGEP(t0, arg, 2));
  auto v32 = b.CreateLoad(i32, b.CreateStructGEP(t0, arg, 3));

  auto ret = meh->getArg(0);
  b.CreateStore(v128, b.CreateStructGEP(t1, ret, 1));
  b.CreateStore(v64, b.CreateStructGEP(t1, ret, 2));
  b.CreateStore(v32, b.CreateStructGEP(t1, ret, 3));

  b.CreateRetVoid();

  // static t1 moo() { return meh(t0 { {}, 1, 2, 3 }); }
  fn_tp = llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), { t1p }, false);
  auto moo = llvm::Function::Create(fn_tp, llvm::Function::InternalLinkage, "moo", m);
  b.SetInsertPoint(llvm::BasicBlock::Create(ctx, "entry", moo));

  auto in = b.CreateAlloca(t0);
  b.CreateStore(llvm::ConstantInt::get(i128, 1), b.CreateStructGEP(t0, in, 1));
  b.CreateStore(llvm::ConstantInt::get(i64, 2), b.CreateStructGEP(t0, in, 2));
  b.CreateStore(llvm::ConstantInt::get(i32, 3), b.CreateStructGEP(t0, in, 3));

  ret = moo->getArg(0);
  b.CreateCall(meh, { ret, in });
  b.CreateRetVoid();

  // auto mainly() { auto out = meh(); return out.i128 + out.i64 + out.i32; }
  fn_tp = llvm::FunctionType::get(llvm::IntegerType::getInt128Ty(ctx), false);
  auto main = llvm::Function::Create(fn_tp, llvm::Function::ExternalLinkage, "mainly", m);
  b.SetInsertPoint(llvm::BasicBlock::Create(ctx, "entry", main));

  in = b.CreateAlloca(t1);
  auto in1 = b.CreateStructGEP(t1, in, 1);
  auto in2 = b.CreateStructGEP(t1, in, 2);
  auto in3 = b.CreateStructGEP(t1, in, 3);
  b.CreateStore(llvm::ConstantInt::get(i128, 1), in1);
  b.CreateStore(llvm::ConstantInt::get(i64, 2), in2);
  b.CreateStore(llvm::ConstantInt::get(i32, 3), in3);
  b.CreateCall(moo, { in });
  auto out1 = b.CreateLoad(i128, in1);
  auto out2 = b.CreateSExt(b.CreateLoad(i64, in2), i128);
  auto out3 = b.CreateSExt(b.CreateLoad(i32, in3), i128);

  b.CreateRet(b.CreateAdd(out1, b.CreateAdd(out2, out3)));

  // Quite verbose on -O0, but becomes a "return 6" with -O3
  m.print(llvm::outs(), nullptr);
  llvm::verifyModule(m, &llvm::errs());
}
