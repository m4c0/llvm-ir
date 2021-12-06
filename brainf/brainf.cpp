#include <iostream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <m4c0/ce_list.hpp>
#include <m4c0/parser/combiners.hpp>
#include <m4c0/parser/str.hpp>
#include <string>

using namespace m4c0::parser;

class context {
  llvm::IRBuilder<> *m_builder;
  llvm::Value *m_data;

  llvm::Value *m_zero;
  llvm::Value *m_one;

  llvm::FunctionCallee *m_getchar;
  llvm::FunctionCallee *m_putchar;

  llvm::Value *m_ptr;

  [[nodiscard]] auto gep() noexcept {
    return m_builder->CreateInBoundsGEP(m_data, {m_zero, m_ptr});
  }

  [[nodiscard]] auto load_data() noexcept {
    return m_builder->CreateLoad(gep());
  }
  void store_data(llvm::Value *v) noexcept { m_builder->CreateStore(v, gep()); }

  [[nodiscard]] auto dec(llvm::Value *v) noexcept {
    return m_builder->CreateSub(v, m_one);
  }
  [[nodiscard]] auto inc(llvm::Value *v) noexcept {
    return m_builder->CreateAdd(v, m_one);
  }

  [[nodiscard]] auto *ptr() noexcept { return m_ptr; }

public:
  void dec_ptr() noexcept { m_ptr = dec(ptr()); }
  void inc_ptr() noexcept { m_ptr = inc(ptr()); }
  void dec_data() noexcept { store_data(dec(load_data())); }
  void inc_data() noexcept { store_data(inc(load_data())); }
  void output() noexcept { m_builder->CreateCall(*m_putchar, {load_data()}); }
  void input() noexcept { store_data(m_builder->CreateCall(*m_getchar)); }

  void begin_block() noexcept {}
  void end_block() noexcept {}
};

using rtype = void (context::*)() noexcept;

struct rtype_buffer {
  m4c0::ce_list_node<rtype> *allocate();
};
using rtype_list = m4c0::ce_list<rtype, rtype_buffer>;

static constexpr const auto gt = match('<') & &context::dec_ptr;
static constexpr const auto lt = match('>') & &context::inc_ptr;
static constexpr const auto plus = match('+') & &context::inc_data;
static constexpr const auto minus = match('-') & &context::dec_data;
static constexpr const auto dot = match('.') & &context::output;
static constexpr const auto comma = match(',') & &context::input;
static constexpr const auto lbr = match('[');
static constexpr const auto rbr = match(']');

struct block {
  constexpr result<rtype> operator()(input_t) const noexcept;
};

static constexpr const auto ops =
    gt | lt | plus | minus | dot | comma | block();

static constexpr const auto many_ops = many(skip(ops));

constexpr result<rtype> block::operator()(input_t in) const noexcept {
  constexpr const auto p = lbr & many_ops & rbr & &context::end_block;
  return p(in);
}

static auto &read_user_input(std::string &line) {
  return std::getline(std::cin, line);
}
int main() {
  llvm::LLVMContext ctx;
  llvm::Module m{"brainf", ctx};
  llvm::IRBuilder<> builder{ctx};

  while (std::cin) {
    for (std::string line; read_user_input(line);) {
      const auto r = many_ops({line.data(), line.size()});
      std::cout << std::string_view{"OK"} << "\n";
    }
  }
}
