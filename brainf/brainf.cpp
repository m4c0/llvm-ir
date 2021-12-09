#include <iostream>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <string>

static auto &read_user_input(std::string &line) {
  return std::getline(std::cin, line);
}
int main() {
  while (std::cin) {
    for (std::string line; read_user_input(line);) {
    }
  }
}
