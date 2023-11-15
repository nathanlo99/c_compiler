
#pragma once

#include "mips_instruction.hpp"

#include <iostream>

struct MIPSGenerator {
  std::vector<MIPSInstruction> instructions;
  bool constants_init = false;

  void annotate(const std::string &comment) {
    debug_assert(instructions.size() > 0, "No instructions to annotate");
    instructions.back().comment_value = comment;
  }

  void init_constants() {
    if (constants_init)
      return;
    load_const(Reg::R4, 4);
    slt(Reg::R11, Reg::R0, Reg::R4);
    annotate("$11 = ($0 < $4) = 1");
    constants_init = true;
  }

  template <typename T> void push_registers(const T &registers) {
    for (const auto &reg : registers)
      push(reg);
  }
  template <typename T> void pop_registers(const T &registers) {
    for (auto rit = registers.rbegin(); rit != registers.rend(); ++rit)
      pop(*rit);
  }

  void pop_and_discard(const size_t num_values = 1) {
    // Two options: one is to repeat 'add $30, $30, $4', num_values times
    // The other is lis, word, add
    // The second option is better when num_values > 3
    if (num_values > 3) {
      // NOTE: We really don't want to use $3 here because this happens after
      // the return value of a function is computed, and we don't want to
      // clobber it
      load_const(Reg::R5, num_values * 4);
      add(Reg::SP, Reg::SP, Reg::R5);
    } else {
      for (size_t i = 0; i < num_values; ++i)
        add(Reg::SP, Reg::SP, Reg::R4);
    }
  }

  std::string generate_label(const std::string &label_type) {
    static std::unordered_map<std::string, int> next_idx;
    const int idx = next_idx[label_type]++;
    return label_type + std::to_string(idx);
  }

  void load_const(const Reg reg, const int64_t value) {
    if (value == 0) {
      add(reg, Reg::R0, Reg::R0);
    } else if (value == -4 && constants_init) {
      sub(reg, Reg::R0, Reg::R4);
    } else if (value == -3 && constants_init) {
      sub(reg, Reg::R11, Reg::R4);
    } else if (value == -1 && constants_init) {
      sub(reg, Reg::R0, Reg::R1);
    } else if (value == 1 && constants_init) {
      add(reg, Reg::R11, Reg::R0);
    } else if (value == 2 && constants_init) {
      add(reg, Reg::R11, Reg::R11);
    } else if (value == 3 && constants_init) {
      sub(reg, Reg::R4, Reg::R11);
    } else if (value == 4 && constants_init) {
      add(reg, Reg::R4, Reg::R0);
    } else if (value == 5 && constants_init) {
      add(reg, Reg::R11, Reg::R4);
    } else if (value == 8 && constants_init) {
      add(reg, Reg::R4, Reg::R4);
    } else {
      lis(reg);
      word(value);
    }
  }

  void load_const(const Reg reg, const std::string &label) {
    lis(reg);
    word(label);
  }

  void add_const(const Reg reg, const Reg src, const int64_t value,
                 const Reg temp_reg) {
    if (value == 0) {
      copy(reg, src);
    } else if (value == 1) {
      add(reg, src, Reg::R11);
    } else if (value == 4) {
      add(reg, src, Reg::R4);
    } else if (value == -4) {
      sub(reg, src, Reg::R4);
    } else if (value == -1) {
      sub(reg, src, Reg::R11);
    } else {
      load_const(temp_reg, value);
      add(reg, src, temp_reg);
    }
  }

  void push_const(const Reg reg, const int64_t value) {
    if (value == 0) {
      push(Reg::R0);
    } else if (value == 1 && constants_init) {
      push(Reg::R11);
    } else if (value == 4 && constants_init) {
      push(Reg::R4);
    } else {
      load_const(reg, value);
      push(reg);
    }
  }

  void load_and_jalr(const Reg reg, const std::string &label) {
    load_const(reg, label);
    jalr(reg);
  }

