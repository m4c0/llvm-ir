#include "context.hpp"
#include "globals.hpp"
#include "ops.hpp"

#include <iostream>
#include <string>

int main() {
  bf::globals g{"brainf"};

  // Handcrafted IR builder for this code from
  // http://www.brainfuck.org/short.b:
  //
  // ++++[>++++++++<-],[[>+.-<-]>.<,]
  // Show ASCII values of input in unary, separated by spaces.
  // (Useful for checking your implementation's newline behavior on input.)

  bf::context c{&g};

  bf::ops ops{c, c.main_entry(), c.main_exit(), c.zero()};
  // ++++
  for (auto i = 0; i < 4; i++) {
    ops.plus();
  }

  // [>++++++++<-]
  ops.loop([](bf::ops &ops) {
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
  ops.loop([](bf::ops &ops) {
    ops.loop([](bf::ops &ops) {
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

  ops.finish();
  c.finish();
  return g.finish();
}
