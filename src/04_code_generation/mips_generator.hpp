
#include "mips_instruction.hpp"

#include <iostream>

struct MIPSGenerator {
  std::vector<MIPSInstruction> instructions;

  std::string generate_label(const std::string &label_type) {
    static std::map<std::string, int> next_idx;
    const int idx = next_idx[label_type]++;
    return "_" + label_type + "_" + std::to_string(idx);
  }

  void load_const(int reg, int value) {
    if (value == 0) {
      instructions.push_back(MIPSInstruction::add(reg, 0, 0));
    } else {
      instructions.push_back(MIPSInstruction::lis(reg));
      instructions.push_back(MIPSInstruction::word(value));
    }
  }

  void load_const(int reg, const std::string &label) {
    instructions.push_back(MIPSInstruction::lis(reg));
    instructions.push_back(MIPSInstruction::word(label));
  }

  void push(int reg) {
    instructions.push_back(MIPSInstruction::sw(reg, -4, 30));
    instructions.push_back(MIPSInstruction::sub(30, 30, 4));
  }

  void pop(int reg) {
    instructions.push_back(MIPSInstruction::add(30, 30, 4));
    instructions.push_back(MIPSInstruction::lw(reg, -4, 30));
  }

  void print() {
    for (const auto &instruction : instructions) {
      std::cout << instruction.to_string() << std::endl;
    }
  }
};
