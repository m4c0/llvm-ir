#include "context.hpp"
#include "globals.hpp"
#include "ops.hpp"

#include <fstream>
#include <iostream>
#include <string>

static llvm::cl::opt<std::string>
    g_input_filename(llvm::cl::Positional, llvm::cl::desc("<input brainf>"));

auto run(std::istream &f) {
  bf::globals g{"brainf"};
  bf::context c{&g};
  std::vector<std::unique_ptr<bf::ops>> stack{};
  stack.emplace_back(new bf::ops{c, c.main_entry(), c.main_exit(), c.zero()});

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
  return c.finish_and_run();
}

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv,
                                    "BrainF compiler and interpreter\n");

  if (g_input_filename == "") {
    llvm::errs() << "Missing filename. Use --help to see options.\n";
    return 1;
  }

  if (g_input_filename == "-") {
    return run(std::cin);
  }
  std::fstream f{g_input_filename};
  return run(f);
}
