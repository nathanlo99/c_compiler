
#include "mips_instruction.hpp"

#include <iostream>

struct MIPSGenerator {
  std::vector<MIPSInstruction> instructions;

  void annotate(const std::string &comment) {
    instructions.back().comment_value = comment;
  }

  void save_registers() {
    instructions.push_back(MIPSInstruction::comment("Saving registers"));
    push(1);
    push(2);
    push(6);
    push(7);
  }

  void pop_registers() {
    instructions.push_back(MIPSInstruction::comment("Pop registers"));
    pop(7);
    pop(6);
    pop(2);
    pop(1);
  }

  void pop_and_discard(const size_t num_values) {
    // Two options: one is to repeat 'add $30, $30, $4', stack_size times
    // The other is lis, word, add
    // The second option is better when stack_size > 3
    if (num_values > 3) {
      // NOTE: We really don't want to use $3 here because this happens after
      // the return value of a function is computed, and we don't want to
      // clobber it
      load_const(5, num_values * 4);
      instructions.push_back(MIPSInstruction::add(30, 30, 5));
    } else {
      for (size_t i = 0; i < num_values; ++i)
        instructions.push_back(MIPSInstruction::add(30, 30, 4));
    }
  }

  std::string generate_label(const std::string &label_type) {
    static std::map<std::string, int> next_idx;
    const int idx = next_idx[label_type]++;
    return label_type + std::to_string(idx);
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

  void load_and_jalr(const std::string &label) {
    load_const(5, label);
    jalr(5);
  }

  void push(int reg) {
    instructions.push_back(MIPSInstruction::sw(reg, -4, 30));
    annotate("  push " + std::to_string(reg));
    instructions.push_back(MIPSInstruction::sub(30, 30, 4));
    annotate("  ^");
  }

  void pop(int reg) {
    instructions.push_back(MIPSInstruction::add(30, 30, 4));
    annotate("  pop " + std::to_string(reg));
    instructions.push_back(MIPSInstruction::lw(reg, -4, 30));
    annotate("  ^");
  }

  void print() {
    for (const auto &instruction : instructions) {
      std::cout << instruction.to_string() << std::endl;
    }
  }

  size_t num_assembly_instructions() const {
    size_t result = 0;
    for (const auto &instruction : instructions) {
      if (instruction.opcode != Opcode::Comment &&
          instruction.opcode != Opcode::Label)
        result++;
    }
    return result;
  }

  // Convenience functions
  void add(int d, int s, int t) {
    instructions.push_back(MIPSInstruction::add(d, s, t));
  }
  void sub(int d, int s, int t) {
    instructions.push_back(MIPSInstruction::sub(d, s, t));
  }
  void mult(int s, int t) {
    instructions.push_back(MIPSInstruction::mult(s, t));
  }
  void multu(int s, int t) {
    instructions.push_back(MIPSInstruction::multu(s, t));
  }
  void div(int s, int t) { instructions.push_back(MIPSInstruction::div(s, t)); }
  void divu(int s, int t) {
    instructions.push_back(MIPSInstruction::divu(s, t));
  }
  void mfhi(int d) { instructions.push_back(MIPSInstruction::mfhi(d)); }
  void mflo(int d) { instructions.push_back(MIPSInstruction::mflo(d)); }
  void lis(int d) { instructions.push_back(MIPSInstruction::lis(d)); }
  void lw(int t, int32_t i, int s) {
    instructions.push_back(MIPSInstruction::lw(t, i, s));
  }
  void sw(int t, int32_t i, int s) {
    instructions.push_back(MIPSInstruction::sw(t, i, s));
  }
  void slt(int d, int s, int t) {
    instructions.push_back(MIPSInstruction::slt(d, s, t));
  }
  void sltu(int d, int s, int t) {
    instructions.push_back(MIPSInstruction::sltu(d, s, t));
  }
  void beq(int s, int t, int i) {
    instructions.push_back(MIPSInstruction::beq(s, t, i));
  }
  void beq(int s, int t, const std::string &label) {
    instructions.push_back(MIPSInstruction::beq(s, t, label));
  }
  void bne(int s, int t, int i) {
    instructions.push_back(MIPSInstruction::bne(s, t, i));
  }
  void bne(int s, int t, const std::string &label) {
    instructions.push_back(MIPSInstruction::bne(s, t, label));
  }
  void jr(int s) { instructions.push_back(MIPSInstruction::jr(s)); }
  void jalr(int s) { instructions.push_back(MIPSInstruction::jalr(s)); }
  void word(int32_t i) { instructions.push_back(MIPSInstruction::word(i)); }
  void word(const std::string &label) {
    instructions.push_back(MIPSInstruction::word(label));
  }
  void label(const std::string &name) {
    instructions.push_back(MIPSInstruction::label(name));
  }
  void import(const std::string &value) {
    instructions.push_back(MIPSInstruction::import(value));
  }
  void comment(const std::string &value) {
    instructions.push_back(MIPSInstruction::comment(value));
  }
};
