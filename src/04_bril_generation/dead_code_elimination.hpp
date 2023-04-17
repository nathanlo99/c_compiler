
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
        std::cerr << "Removing globally unused instruction " << instruction
                  << std::endl;
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
    std::cerr << "Removing locally unused instruction "
              << block.instructions[idx] << std::endl;
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

// Remove non-entry blocks which consist only of a single jump instruction
inline size_t remove_trivial_blocks(ControlFlowGraph &graph) {
  // First, compute the arguments of phi nodes
  std::set<std::string> phi_arguments;
  for (const auto &[block_label, block] : graph.blocks) {
    for (const auto &instruction : block.instructions) {
      if (instruction.opcode != Opcode::Phi)
        continue;
      for (const auto &param : instruction.labels) {
        phi_arguments.insert(param);
      }
    }
  }

  size_t result = 0;
  for (const auto &label : graph.block_labels) {
    const auto &block = graph.get_block(label);
    if (label == graph.entry_label || block.instructions.size() > 2)
      continue;
    const auto jump_instruction = block.instructions[1];
    if (jump_instruction.opcode != Opcode::Jmp)
      continue;
    const auto target = jump_instruction.labels[0];

    // Can't replace infinite loops
    if (target == label)
      continue;

    // If the block is the target of a phi node, and has multiple predecessors,
    // we cannot yet completely remove it
    if (phi_arguments.count(label) > 0 && block.incoming_blocks.size() != 1)
      continue;
    const auto pred = *block.incoming_blocks.begin();

    std::cerr << "Replacing all jumps to " << label << " with jumps to "
              << target << std::endl;

    for (const auto &other_label : graph.block_labels) {
      if (label == other_label)
        continue;
      auto &other_block = graph.get_block(other_label);
      for (auto &instruction : other_block.instructions) {
        auto it = std::find(instruction.labels.begin(),
                            instruction.labels.end(), label);
        if (it == instruction.labels.end())
          continue;
        if (instruction.opcode == Opcode::Phi)
          *it = pred;
        else
          *it = target;
        graph.is_graph_dirty = true;
      }
    }
  }

  graph.recompute_graph();
  return result;
}

inline size_t remove_trivial_phi_instructions(Block &block) {
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

} // namespace bril
