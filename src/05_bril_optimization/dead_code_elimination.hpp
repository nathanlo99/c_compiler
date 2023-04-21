
#pragma once

#include "bril.hpp"
#include "util.hpp"

namespace bril {

inline size_t remove_global_unused_assignments(ControlFlowGraph &graph) {
  // For any given assignment 'var = value'
  // - If we never use 'var' anywhere else in the function, remove the
  //   assignment
  size_t num_removed_lines = 0;
  std::set<std::string> used_variables;
  std::set<std::string> addressed_variables;
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

inline size_t remove_local_unused_assignments(ControlFlowGraph &,
                                              Block &block) {
  std::set<size_t> to_delete;
  std::map<std::string, size_t> last_def;
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
  if (block.is_exiting) {
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

inline size_t remove_unused_blocks(ControlFlowGraph &graph) {
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
    std::cerr << "Removing unused block " << block_label << std::endl;
    graph.remove_block(block_label);
  }
  graph.recompute_graph();
  return result;
}

inline size_t remove_trivial_phi_instructions(ControlFlowGraph &,
                                              Block &block) {
  size_t result = 0;
  for (auto &instruction : block.instructions) {
    if (instruction.opcode != Opcode::Phi)
      continue;
    if (instruction.arguments.size() != 1)
      continue;
    instruction = Instruction::id(instruction.destination,
                                  instruction.arguments[0], instruction.type);
    result += 1;
  }
  return result;
}

inline size_t combine_extended_blocks(ControlFlowGraph &graph) {
  size_t result = 0;

  // First, identify the edges which can be contracted: (b1, b2) such that
  //   - b2 is the only successor of b2
  //   - b1 is the only predecessor of b2

  while (true) {
    bool changed = false;
    for (const auto &[block_label, block] : graph.blocks) {
      if (block.outgoing_blocks.size() != 1)
        continue;
      const auto &outgoing_block = *block.outgoing_blocks.begin();
      if (graph.get_block(outgoing_block).incoming_blocks.size() != 1)
        continue;

      graph.combine_blocks(block_label, outgoing_block);
      changed = true;
      result++;
      break;
    }
    if (!changed)
      break;
  }

  return result;
}

inline size_t move_constants_to_front(ControlFlowGraph &graph) {
  if (!graph.is_in_ssa_form())
    return 0;

  // First, count the number of constants which are already at the front
  size_t num_constants = 0;
  for (const auto &instruction :
       graph.get_block(graph.entry_label).instructions) {
    if (instruction.opcode == Opcode::Const)
      num_constants++;
    else
      break;
  }

  std::vector<Instruction> constant_instructions;
  for (const auto &label : graph.block_labels) {
    auto &block = graph.get_block(label);
    for (size_t i = 0; i < block.instructions.size(); ++i) {
      const auto &instruction = block.instructions[i];
      if (instruction.opcode == Opcode::Const) {
        constant_instructions.push_back(instruction);
        block.instructions.erase(block.instructions.begin() + i);
        i--;
      }
    }
  }

  auto &entry_instructions = graph.get_block(graph.entry_label).instructions;
  for (size_t i = 0; i < constant_instructions.size(); ++i) {
    const auto &instruction = constant_instructions[i];
    entry_instructions.insert(entry_instructions.begin() + i, instruction);
  }
  return constant_instructions.size() > num_constants ? 1 : 0;
}

inline size_t remove_unused_parameters(Program &program) {
  const std::string wain_name = program.wain().name;
  size_t result = 0;

  // First, identify the unused variables in each function
  std::map<std::string, std::vector<size_t>> unused_parameter_indices;
  for (auto &[function_name, function] : program.cfgs) {
    // Ignore unused parameters in wain
    if (function.name == wain_name)
      continue;

    std::map<std::string, size_t> unused_parameters;
    for (size_t idx = 0; idx < function.arguments.size(); ++idx) {
      const auto &param = function.arguments[idx];
      unused_parameters[param.name] = idx;
    }

    for (const auto &[block_label, block] : function.blocks) {
      for (const auto &instruction : block.instructions) {
        for (const auto &arg : instruction.arguments) {
          unused_parameters.erase(arg);
        }
        unused_parameters.erase(instruction.destination);
      }
    }

    auto &indices = unused_parameter_indices[function_name];
    for (const auto &[param_name, param_idx] : unused_parameters) {
      indices.push_back(param_idx);
    }
    std::sort(indices.begin(), indices.end());
  }

  // Now, remove the unused parameters
  for (auto &[function_name, function] : program.cfgs) {
    auto &indices = unused_parameter_indices[function_name];
    for (auto rit = indices.rbegin(); rit != indices.rend(); ++rit) {
      const auto idx = *rit;
      function.arguments.erase(function.arguments.begin() + idx);
      result++;
    }

    // Now remove the unused parameters from function calls
    for (const auto &label : function.block_labels) {
      auto &block = function.get_block(label);
      for (auto &instruction : block.instructions) {
        if (instruction.opcode != Opcode::Call)
          continue;
        const std::string called_function = instruction.funcs[0];
        const auto &unused_indices =
            unused_parameter_indices[called_function.substr(1)];
        for (auto rit = unused_indices.rbegin(); rit != unused_indices.rend();
             ++rit) {
          const auto idx = *rit;
          instruction.arguments.erase(instruction.arguments.begin() + idx);
          result++;
        }
      }
    }
  }

  return result;
}

} // namespace bril
