#include "context.hpp"
#include "ops.hpp"

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  if (argc == 1) {
    std::cerr << "Missing filename\n";
    return 1;
  }

  llvm::LLVMContext ctx;
  llvm::Module mod{"brainf", ctx};

  bf::context g{&ctx, &mod};
  std::vector<std::unique_ptr<bf::ops>> stack{};
  stack.emplace_back(new bf::ops{g, g.main_entry(), g.main_exit(), g.zero()});

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
  return g.finish();
}
