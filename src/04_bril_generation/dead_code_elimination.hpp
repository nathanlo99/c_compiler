
#pragma once

#include "bril.hpp"

namespace bril {

inline size_t remove_global_unused_assignments(ControlFlowGraph &graph) {
  // For any given assignment 'var = value'
  // - If we never use 'var' anywhere else in the function, remove the
  //   assignment
  size_t num_removed_lines = 0;
  std::set<std::string> used_variables;
  std::set<std::string> addressed_variables;
  for (const auto &block : graph.blocks) {
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
  for (auto &block : graph.blocks) {
    for (size_t idx = 0; idx < block.instructions.size(); idx++) {
      const auto &instruction = block.instructions[idx];
      const std::string destination = instruction.destination;
      if (destination != "" && used_variables.count(destination) == 0 &&
          addressed_variables.count(destination) == 0 &&
          instruction.opcode != Opcode::Call) {
        // std::cerr << "Removing globally unused assignment " << instruction
        //           << std::endl;
        block.instructions.erase(block.instructions.begin() + idx);
        idx--;
        num_removed_lines += 1;
      }
    }
  }

  return num_removed_lines;
}

inline size_t remove_local_unused_assignments(Block &block) {
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
      if (instruction.opcode != Opcode::Call) {
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
    // std::cerr << "Removing locally unused assignment "
    //           << block.instructions[idx] << std::endl;
    block.instructions.erase(block.instructions.begin() + idx);
    num_removed_lines += 1;
  }
  return num_removed_lines;
}

} // namespace bril
