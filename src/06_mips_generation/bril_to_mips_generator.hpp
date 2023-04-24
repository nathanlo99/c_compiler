
#pragma once

#include "bril.hpp"
#include "live_analysis.hpp"
#include "mips_generator.hpp"
#include "mips_instruction.hpp"
#include "util.hpp"

namespace bril {

struct BRILToMIPSGenerator : MIPSGenerator {
  static const inline std::vector<size_t> available_registers = {
      3,  5,  8,  9,  10, 12, 13, 14, 15, 16, 17,
      18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28};
  static const size_t tmp1 = 1, tmp2 = 2, tmp3 = 6, tmp4 = 7;

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
    std::map<size_t, size_t> register_graph;
    std::vector<size_t> to_memory;
    std::vector<size_t> from_memory;
    std::set<size_t> sink_nodes;

    for (size_t i = 0; i < num_arguments; ++i) {
      const auto source_location = source_locations[i];
      const auto target_location = target_locations[i];
      if (target_location.in_memory()) {
        to_memory.push_back(i);
      } else if (source_location.in_memory()) {
        from_memory.push_back(i);
      } else {
        runtime_assert(register_graph.count(target_location.reg) == 0,
                       "Register graph has multiple edges");
        register_graph[target_location.reg] = source_location.reg;
        sink_nodes.insert(target_location.reg);
      }
    }
    for (const auto &[source, target] : register_graph) {
      sink_nodes.erase(target);
    }

    // Sink nodes are nodes with no outgoing edges
    using util::operator<<;
    std::cerr << "Graph: " << register_graph << std::endl;
    std::cerr << "Sink nodes: " << sink_nodes << std::endl;

    // 1. Move arguments from registers to memory
    for (size_t i : to_memory) {
      const auto source_location = source_locations[i];
      const auto target_location = target_locations[i];
      if (source_location.in_memory()) {
        lw(tmp1, source_location.offset, 29);
        sw(tmp1, target_location.offset, 30);
        annotate("Copying argument " + std::to_string(i) +
                 " from memory to memory");
      } else {
        sw(source_location.reg, target_location.offset, 30);
        annotate("Copying argument " + std::to_string(i) +
                 " from register to memory");
      }
    }

    // 2. Handle register to register moves
    // 2a. Handle chains: A -> B -> C -> D
    // - D is the sink, so we copy D to C, C to B, and B to A
    for (const auto &sink : sink_nodes) {
      size_t node = sink;
      while (register_graph.count(node) != 0) {
        const size_t next = register_graph[node];
        register_graph.erase(node);
        copy(node, next);
        std::cerr << "$" << node << " <- $" << next << std::endl;
        annotate("Copying argument from register " + std::to_string(node) +
                 " to register " + std::to_string(next));
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
      size_t node = node_next.first, next = node_next.second, start = node;
      register_graph.erase(node);
      if (node == next)
        continue;
      std::cerr << "Beginning cycle: " << std::endl;
      copy(1, node);
      std::cerr << "$1 <- $" << node << std::endl;
      annotate("Copying argument from register " + std::to_string(node) +
               " to register 1");
      while (next != start) {
        node = next;
        next = register_graph[node];
        register_graph.erase(node);
        copy(next, node);
        std::cerr << "$" << next << " <- $" << node << std::endl;
        annotate("Copying argument from register " + std::to_string(node) +
                 " to register " + std::to_string(next));
      }
      copy(node, 1);
      std::cerr << "$" << node << " <- $1" << std::endl;
      annotate("Copying argument from register 1 to register " +
               std::to_string(node));
    }

    // 3. Move arguments from memory to registers
    for (size_t i : from_memory) {
      const auto source_location = source_locations[i];
      const auto target_location = target_locations[i];
      lw(target_location.reg, source_location.offset, 29);
      annotate("Copying argument " + std::to_string(i) +
               " from memory to register");
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
    add_const(30, 30, -4 * stack_size, tmp1);

    // Jump to wain
    beq(0, 0, create_label("wain", wain.entry_label));
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
      const std::string &function_name, const Instruction &instruction,
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
      beq(0, 0, create_label(function_name, label));
      annotate(instruction.to_string());
    } break;

    case Opcode::Br: {
      const size_t condition_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const std::string &true_label = instruction.labels[0];
      const std::string &false_label = instruction.labels[1];
      beq(condition_reg, 0, create_label(function_name, false_label));
      annotate(instruction.to_string());
      beq(0, 0, create_label(function_name, true_label));
    } break;

    case Opcode::Call: {
      const std::string label = instruction.funcs[0];
      const auto &called_function = program.get_function(label);
      const auto &function_allocation = allocations.at(called_function.name);

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
      // TODO: When we need to write to a register which we also need to read
      // from, we have to save the target register's value to the stack first

      const size_t num_arguments = called_function.arguments.size();
      std::vector<VariableLocation> source_locations;
      std::vector<VariableLocation> target_locations;
      for (size_t i = 0; i < num_arguments; ++i) {
        const auto &parameter = instruction.arguments[i];
        const auto &argument = called_function.arguments[i].name;
        source_locations.push_back(allocation.get_location(parameter));
        target_locations.push_back(function_allocation.get_location(argument));
        std::cerr << "Copying argument " << parameter << " at "
                  << source_locations.back() << " to " << argument << " at "
                  << target_locations.back() << std::endl;
      }

      copy_arguments(source_locations, target_locations);

      const size_t stack_size =
          function_allocation.spilled_variables.size() * 4;
      add(29, 30, 0);
      add_const(30, 30, -stack_size * 4 + 4, tmp1);
      comment("3. Done copying arguments to " + called_function.name);

      // 4. Jump to the function
      push(31);
      load_and_jalr(
          2, create_label(called_function.name, called_function.entry_label));
      annotate("4. Jump to " + called_function.name);
      pop(31);

      // 5. Restore the stack pointer
      comment("5. Restore the stack pointer");
      add_const(30, 30, stack_size * 4, tmp1);

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
      const size_t dest_reg = get_register(tmp4, dest, allocation);
      mult(tmp3, offset_reg, 4);
      add(dest_reg, ptr_reg, tmp3);
      store_variable(dest, dest_reg, allocation);
    } break;

    case Opcode::PointerSub: {
      const size_t ptr_reg =
          load_variable(tmp1, instruction.arguments[0], allocation);
      const size_t offset_reg =
          load_variable(tmp2, instruction.arguments[1], allocation);
      const size_t dest_reg = get_register(tmp4, dest, allocation);
      mult(tmp3, offset_reg, 4);
      sub(dest_reg, ptr_reg, tmp3);
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
      add_const(dest_reg, 29, offset, tmp1);
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
