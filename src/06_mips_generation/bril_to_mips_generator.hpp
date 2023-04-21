
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
  bool uses_heap = false;
  bool uses_print = false;
  std::map<std::string, RegisterAllocation> allocations;
  std::map<std::string, std::map<std::string, LivenessData>> liveness_data;

  BRILToMIPSGenerator(const Program &program) : program(program) {
    uses_heap = program.uses_heap();
    uses_print = program.uses_print();
    generate();
  }

private:
  void compute_allocations() {
    for (const auto &[name, function] : program.cfgs) {
      std::cerr << "Computing register allocation for function " << name
                << std::endl;

      allocations[function.name] =
          allocate_registers(function, available_registers);
      liveness_data[function.name] = allocations[function.name].liveness_data;
    }
  }

  void generate() {
    compute_allocations();

    std::cerr << program << std::endl;

    // Load the arguments to wain into the correct registers
    const auto &wain = program.wain();
    const auto wain_allocations = allocations.at(wain.name);

    std::stringstream ss;
    program.print_flattened(ss);
    std::string line;
    while (std::getline(ss, line)) {
      comment(line);
    }

    if (uses_heap) {
      import("init");
      import("new");
      import("delete");
    }

    if (uses_print) {
      import("print");
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

    if (uses_heap) {
      comment("Calling init");
      const bool first_arg_is_array = wain.arguments[0].type == Type::IntStar;
      if (!first_arg_is_array) {
        add(2, 0, 0);
      }
      push(31);
      load_and_jalr(1, "init");
      pop(31);
      comment("Done calling init");
    }

    // Set up the stack pointer
    const int stack_size = wain_allocations.spilled_variables.size();
    load_const(1, 4 * stack_size - 4);
    sub(30, 29, 1);
    annotate("$30 = $29 - (" + std::to_string(4 * stack_size - 4) + ")");

    // Jump to wain
    beq(0, 0, wain.entry_label);
    annotate("Done prologue, jumping to wain");

    // Generate code for all the functions
    for (const auto &[name, function] : program.cfgs) {
      generate_function(function);
    }

    optimize();

    comment("Number of instructions: " +
            std::to_string(num_assembly_instructions()));
  }

  void optimize() {
    while (true) {
      bool changed = false;
      changed |= remove_fallthrough_jumps();
      changed |= remove_unused_labels();
      changed |= remove_globally_unused_writes();
      changed |= remove_locally_unused_writes();
      changed |= collapse_moves();
      if (!changed)
        break;
    }
  }

  bool remove_globally_unused_writes();
  bool remove_locally_unused_writes();
  bool remove_fallthrough_jumps();
  bool remove_unused_labels();
  bool collapse_moves();

  void generate_function(const ControlFlowGraph &function) {
    const RegisterAllocation allocation = allocations.at(function.name);
    comment("Code for function " + function.name);
    label(function.entry_label);
    for (const auto &label : function.block_labels) {
      const auto block = function.get_block(label);
      const auto &live_variables =
          allocation.liveness_data.at(label).live_variables;
      for (size_t i = 0; i < block.instructions.size(); ++i) {
        const auto &instruction = block.instructions[i];
        generate_instruction(instruction, live_variables[i],
                             live_variables[i + 1], allocation);
      }
    }
    comment("Done with function " + function.name);
  }

  size_t load_variable(const size_t temp_reg, const std::string &argument,
                       const RegisterAllocation &allocation) {
    if (allocation.in_register(argument)) {
      return allocation.get_register(argument);
    } else {
      runtime_assert(allocation.is_spilled(argument),
                     "Variable " + argument +
                         " is not in a register nor on the stack");
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

  void generate_instruction(
      const Instruction &instruction,
      [[maybe_unused]] const std::set<std::string> &live_variables_before,
      const std::set<std::string> &live_variables_after,
      const RegisterAllocation &allocation) {
    const std::string &dest = instruction.destination;

    switch (instruction.opcode) {
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
      slt(tmp3, lhs_reg, rhs_reg);
      slt(tmp4, rhs_reg, lhs_reg);
      add(dest_reg, tmp3, tmp4);
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
      slt(tmp3, lhs_reg, rhs_reg);
      slt(tmp4, rhs_reg, lhs_reg);
      add(dest_reg, tmp3, tmp4);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Jmp: {
      const std::string &label = instruction.labels[0];
      beq(0, 0, label);
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

    case Opcode::Call: {
      const std::string label = instruction.funcs[0];
      const auto &called_function = program.get_function(label);
      const auto &function_allocation = allocations.at(called_function.name);

      // TODO: Test this
      std::cerr << "Generating function call " << instruction.to_string()
                << std::endl;

      // 1. Save the live registers, including $29
      std::set<size_t> live_registers = {29};
      for (const auto &var : live_variables_after) {
        if (var != dest && allocation.in_register(var))
          live_registers.insert(allocation.get_register(var));
      }

      comment("Generating function call: " + instruction.to_string());
      comment("1. Save the live registers");
      for (const size_t reg : live_registers)
        push(reg);

      // 2. Subtract 4 from $30 to obtain the new base pointer
      sub(30, 30, 4);
      annotate("2. Obtain the new base pointer");

      // 3. For each argument in memory, copy it to the stack
      //    for each argument in register, load it into the register
      // NOTE: We have to preserve $29 to access variables on the stack
      const size_t stack_size =
          function_allocation.spilled_variables.size() * 4;
      for (size_t i = 0; i < called_function.arguments.size(); ++i) {
        const auto &parameter = instruction.arguments[i];
        const auto &argument = called_function.arguments[i];
        if (function_allocation.is_spilled(argument.name)) {
          const int offset = function_allocation.get_offset(argument.name);
          const size_t src_reg = load_variable(1, parameter, allocation);
          sw(src_reg, offset, 30);
        } else if (function_allocation.in_register(argument.name)) {
          const size_t dest_reg =
              function_allocation.get_register(argument.name);
          const size_t src_reg = load_variable(1, parameter, allocation);
          copy(dest_reg, src_reg);
        } else {
          runtime_assert(false, "Argument " + argument.name +
                                    " not in memory or register");
        }
      }
      add(29, 30, 0);
      load_const(1, stack_size * 4 - 4);
      sub(30, 30, 1);
      comment("3. Done copying arguments to " + called_function.name);

      // 4. Jump to the function
      push(31);
      load_and_jalr(2, called_function.entry_label);
      pop(31);
      annotate("4. Jump to " + called_function.name);

      // 5. Restore the stack pointer
      comment("5. Restore the stack pointer");
      load_const(1, stack_size * 4);
      add(30, 30, 1);

      // 6. Pop the saved registers off the stack
      comment("6. Pop the saved registers off the stack");
      for (auto it = live_registers.rbegin(); it != live_registers.rend(); ++it)
        pop(*it);
      comment("7. Done with function call");

      const size_t dest_reg = get_register(tmp1, dest, allocation);
      copy(dest_reg, 3);
      store_variable(dest, dest_reg, allocation);
      comment("8. Copy return value to " + dest);
    } break;

    case Opcode::Ret: {
      const size_t return_value_reg =
          load_variable(3, instruction.arguments[0], allocation);
      copy(3, return_value_reg);
      comment(instruction.to_string());
      jr(31);
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

    case Opcode::Print: {
      const size_t arg_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      copy(1, arg_reg);
      push(31);
      load_and_jalr(2, "print");
      pop(31);
      annotate(instruction.to_string());
    } break;

    case Opcode::Nop: {
      // Do nothing
    } break;

    case Opcode::Alloc: {
      const size_t arg_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t dest_reg = get_register(3, dest, allocation);
      const std::string alloc_success = generate_label("allocSuccess");

      // Argument in $1
      // Result in $3: if it was 0, return NULL (1)

      copy(1, arg_reg);
      push(3);
      push(31);
      load_and_jalr(2, "new");
      pop(31);
      bne(3, 0, alloc_success);
      add(3, 11, 0);
      label(alloc_success);
      copy(dest_reg, 3);
      if (dest_reg != 3) {
        pop(3);
      } else {
        pop_and_discard();
      }
      store_variable(dest, dest_reg, allocation);
    } break;

    case Opcode::Free: {
      const auto skip_label = generate_label("deleteSkip");
      const size_t arg_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      copy(1, arg_reg);
      beq(1, 11, skip_label);
      push(31);
      load_and_jalr(2, "delete");
      pop(31);
      label(skip_label);
    } break;

    case Opcode::Store: {
      // store location value
      const size_t dest_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t src_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      sw(src_reg, 0, dest_reg);
      annotate(instruction.to_string());
    } break;

    case Opcode::Load: {
      // dest = load location
      const size_t src_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t dest_reg = get_register(tmp2, dest, allocation);
      lw(dest_reg, 0, src_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::PointerAdd: {
      // dest = ptradd ptr offset
      // -> dest = ptr + 4 * offset
      const size_t ptr_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t offset_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp1, dest, allocation);
      copy(tmp1, offset_reg);
      add(tmp1, tmp1, tmp1);
      add(tmp1, tmp1, tmp1);
      add(dest_reg, ptr_reg, tmp1);
      store_variable(dest, dest_reg, allocation);
    } break;

    case Opcode::PointerSub: {
      const size_t ptr_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t offset_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp1, dest, allocation);
      copy(tmp1, offset_reg);
      add(tmp1, tmp1, tmp1);
      add(tmp1, tmp1, tmp1);
      sub(dest_reg, ptr_reg, tmp1);
      store_variable(dest, dest_reg, allocation);
    } break;

    case Opcode::PointerDiff: {
      const size_t ptr1_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t ptr2_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp1, dest, allocation);
      sub(dest_reg, ptr1_reg, ptr2_reg);
      div(dest_reg, dest_reg, 4);
      store_variable(dest, dest_reg, allocation);
    } break;

    case Opcode::AddressOf: {
      const std::string &var = instruction.arguments[0];
      const size_t dest_reg = get_register(tmp1, dest, allocation);
      runtime_assert(allocation.is_spilled(var),
                     "Addressed variable " + var + " is not in memory");
      const int offset = allocation.get_offset(var);
      load_const(tmp1, offset);
      add(dest_reg, 29, tmp1);
      store_variable(dest, dest_reg, allocation);
    } break;

    case Opcode::Label: {
      label(instruction.labels[0]);
    } break;

    default: {
      unreachable("Unsupported instruction " + instruction.to_string());
    } break;
    }
  }
};

} // namespace bril