#include <iostream>
#include <string>

#include "context.hpp"
#include "ops.hpp"

using bf_globals = bf::context;
using bf_ops = bf::ops;

int main() {
  llvm::LLVMContext ctx;
  llvm::Module mod{"brainf", ctx};

  // Handcrafted IR builder for this code from http://www.brainfuck.org/short.b:
  //
  // ++++[>++++++++<-],[[>+.-<-]>.<,]
  // Show ASCII values of input in unary, separated by spaces.
  // (Useful for checking your implementation's newline behavior on input.)

  bf_globals g{&ctx, &mod};

  auto main = g.create_main_function();
  auto entry = g.create_basic_block("entry", main);
  auto data = g.create_data(entry);

  bf_ops ops{g, entry, data, g.zero()};
  // ++++
  for (auto i = 0; i < 4; i++) {
    ops.plus();
  }

  // [>++++++++<-]
  ops.loop([](bf_ops &ops) {
    ops.inc();
    for (auto i = 0; i < 8; i++) {
      ops.plus();
    }
    ops.dec();
    ops.minus();
  });

  // ,
  ops.in();

  // [[>+.-<-]>.<,]
  ops.loop([](bf_ops &ops) {
    ops.loop([](bf_ops &ops) {
      // [>+.-<-]
      ops.inc();
      ops.plus();
      ops.out();
      ops.minus();
      ops.dec();
      ops.minus();
    });

    // >.<,
    ops.inc();
    ops.out();
    ops.dec();
    ops.in();
  });

  ops.builder().CreateRet(g.zero());

  mod.print(llvm::outs(), nullptr);
  return llvm::verifyModule(mod, &llvm::errs()) ? 1 : 0;
}
