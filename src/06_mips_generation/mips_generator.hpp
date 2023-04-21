
#pragma once

#include "mips_instruction.hpp"

#include <iostream>

struct MIPSGenerator {
  std::vector<MIPSInstruction> instructions;
  bool constants_init = false;

  void annotate(const std::string &comment) {
    runtime_assert(instructions.size() > 0, "No instructions to annotate");
    instructions.back().comment_value = comment;
  }

  void init_constants() {
    if (constants_init)
      return;
    load_const(4, 4);
    slt(11, 0, 4);
    annotate("$11 = ($0 < $4) = 1");
    constants_init = true;
  }

  void pop_and_discard(const size_t num_values = 1) {
    // Two options: one is to repeat 'add $30, $30, $4', stack_size times
    // The other is lis, word, add
    // The second option is better when stack_size > 3
    if (num_values > 3) {
      // NOTE: We really don't want to use $3 here because this happens after
      // the return value of a function is computed, and we don't want to
      // clobber it
      load_const(5, num_values * 4);
      add(30, 30, 5);
    } else {
      for (size_t i = 0; i < num_values; ++i)
        add(30, 30, 4);
    }
  }

  std::string generate_label(const std::string &label_type) {
    static std::map<std::string, int> next_idx;
    const int idx = next_idx[label_type]++;
    return label_type + std::to_string(idx);
  }

  void load_const(int reg, int value) {
    if (value == 0) {
      add(reg, 0, 0);
    } else if (value == -4 && constants_init) {
      sub(reg, 0, 4);
    } else if (value == -3 && constants_init) {
      sub(reg, 11, 4);
    } else if (value == -1 && constants_init) {
      sub(reg, 0, 1);
    } else if (value == 1 && constants_init) {
      add(reg, 11, 0);
    } else if (value == 2 && constants_init) {
      add(reg, 11, 11);
    } else if (value == 3 && constants_init) {
      sub(reg, 4, 11);
    } else if (value == 4 && constants_init) {
      add(reg, 4, 0);
    } else if (value == 5 && constants_init) {
      add(reg, 11, 4);
    } else if (value == 8 && constants_init) {
      add(reg, 4, 4);
    } else {
      lis(reg);
      word(value);
    }
  }

  void load_const(int reg, const std::string &label) {
    lis(reg);
    word(label);
  }

  void push_const(int reg, int value) {
    if (value == 0) {
      push(0);
    } else if (value == 1 && constants_init) {
      push(11);
    } else if (value == 4 && constants_init) {
      push(4);
    } else {
      load_const(reg, value);
      push(reg);
    }
  }

  void load_and_jalr(const size_t reg, const std::string &label) {
    load_const(reg, label);
    jalr(reg);
  }

  void push(int reg) {
    sw(reg, -4, 30);
    annotate("  push $" + std::to_string(reg));
    sub(30, 30, 4);
    annotate("  ^");
  }

  void pop(int reg) {
    add(30, 30, 4);
    annotate("  pop $" + std::to_string(reg));
    lw(reg, -4, 30);
    annotate("  ^");
  }

  void print(std::ostream &os) {
    for (const auto &instruction : instructions) {
      os << instruction.to_string() << std::endl;
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

  // Optimizations
  void optimize() {
    while (true) {
      bool changed = false;
      // changed |= peephole_optimize();
      // changed |= optimize_moves();
      if (!changed)
        break;
    }
  }
  bool peephole_optimize();
  bool optimize_moves();

  // Convenience functions
  void copy(int d, int s) {
    if (d != s)
      add(d, s, 0);
  }
  void mult(int d, int s, int t) {
    mult(s, t);
    mflo(d);
  }
  void multu(int d, int s, int t) {
    multu(s, t);
    mflo(d);
  }
  void div(int d, int s, int t) {
    div(s, t);
    mflo(d);
  }
  void divu(int d, int s, int t) {
    divu(s, t);
    mflo(d);
  }
  void mod(int d, int s, int t) {
    div(s, t);
    mfhi(d);
  }
  void modu(int d, int s, int t) {
    divu(s, t);
    mfhi(d);
  }

  // Raw instructions
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
