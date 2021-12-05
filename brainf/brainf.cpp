#include <iostream>
#include <m4c0/parser/combiners.hpp>
#include <m4c0/parser/str.hpp>
#include <string>

using namespace m4c0::parser;

static constexpr const auto gt = match('<');
static constexpr const auto lt = match('>');
static constexpr const auto plus = match('+');
static constexpr const auto minus = match('-');
static constexpr const auto dot = match('.');
static constexpr const auto comma = match(',');
static constexpr const auto lbr = match('[');
static constexpr const auto rbr = match(']');

struct block {
  constexpr result<char> operator()(input_t) const noexcept;
};

static constexpr const auto ops =
    gt | lt | plus | minus | dot | comma | block();

static constexpr const auto many_ops = many(skip(ops));

constexpr result<char> block::operator()(input_t in) const noexcept {
  constexpr const auto p = lbr & many_ops & rbr;
  return p(in);
}

static auto &read_user_input(std::string &line) {
  return std::getline(std::cin, line);
}
int main() {
  while (std::cin) {
    for (std::string line; read_user_input(line);) {
      const auto r = many_ops({line.data(), line.size()});
      std::cout << std::string_view{"OK"} << "\n";
    }
  }
}