  void push(const Reg reg) {
    sw(reg, -4, Reg::SP);
    annotate(fmt::format("  push {}", reg));
    sub(Reg::SP, Reg::SP, Reg::R4);
    annotate("  ^");
  }

  void pop(const Reg reg) {
    add(Reg::SP, Reg::SP, Reg::R4);
    annotate(fmt::format("  pop {}", reg));
    lw(reg, -4, Reg::SP);
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
  void copy(Reg d, Reg s) {
    if (d != s)
      add(d, s, Reg::R0);
  }
  void mult(Reg d, Reg s, Reg t) {
    mult(s, t);
    mflo(d);
  }
  void multu(Reg d, Reg s, Reg t) {
    multu(s, t);
    mflo(d);
  }
  void div(Reg d, Reg s, Reg t) {
    div(s, t);
    mflo(d);
  }
  void divu(Reg d, Reg s, Reg t) {
    divu(s, t);
    mflo(d);
  }
  void mod(Reg d, Reg s, Reg t) {
    div(s, t);
    mfhi(d);
  }
  void modu(Reg d, Reg s, Reg t) {
    divu(s, t);
    mfhi(d);
  }

  // Raw instructions
  void add(Reg d, Reg s, Reg t) {
    instructions.push_back(MIPSInstruction::add(d, s, t));
  }
  void sub(Reg d, Reg s, Reg t) {
    instructions.push_back(MIPSInstruction::sub(d, s, t));
  }
  void mult(Reg s, Reg t) {
    instructions.push_back(MIPSInstruction::mult(s, t));
  }
  void multu(Reg s, Reg t) {
    instructions.push_back(MIPSInstruction::multu(s, t));
  }
  void div(Reg s, Reg t) { instructions.push_back(MIPSInstruction::div(s, t)); }
  void divu(Reg s, Reg t) {
    instructions.push_back(MIPSInstruction::divu(s, t));
  }
  void mfhi(Reg d) { instructions.push_back(MIPSInstruction::mfhi(d)); }
  void mflo(Reg d) { instructions.push_back(MIPSInstruction::mflo(d)); }
  void lis(Reg d) { instructions.push_back(MIPSInstruction::lis(d)); }
  void lw(Reg t, int32_t i, Reg s) {
    instructions.push_back(MIPSInstruction::lw(t, i, s));
  }
  void sw(Reg t, int32_t i, Reg s) {
    instructions.push_back(MIPSInstruction::sw(t, i, s));
  }
  void slt(Reg d, Reg s, Reg t) {
    instructions.push_back(MIPSInstruction::slt(d, s, t));
  }
  void sltu(Reg d, Reg s, Reg t) {
    instructions.push_back(MIPSInstruction::sltu(d, s, t));
  }
  void beq(Reg s, Reg t, int32_t i) {
    instructions.push_back(MIPSInstruction::beq(s, t, i));
  }
  void beq(Reg s, Reg t, const std::string &label) {
    instructions.push_back(MIPSInstruction::beq(s, t, label));
  }
  void jmp(int32_t i) { beq(Reg::R0, Reg::R0, i); }
  void jmp(const std::string &label) { beq(Reg::R0, Reg::R0, label); }

  void bne(Reg s, Reg t, int32_t i) {
    instructions.push_back(MIPSInstruction::bne(s, t, i));
  }
  void bne(Reg s, Reg t, const std::string &label) {
    instructions.push_back(MIPSInstruction::bne(s, t, label));
  }
  void jr(Reg s) { instructions.push_back(MIPSInstruction::jr(s)); }
  void jalr(Reg s) { instructions.push_back(MIPSInstruction::jalr(s)); }
  void word(int64_t i) { instructions.push_back(MIPSInstruction::word(i)); }
  void word(const std::string &label) {
    instructions.push_back(MIPSInstruction::word(label));
  }
  void label(const std::string &name) {
    instructions.push_back(MIPSInstruction::label(name));
  }
  void import_module(const std::string &value) {
    instructions.push_back(MIPSInstruction::import_module(value));
  }
  void comment(const std::string &value) {
    instructions.push_back(MIPSInstruction::comment(value));
  }
};
