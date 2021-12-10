#include "context.hpp"
#include "globals.hpp"
#include "ops.hpp"

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  if (argc == 1) {
    std::cerr << "Missing filename\n";
    return 1;
  }

  bf::globals g{"brainf"};
  bf::context c{&g};
  std::vector<std::unique_ptr<bf::ops>> stack{};
  stack.emplace_back(new bf::ops{c, c.main_entry(), c.main_exit(), c.zero()});

  auto f = std::fstream{argv[1]};
  while (f) {
    auto chr = f.get();
    auto &ops = *stack.back();

    switch (chr) {
    case '+':
      ops.plus();
      break;
    case '-':
      ops.minus();
      break;
    case '>':
      ops.inc();
      break;
    case '<':
      ops.dec();
      break;
    case ',':
      ops.in();
      break;
    case '.':
      ops.out();
      break;
    case '[':
      stack.emplace_back(ops.begin_loop());
      break;
    case ']': {
      auto back = std::move(stack.back());
      stack.pop_back();
      stack.back()->end_loop(*back);
      break;
    }
    }
  }

  stack.back()->finish();
  c.finish();
  return g.finish();
}
