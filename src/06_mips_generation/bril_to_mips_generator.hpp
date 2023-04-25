
#pragma once

#include "bril.hpp"
#include "live_analysis.hpp"
#include "mips_generator.hpp"
#include "mips_instruction.hpp"
#include "util.hpp"

namespace bril {

struct BRILToMIPSGenerator : MIPSGenerator {
  static const inline std::vector<Reg> available_registers = {
      Reg::R3,  Reg::R5,  Reg::R8,  Reg::R9,  Reg::R10, Reg::R12,
      Reg::R13, Reg::R14, Reg::R15, Reg::R16, Reg::R17, Reg::R18,
      Reg::R19, Reg::R20, Reg::R21, Reg::R22, Reg::R23, Reg::R24,
      Reg::R25, Reg::R26, Reg::R27, Reg::R28};
  static const Reg tmp1 = Reg::R1, tmp2 = Reg::R2, tmp3 = Reg::R6,
                   tmp4 = Reg::R7;

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
  static std::string create_label(const std::string &function_name,
                                  const std::string &label_name) {
    return function_name + label_name.substr(1);
  }

  void compute_allocations() {
    program.for_each_function([&](const ControlFlowGraph &function) {
      allocations[function.name] =
          allocate_registers(function, available_registers);
      liveness_data[function.name] = allocations[function.name].liveness_data;
    });
  }

