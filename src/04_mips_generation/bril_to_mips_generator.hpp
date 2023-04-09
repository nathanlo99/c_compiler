
#pragma once

#include "bril.hpp"
#include "mips_generator.hpp"

namespace bril {

struct BRILToMIPSGenerator : MIPSGenerator {
  void generate(const Program &program) {
    for (const auto &[name, function] : program.cfgs) {
      generate(function);
    }
  }
  void generate(const ControlFlowGraph &function) {
    for (const auto &instruction : function.flatten()) {
      generate(instruction);
    }
  }
  void generate(const Instruction &instruction);

  // TODO: Implement this
};

} // namespace bril
