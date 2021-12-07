#include <iostream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <m4c0/parser/combiners.hpp>
#include <m4c0/parser/str.hpp>
#include <string>

using namespace m4c0::parser;

static constexpr const auto max_elements_in_data = 30000;

struct context {
  llvm::LLVMContext *ctx;
  llvm::Module *mod;

  llvm::IntegerType *i32;
  llvm::Type *data_tp;

  llvm::Value *zero;
  llvm::Value *one;
  llvm::FunctionCallee getchar;
  llvm::FunctionCallee putchar;
};

class builder {
  llvm::IRBuilder<> *m_builder;
  context m_ctx;
  llvm::Value *m_data;
  llvm::Value *m_ptr;

  [[nodiscard]] auto gep() noexcept {
    return m_builder->CreateInBoundsGEP(m_data, {m_ctx.zero, m_ptr});
  }

  [[nodiscard]] auto load_data() noexcept {
    return m_builder->CreateLoad(gep());
  }
  void store_data(llvm::Value *v) noexcept { m_builder->CreateStore(v, gep()); }

  [[nodiscard]] auto dec(llvm::Value *v) noexcept {
    return m_builder->CreateSub(v, m_ctx.one);
  }
  [[nodiscard]] auto inc(llvm::Value *v) noexcept {
    return m_builder->CreateAdd(v, m_ctx.one);
  }

public:
  constexpr builder(llvm::IRBuilder<> *b, context g, llvm::Value *d,
                    llvm::Value *p)
      : m_builder(b), m_ctx(g), m_data(d), m_ptr(p) {}

  void dec_ptr() noexcept { m_ptr = dec(m_ptr); }
  void inc_ptr() noexcept { m_ptr = inc(m_ptr); }
  void dec_data() noexcept { store_data(dec(load_data())); }
  void inc_data() noexcept { store_data(inc(load_data())); }
  void output() noexcept {
    m_builder->CreateCall(m_ctx.putchar, {load_data()});
  }
  void input() noexcept { store_data(m_builder->CreateCall(m_ctx.getchar)); }

  void begin_block() noexcept {}
  void end_block() noexcept {}
};

using rtype = void (builder::*)() noexcept;

class meh {
  builder *m_c;

public:
  constexpr meh(builder *c) : m_c(c) {}

  constexpr meh operator+(rtype m) const noexcept {
    (m_c->*m)();
    return *this;
  }
};

namespace bf::parser {
static constexpr const auto gt = match('<') & &builder::dec_ptr;
static constexpr const auto lt = match('>') & &builder::inc_ptr;
static constexpr const auto plus = match('+') & &builder::inc_data;
static constexpr const auto minus = match('-') & &builder::dec_data;
static constexpr const auto dot = match('.') & &builder::output;
static constexpr const auto comma = match(',') & &builder::input;
static constexpr const auto lbr = match('[');
static constexpr const auto rbr = match(']');

class block {
  builder *m_c;

public:
  explicit constexpr block(builder *c) : m_c(c) {}
  constexpr result<rtype> operator()(input_t) const noexcept;
};

static constexpr const auto ops = gt | lt | plus | minus | dot | comma;

static constexpr auto many_blocks(builder *c) {
  return at_least_one(block{c}, meh{c});
}

static constexpr auto loop(builder *c) {
  return lbr & many_blocks(c) & rbr & &builder::end_block;
}

constexpr result<rtype> block::operator()(input_t in) const noexcept {
  return (ops | loop(m_c))(in);
}
} // namespace bf::parser

static auto &read_user_input(std::string &line) {
  return std::getline(std::cin, line);
}
int main() {
  llvm::LLVMContext ctx;
  while (std::cin) {
    for (std::string line; read_user_input(line);) {
      llvm::Module m{"brainf", ctx};
      llvm::IRBuilder<> bld{ctx};

      auto i32 = llvm::Type::getInt32Ty(ctx);
      auto main_tp = llvm::FunctionType::get(i32, false);
      auto main = llvm::Function::Create(
          main_tp, llvm::Function::ExternalLinkage, "main", m);
      bld.SetInsertPoint(llvm::BasicBlock::Create(ctx, "entry", main));

      context c{
          .ctx = &ctx,
          .mod = &m,
          .data_tp = llvm::ArrayType::get(i32, max_elements_in_data),
          .zero = llvm::Constant::getIntegerValue(i32, llvm::APInt(32, 0)),
          .one = llvm::Constant::getIntegerValue(i32, llvm::APInt(32, 1)),
          .getchar = m.getOrInsertFunction("getchar", i32),
          .putchar = m.getOrInsertFunction("putchar", i32, i32),
      };
      auto data = bld.CreateAlloca(c.data_tp, nullptr, "data");
      auto data_init = llvm::ConstantAggregateZero::get(c.data_tp);
      bld.CreateStore(data_init, data);

      builder b{&bld, c, data, c.zero};
      const auto r = bf::parser::many_blocks(&b)({line.data(), line.size()});

      bld.CreateRet(c.zero);

      if (!llvm::verifyModule(m, &llvm::errs())) {
        m.print(llvm::outs(), nullptr);
      }
    }
  }
}