  void copy_arguments(const std::vector<VariableLocation> &source_locations,
                      const std::vector<VariableLocation> &target_locations) {
    comment("Copying arguments");
    const size_t num_arguments = source_locations.size();
    // Points from source registers to target registers
    std::map<Reg, Reg> register_graph;
    std::vector<size_t> to_memory;
    std::vector<size_t> from_memory;
    std::set<Reg> sink_nodes;

    for (size_t i = 0; i < num_arguments; ++i) {
      const auto source_location = source_locations[i];
      const auto target_location = target_locations[i];
      if (target_location.in_memory()) {
        to_memory.push_back(i);
      } else if (source_location.in_memory()) {
        from_memory.push_back(i);
      } else {
        debug_assert(register_graph.count(target_location.reg) == 0,
                     "Register graph has multiple edges");
        register_graph[target_location.reg] = source_location.reg;
        sink_nodes.insert(target_location.reg);
      }
    }
    for (const auto &[source, target] : register_graph) {
      sink_nodes.erase(target);
    }

    // Sink nodes are nodes with no outgoing edges
    // using util::operator<<;
    // std::cerr << "Graph: " << register_graph << std::endl;
    // std::cerr << "Sink nodes: " << sink_nodes << std::endl;

    // 1. Move arguments from registers to memory
    for (size_t i : to_memory) {
      const auto source_location = source_locations[i];
      const auto target_location = target_locations[i];
      if (source_location.in_memory()) {
        lw(tmp1, source_location.offset, Reg::R29);
        sw(tmp1, target_location.offset, Reg::R30);
        annotate(fmt::format("Copying argument {} from memory to memory", i));
      } else {
        sw(source_location.reg, target_location.offset, Reg::R30);
        annotate("Copying argument " + std::to_string(i) +
                 " from register to memory");
      }
    }

    // 2. Handle register to register moves
    // 2a. Handle chains: A -> B -> C -> D
    // - D is the sink, so we copy D to C, C to B, and B to A
    for (const auto &sink : sink_nodes) {
      Reg node = sink;
      while (register_graph.count(node) != 0) {
        const Reg next = register_graph[node];
        register_graph.erase(node);
        copy(node, next);
        // std::cerr << "$" << node << " <- $" << next << std::endl;
        annotate(fmt::format("Copying argument from register {} to register {}",
                             node, next));
        node = next;
      }
    }

    // 2b. Everything else is in a cycle
    // Z <- A <- B <- Y <- Z
    // - Copy Z into temp
    // - Copy Y into Z, B into Y, A into B
    // - Copy temp into A
    while (!register_graph.empty()) {
      auto node_next = *register_graph.begin();
      Reg node = node_next.first, next = node_next.second, start = node;
      register_graph.erase(node);
      if (node == next)
        continue;
      // std::cerr << "Beginning cycle: " << std::endl;
      copy(Reg::R1, node);
      // std::cerr << "$1 <- $" << node << std::endl;
      annotate(
          fmt::format("Copying argument from register {} to register 1", node));
      while (next != start) {
        node = next;
        next = register_graph[node];
        register_graph.erase(node);
        copy(next, node);
        // std::cerr << "$" << next << " <- $" << node << std::endl;
        annotate(fmt::format("Copying argument from register {} to register {}",
                             node, next));
      }
      copy(node, Reg::R1);
      // std::cerr << "$" << node << " <- $1" << std::endl;
      annotate(
          fmt::format("Copying argument from register 1 to register {}", node));
    }

    // 3. Move arguments from memory to registers
    for (size_t i : from_memory) {
      const auto source_location = source_locations[i];
      const auto target_location = target_locations[i];
      lw(target_location.reg, source_location.offset, Reg::R29);
      annotate(fmt::format("Copying argument {} from memory to register", i));
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
    sub(Reg::FP, Reg::SP, Reg::R4);
    annotate("Initializing base pointer");

    // Load the arguments ($1 and $2) into the registers expected by wain
    const auto arg1 = wain.arguments[0].name;
    const auto arg2 = wain.arguments[1].name;
    if (wain_allocations.in_register(arg1)) {
      copy(wain_allocations.get_register(arg1), Reg::R1);
      annotate("Loading argument 1 into register");
    } else {
      const int offset = wain_allocations.get_offset(arg1);
      sw(Reg::R1, offset, Reg::R29);
      annotate("Loading argument 1 into variable " + arg1);
    }
    if (wain_allocations.in_register(arg2)) {
      copy(wain_allocations.get_register(arg2), Reg::R2);
      annotate("Loading argument 2 into register");
    } else {
      const int offset = wain_allocations.get_offset(arg2);
      sw(Reg::R2, offset, Reg::R29);
      annotate("Loading argument 2 into variable " + arg2);
    }

    if (uses_heap) {
      comment("Calling init");
      const bool first_arg_is_array = wain.arguments[0].type == Type::IntStar;
      if (!first_arg_is_array) {
        copy(Reg::R2, Reg::R0);
      }
      push(Reg::R31);
      load_and_jalr(Reg::R1, "init");
      pop(Reg::R31);
      comment("Done calling init");
    }

    // Set up the stack pointer
    const int stack_size = wain_allocations.spilled_variables.size();
    add_const(Reg::SP, Reg::SP, -4 * stack_size, tmp1);

    // Jump to wain
    beq(Reg::R0, Reg::R0, create_label("wain", wain.entry_label));
    annotate("Done prologue, jumping to wain");

    // Generate code for all the functions
    generate_function(program.wain());
    program.for_each_function([&](const auto &function) {
      if (function.name == "wain")
        return;
      generate_function(function);
    });

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
    label(create_label(function.name, function.entry_label));
    for (const auto &label : function.block_labels) {
      const auto block = function.get_block(label);
      const auto &live_variables =
          allocation.liveness_data.at(label).live_variables;
      for (size_t i = 0; i < block.instructions.size(); ++i) {
        const auto &instruction = block.instructions[i];
        generate_instruction(function.name, instruction, live_variables[i],
                             live_variables[i + 1], allocation);
      }
    }
    comment("Done with function " + function.name);
  }

  Reg load_variable(const Reg temp_reg, const std::string &argument,
                    const RegisterAllocation &allocation) {
    if (allocation.in_register(argument)) {
      return allocation.get_register(argument);
    } else {
      debug_assert(allocation.is_spilled(argument),
                   "Variable {} is not in a register nor on the stack",
                   argument);
      const int offset = allocation.get_offset(argument);
      lw(temp_reg, offset, Reg::R29);
      annotate("Loading variable " + argument + " from offset " +
               std::to_string(offset));
      return temp_reg;
    }
  }
  void store_variable(const std::string &variable, const Reg temp_reg,
                      const RegisterAllocation &allocation) {
    if (allocation.is_spilled(variable)) {
      const int offset = allocation.get_offset(variable);
      sw(temp_reg, offset, Reg::R29);
      annotate("Storing variable " + variable + " to offset " +
               std::to_string(offset));
    }
  }

  inline Reg get_register(const Reg temp_reg, const std::string &variable,
                          const RegisterAllocation &allocation) const {
    return allocation.in_register(variable) ? allocation.get_register(variable)
                                            : temp_reg;
  }

  void generate_instruction(
      const std::string &function_name, const Instruction &instruction,
      [[maybe_unused]] const std::set<std::string> &live_variables_before,
      const std::set<std::string> &live_variables_after,
      const RegisterAllocation &allocation) {
    const std::string &dest = instruction.destination;

    switch (instruction.opcode) {
    case Opcode::Add: {
      const Reg lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp1, dest, allocation);
      add(dest_reg, lhs_reg, rhs_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Sub: {
      const Reg lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp1, dest, allocation);
      sub(dest_reg, lhs_reg, rhs_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Mul: {
      const Reg lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp1, dest, allocation);
      mult(dest_reg, lhs_reg, rhs_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Div: {
      const Reg lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp1, dest, allocation);
      div(dest_reg, lhs_reg, rhs_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Mod: {
      const Reg lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp1, dest, allocation);
      div(lhs_reg, rhs_reg);
      mfhi(dest_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Lt: {
      const Reg lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp3, dest, allocation);
      slt(dest_reg, lhs_reg, rhs_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Le: {
      const Reg lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp3, dest, allocation);
      slt(dest_reg, rhs_reg, lhs_reg);
      sub(dest_reg, Reg::R11, dest_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Gt: {
      const Reg lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp3, dest, allocation);
      slt(dest_reg, rhs_reg, lhs_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Ge: {
      const Reg lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp3, dest, allocation);
      slt(dest_reg, lhs_reg, rhs_reg);
      sub(dest_reg, Reg::R11, dest_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Eq: {
      const Reg lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp3, dest, allocation);
      slt(tmp3, lhs_reg, rhs_reg);
      slt(tmp4, rhs_reg, lhs_reg);
      add(dest_reg, tmp3, tmp4);
      sub(dest_reg, Reg::R11, dest_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Ne: {
      const Reg lhs_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg rhs_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp3, dest, allocation);
      slt(tmp3, lhs_reg, rhs_reg);
      slt(tmp4, rhs_reg, lhs_reg);
      add(dest_reg, tmp3, tmp4);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Jmp: {
      const std::string &label = instruction.labels[0];
      beq(Reg::R0, Reg::R0, create_label(function_name, label));
      annotate(instruction.to_string());
    } break;

    case Opcode::Br: {
      const Reg condition_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const std::string &true_label = instruction.labels[0];
      const std::string &false_label = instruction.labels[1];
      beq(condition_reg, Reg::R0, create_label(function_name, false_label));
      annotate(instruction.to_string());
      beq(Reg::R0, Reg::R0, create_label(function_name, true_label));
    } break;

    case Opcode::Call: {
      const std::string label = instruction.funcs[0];
      const auto &called_function = program.get_function(label);
      const auto &function_allocation = allocations.at(called_function.name);

      // 1. Save the live registers, including $29
      std::set<Reg> live_registers = {Reg::R29};
      for (const auto &var : live_variables_after) {
        if (var != dest && allocation.in_register(var))
          live_registers.insert(allocation.get_register(var));
      }

      comment("Generating function call: " + instruction.to_string());
      comment("1. Save the live registers");
      for (const Reg reg : live_registers)
        push(reg);

      // 2. Subtract 4 from $30 to obtain the new base pointer
      sub(Reg::R30, Reg::R30, Reg::R4);
      annotate("2. Obtain the new base pointer");

      // 3. Copy arguments to the new stack frame
      const size_t num_arguments = called_function.arguments.size();
      std::vector<VariableLocation> source_locations;
      std::vector<VariableLocation> target_locations;
      for (size_t i = 0; i < num_arguments; ++i) {
        const auto &parameter = instruction.arguments[i];
        const auto &argument = called_function.arguments[i].name;
        source_locations.push_back(allocation.get_location(parameter));
        target_locations.push_back(function_allocation.get_location(argument));
      }

      copy_arguments(source_locations, target_locations);

      const size_t stack_size =
          function_allocation.spilled_variables.size() * 4;
      copy(Reg::R29, Reg::R30);
      add_const(Reg::R30, Reg::R30, -stack_size * 4 + 4, tmp1);
      comment("3. Done copying arguments to " + called_function.name);

      // 4. Jump to the function
      push(Reg::R31);
      load_and_jalr(Reg::R2, create_label(called_function.name,
                                          called_function.entry_label));
      annotate("4. Jump to " + called_function.name);
      pop(Reg::R31);

      // 5. Restore the stack pointer
      comment("5. Restore the stack pointer");
      add_const(Reg::R30, Reg::R30, stack_size * 4, tmp1);

      // 6. Pop the saved registers off the stack
      comment("6. Pop the saved registers off the stack");
      for (auto it = live_registers.rbegin(); it != live_registers.rend(); ++it)
        pop(*it);
      comment("7. Done with function call");

      const Reg dest_reg = get_register(tmp1, dest, allocation);
      copy(dest_reg, Reg::R3);
      store_variable(dest, dest_reg, allocation);
      comment("8. Copy return value to " + dest);
    } break;

    case Opcode::Ret: {
      const Reg return_value_reg =
          load_variable(Reg::R3, instruction.arguments[0], allocation);
      copy(Reg::R3, return_value_reg);
      comment(instruction.to_string());
      jr(Reg::R31);
    } break;

    case Opcode::Const: {
      const Reg dest_reg = get_register(tmp1, dest, allocation);
      load_const(dest_reg, instruction.value);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::Id: {
      const Reg src_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg dest_reg = get_register(tmp1, dest, allocation);
      copy(dest_reg, src_reg);
      store_variable(dest, dest_reg, allocation);
      comment(instruction.to_string());
    } break;

    case Opcode::Print: {
      const Reg arg_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      copy(tmp1, arg_reg);
      push(Reg::R31);
      load_and_jalr(tmp2, "print");
      pop(Reg::R31);
      annotate(instruction.to_string());
    } break;

    case Opcode::Nop: {
      // Do nothing
    } break;

    case Opcode::Alloc: {
      const Reg arg_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg dest_reg = get_register(Reg::R3, dest, allocation);
      const std::string alloc_success = generate_label("allocSuccess");

      // Argument in $1
      // Result in $3: if it was 0, return NULL (1)
      copy(tmp1, arg_reg);
      push(Reg::R3);
      push(Reg::R31);
      load_and_jalr(tmp2, "new");
      pop(Reg::R31);
      bne(Reg::R3, Reg::R0, alloc_success);
      add(Reg::R3, Reg::R11, Reg::R0);
      label(alloc_success);
      copy(dest_reg, Reg::R3);
      if (dest_reg != Reg::R3) {
        pop(Reg::R3);
      } else {
        pop_and_discard();
      }
      store_variable(dest, dest_reg, allocation);
    } break;

    case Opcode::Free: {
      const auto skip_label = generate_label("deleteSkip");
      const Reg arg_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      copy(Reg::R1, arg_reg);
      beq(Reg::R1, Reg::R11, skip_label);
      push(Reg::R31);
      load_and_jalr(Reg::R2, "delete");
      pop(Reg::R31);
      label(skip_label);
    } break;

    case Opcode::Store: {
      // store location value
      const Reg dest_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg src_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      sw(src_reg, 0, dest_reg);
      annotate(instruction.to_string());
    } break;

    case Opcode::Load: {
      // dest = load location
      const Reg src_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg dest_reg = get_register(tmp2, dest, allocation);
      lw(dest_reg, 0, src_reg);
      store_variable(dest, dest_reg, allocation);
      annotate(instruction.to_string());
    } break;

    case Opcode::PointerAdd: {
      // dest = ptradd ptr offset
      // -> dest = ptr + 4 * offset
      const Reg ptr_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg offset_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp4, dest, allocation);
      mult(tmp3, offset_reg, Reg::R4);
      add(dest_reg, ptr_reg, tmp3);
      store_variable(dest, dest_reg, allocation);
    } break;

    case Opcode::PointerSub: {
      const Reg ptr_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg offset_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp4, dest, allocation);
      mult(tmp3, offset_reg, Reg::R4);
      sub(dest_reg, ptr_reg, tmp3);
      store_variable(dest, dest_reg, allocation);
    } break;

    case Opcode::PointerDiff: {
      const Reg ptr1_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const Reg ptr2_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const Reg dest_reg = get_register(tmp1, dest, allocation);
      sub(dest_reg, ptr1_reg, ptr2_reg);
      div(dest_reg, dest_reg, Reg::R4);
      store_variable(dest, dest_reg, allocation);
    } break;

    case Opcode::AddressOf: {
      const std::string &var = instruction.arguments[0];
      const Reg dest_reg = get_register(tmp1, dest, allocation);
      debug_assert(allocation.is_spilled(var),
                   "Addressed variable {} is not in memory", var);
      const int offset = allocation.get_offset(var);
      add_const(dest_reg, Reg::R29, offset, tmp1);
      store_variable(dest, dest_reg, allocation);
    } break;

    case Opcode::Label: {
      label(create_label(function_name, instruction.labels[0]));
    } break;

    default: {
      unreachable("Unsupported instruction " + instruction.to_string());
    } break;
    }
  }
};

} // namespace bril
