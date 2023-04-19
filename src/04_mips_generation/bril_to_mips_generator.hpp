
#pragma once

#include "bril.hpp"
#include "live_analysis.hpp"
#include "mips_generator.hpp"
#include "mips_instruction.hpp"
#include "util.hpp"

namespace bril {

struct BRILToMIPSGenerator : MIPSGenerator {
  static const inline std::vector<size_t> available_registers = {
      3,  5,  6,  7,  8,  9,  10, 12, 13, 14, 15,
      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26};
  static const size_t tmp1 = 1, tmp2 = 2, tmp3 = 27, tmp4 = 28;

  const Program &program;
  std::map<std::string, RegisterAllocation> allocations;

  BRILToMIPSGenerator(const Program &program) : program(program) { generate(); }

private:
  void compute_allocations() {
    for (const auto &[name, function] : program.cfgs) {
      std::cerr << "Computing register allocation for function " << name
                << std::endl;
      allocations[function.name] =
          allocate_registers(function, available_registers);
    }
  }

  void generate() {
    compute_allocations();

    // Load the arguments to wain into the correct registers
    const auto &wain = program.wain();
    const auto wain_allocations = allocations.at(wain.name);

    std::stringstream ss;
    program.print_flattened(ss);
    std::string line;
    while (std::getline(ss, line)) {
      comment(line);
    }

    // Initialize constants and set the base pointer
    init_constants();
    sub(29, 30, 4);
    annotate("Initializing base pointer");

    // Load the arguments ($1 and $2) into the registers expected by wain
    const auto arg1 = wain.arguments[0].name;
    const auto arg2 = wain.arguments[1].name;
    if (wain_allocations.in_register(arg1)) {
      copy(wain_allocations.get_register(arg1), 1);
      annotate("Loading argument 1 into register");
    } else {
      const int offset = wain_allocations.get_offset(arg1);
      sw(1, offset, 29);
      annotate("Loading argument 1 into variable " + arg1);
    }
    if (wain_allocations.in_register(arg2)) {
      copy(wain_allocations.get_register(arg2), 2);
      annotate("Loading argument 2 into register");
    } else {
      const int offset = wain_allocations.get_offset(arg2);
      sw(2, offset, 29);
      annotate("Loading argument 2 into variable " + arg2);
    }
    // Jump to wain
    beq(0, 0, wain.entry_label);
    annotate("Done prologue, jumping to wain");

    // Generate code for all the functions
    for (const auto &[name, function] : program.cfgs) {
      generate_function(function);
    }

    // Whenever an unconditional jump is followed by the label it jumps to,
    // remove it
    for (size_t i = 0; i + 1 < instructions.size(); ++i) {
      const auto &this_instruction = instructions[i];
      const auto &next_instruction = instructions[i + 1];
      if (this_instruction.opcode != ::Opcode::Beq ||
          next_instruction.opcode != ::Opcode::Label)
        continue;
      if (this_instruction.s == this_instruction.t &&
          this_instruction.string_value == next_instruction.string_value) {
        instructions[i] = MIPSInstruction::comment(
            "jmp " + next_instruction.string_value + "; (fallthrough)");
      }
    }

    comment("Number of instructions: " +
            std::to_string(num_assembly_instructions()));
  }

  void generate_function(const ControlFlowGraph &function) {
    const RegisterAllocation allocation = allocations.at(function.name);
    std::cerr << allocation << std::endl;

    comment("Code for function " + function.name);
    label(function.entry_label);
    for (const auto &instruction : function.flatten()) {
      generate_instruction(instruction, allocation);
    }
    comment("Done with function " + function.name);
  }

  size_t load_variable(const size_t temp_reg, const std::string &argument,
                       const RegisterAllocation &allocation) {
    if (allocation.in_register(argument)) {
      return allocation.get_register(argument);
    } else {
      const int offset = allocation.get_offset(argument);
      lw(temp_reg, offset, 29);
      annotate("Loading variable " + argument + " from offset " +
               std::to_string(offset));
      return temp_reg;
    }
  }
  void store_variable(const std::string &variable, const size_t temp_reg,
                      const RegisterAllocation &allocation) {
    if (allocation.is_spilled(variable)) {
      const int offset = allocation.get_offset(variable);
      sw(temp_reg, offset, 29);
      annotate("Storing variable " + variable + " to offset " +
               std::to_string(offset));
    }
  }

  inline size_t get_register(const size_t temp_reg, const std::string &variable,
                             const RegisterAllocation &allocation) const {
    return allocation.in_register(variable) ? allocation.get_register(variable)
                                            : temp_reg;
  }

  void generate_instruction(const Instruction &instruction,
                            const RegisterAllocation &allocation) {
    const std::string &dest = instruction.destination;

    switch (instruction.opcode) {
    case Opcode::Label: {
      label(instruction.labels[0]);
    } break;

    case Opcode::Const: {
      const size_t dest_reg = get_register(tmp1, dest, allocation);
      load_const(dest_reg, instruction.value);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Id: {
      const size_t src_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t dest_reg = get_register(tmp1, dest, allocation);
      copy(dest_reg, src_reg);
      store_variable(dest, dest_reg, allocation);
      comment(instruction.to_string());
    } break;

    case Opcode::Add: {
      const size_t lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp1, dest, allocation);
      add(dest_reg, lhs_reg, rhs_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Sub: {
      const size_t lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp1, dest, allocation);
      sub(dest_reg, lhs_reg, rhs_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Mul: {
      const size_t lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp1, dest, allocation);
      mult(dest_reg, lhs_reg, rhs_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Div: {
      const size_t lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp1, dest, allocation);
      div(dest_reg, lhs_reg, rhs_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Mod: {
      const size_t lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp1, dest, allocation);
      div(lhs_reg, rhs_reg);
      mfhi(dest_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Lt: {
      const size_t lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp3, dest, allocation);
      slt(dest_reg, lhs_reg, rhs_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Le: {
      const size_t lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp3, dest, allocation);
      slt(dest_reg, rhs_reg, lhs_reg);
      sub(dest_reg, 11, dest_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Gt: {
      const size_t lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp3, dest, allocation);
      slt(dest_reg, rhs_reg, lhs_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Ge: {
      const size_t lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp3, dest, allocation);
      slt(dest_reg, lhs_reg, rhs_reg);
      sub(dest_reg, 11, dest_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Eq: {
      const size_t lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp3, dest, allocation);
      slt(dest_reg, lhs_reg, rhs_reg);
      slt(tmp4, rhs_reg, lhs_reg);
      add(dest_reg, dest_reg, tmp4);
      sub(dest_reg, 11, dest_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Ne: {
      const size_t lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp3, dest, allocation);
      slt(dest_reg, lhs_reg, rhs_reg);
      slt(tmp4, rhs_reg, lhs_reg);
      add(dest_reg, dest_reg, tmp4);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Br: {
      const size_t condition_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const std::string &true_label = instruction.labels[0];
      const std::string &false_label = instruction.labels[1];
      beq(condition_reg, 0, false_label);
      annotate(instruction.to_string());
      beq(0, 0, true_label);
    } break;

    case Opcode::Jmp: {
      const std::string &label = instruction.labels[0];
      beq(0, 0, label);
      annotate(instruction.to_string());
    } break;

    case Opcode::Ret: {
      const size_t return_value_reg =
          load_variable(3, instruction.arguments[0], allocation);
      copy(3, return_value_reg);
      comment(instruction.to_string());
      jr(31);
    } break;

    default: {
      unreachable("Unsupported instruction " + instruction.to_string());
    } break;
    }
  }
};

} // namespace bril
