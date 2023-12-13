
#include "dead_code_elimination.hpp"

namespace bril {

size_t remove_global_unused_assignments(ControlFlowGraph &graph) {
  // For any given assignment 'var = value'
  // - If we never use 'var' anywhere else in the function, remove the
  //   assignment
  size_t num_removed_lines = 0;
  std::unordered_set<std::string> used_variables;
  std::unordered_set<std::string> addressed_variables;
  for (const auto &[block_label, block] : graph.blocks) {
    for (const auto &instruction : block.instructions) {
      for (const auto &argument : instruction.arguments) {
        used_variables.insert(argument);
      }
      if (instruction.opcode == Opcode::AddressOf ||
          instruction.opcode == Opcode::Load) {
        for (const auto &argument : instruction.arguments) {
          addressed_variables.insert(argument);
        }
      }
    }
  }

  // TODO: Figure out what to do if a memory access / write happens
  for (auto &[block_label, block] : graph.blocks) {
    for (size_t idx = 0; idx < block.instructions.size(); idx++) {
      const auto &instruction = block.instructions[idx];
      const std::string destination = instruction.destination;
      if (destination != "" && used_variables.count(destination) == 0 &&
          addressed_variables.count(destination) == 0 &&
          instruction.is_pure()) {
        block.instructions.erase(block.instructions.begin() + idx);
        idx--;
        num_removed_lines += 1;
      }
    }
  }

  return num_removed_lines;
}

size_t remove_local_unused_assignments(ControlFlowGraph &graph, Block &block) {
  std::set<size_t> to_delete;
  std::unordered_map<std::string, size_t> last_def;
  for (size_t idx = 0; idx < block.instructions.size(); ++idx) {
    const auto &instruction = block.instructions[idx];
    const std::string destination = instruction.destination;

    // Check for uses
    for (const auto &arg : instruction.arguments) {
      last_def.erase(arg);
    }

    // Check for memory accesses, pessimistically assume this destroys all
    // invariants
    if (instruction.uses_memory()) {
      last_def.clear();
    }

    // Check for definitions
    if (destination != "") {
      if (last_def.count(destination) > 0) {
        to_delete.insert(last_def.at(destination));
      }
      // Add pure instructions to the last def map
      if (instruction.is_pure()) {
        last_def[destination] = idx;
      }
    }
  }

  // Any definitions unused by the end of an exiting block can also be deleted
  if (graph.exiting_blocks.count(block.entry_label) > 0) {
    for (const auto &[variable, def] : last_def) {
      to_delete.insert(def);
    }
  }

  // Loop over the indices to delete in reverse order to keep indices valid
  size_t num_removed_lines = 0;
  for (auto rit = to_delete.rbegin(); rit != to_delete.rend(); ++rit) {
    const auto idx = *rit;
    block.instructions.erase(block.instructions.begin() + idx);
    num_removed_lines += 1;
  }
  return num_removed_lines;
}

size_t remove_unused_blocks(ControlFlowGraph &graph) {
  size_t result = 0;
  std::set<std::string> blocks_to_remove;
  for (auto &[block_label, block] : graph.blocks) {
    if (block_label == graph.entry_label)
      continue;
    if (block.incoming_blocks.empty()) {
      blocks_to_remove.insert(block_label);
      result += block.instructions.size();
    }
  }
  for (const auto &block_label : blocks_to_remove) {
    // std::cerr << "Removing unused block " << block_label << std::endl;
    graph.remove_block(block_label);
  }
  graph.recompute_graph();
  return result;
}

size_t remove_unused_functions(Program &program) {
  size_t removed_lines = 0;
  std::unordered_set<std::string> reachable_functions;
  std::vector<std::string> worklist = {"wain"};
  while (!worklist.empty()) {
    const std::string function_name = worklist.back();
    worklist.pop_back();
    if (reachable_functions.count(function_name) > 0)
      continue;
    reachable_functions.insert(function_name);
    const auto &function = program.functions.at(function_name);
    function.for_each_instruction([&](const Instruction &instruction) {
      if (instruction.opcode == Opcode::Call) {
        worklist.push_back(instruction.funcs[0]);
      }
    });
  }

  std::unordered_set<std::string> unused_functions;
  program.for_each_function([&](const ControlFlowGraph &function) {
    if (reachable_functions.count(function.name) == 0) {
      removed_lines += function.num_instructions();
      unused_functions.insert(function.name);
    }
  });

  for (const auto &name : unused_functions) {
    program.functions.erase(name);
  }
  return removed_lines;
}

size_t remove_trivial_phi_instructions(ControlFlowGraph &, Block &block) {
  size_t result = 0;
  for (auto &instruction : block.instructions) {
    if (instruction.opcode != Opcode::Phi)
      continue;
    std::vector<std::string> new_arguments, new_labels;
    std::unordered_set<std::string> arguments;
    for (size_t i = 0; i < instruction.arguments.size(); ++i) {
      const std::string &argument = instruction.arguments[i];
      const std::string &label = instruction.labels[i];
      if (block.incoming_blocks.count(instruction.labels[i]) == 0)
        continue;
      new_arguments.push_back(argument);
      new_labels.push_back(label);
      arguments.insert(instruction.arguments[i]);
    }
    if (arguments.size() == 1) {
      instruction = Instruction::id(instruction.destination,
                                    instruction.arguments[0], instruction.type);
      result += 1;
    } else {
      instruction.arguments = new_arguments;
      instruction.labels = new_labels;
    }
  }
  return result;
}

size_t combine_extended_blocks(ControlFlowGraph &function) {
  size_t result = 0;

  // First, identify the edges which can be contracted: (b1, b2) such that
  //   - b2 is the only successor of b2
  //   - b1 is the only predecessor of b2

  while (true) {
    bool changed = false;
    for (const auto &[block_label, block] : function.blocks) {
      if (block.outgoing_blocks.size() != 1)
        continue;
      const auto &outgoing_block = *block.outgoing_blocks.begin();
      if (function.get_block(outgoing_block).incoming_blocks.size() != 1)
        continue;

      function.combine_blocks(block_label, outgoing_block);
      changed = true;
      result++;
      break;
    }
    if (!changed)
      break;
  }

  return result;
}

size_t remove_unused_parameters(Program &program) {
  const std::string wain_name = "wain";
  size_t result = 0;

  // First, identify the unused variables in each function
  std::unordered_map<std::string, std::vector<size_t>> unused_parameter_indices;
  for (auto &[function_name, function] : program.functions) {
    // Ignore unused parameters in wain
    if (function.name == wain_name)
      continue;

    std::unordered_set<std::string> used_parameters;

    // If the variable is used anywhere as an argument, it becomes used
    function.for_each_instruction([&](const Instruction &instruction) {
      for (const auto &argument : instruction.arguments)
        used_parameters.insert(argument);
      // TODO: If the parameter is written to, should we still consider it used?
      // used_parameters.insert(instruction.destination);
    });

    auto &indices = unused_parameter_indices[function_name];
    for (size_t idx = 0; idx < function.arguments.size(); ++idx) {
      if (used_parameters.count(function.arguments[idx].name) == 0)
        indices.push_back(idx);
    }
  }

  // Now, remove the unused parameters
  for (auto &[function_name, indices] : unused_parameter_indices) {
    auto &function = program.functions.at(function_name);
    for (auto rit = indices.rbegin(); rit != indices.rend(); ++rit) {
      const auto idx = *rit;
      function.arguments.erase(function.arguments.begin() + idx);
      result++;
    }
  }

  // Now remove the unused parameters from function calls
  for (auto &[function_name, function] : program.functions) {
    function.for_each_instruction([&](Instruction &instruction) {
      if (instruction.opcode != Opcode::Call)
        return;
      const std::string called_function = instruction.funcs[0];
      const auto &unused_indices = unused_parameter_indices[called_function];
      for (auto rit = unused_indices.rbegin(); rit != unused_indices.rend();
           ++rit) {
        const auto idx = *rit;
        instruction.arguments.erase(instruction.arguments.begin() + idx);
        result++;
      }
    });
  }

  return result;
}

} // namespace bril
