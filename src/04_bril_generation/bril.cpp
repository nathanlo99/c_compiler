
#include "bril.hpp"

namespace bril {
ControlFlowGraph::ControlFlowGraph(const Function &function)
    : name(function.name) {
  /*
  loop over the instructions in function:
    - labels start new blocks
    - jumps, branches and ret terminate blocks
    - extract starting labels when a block begins
    - extract exiting labels from the last instruction in the block, if it's a
        jump
  */
  Block current_block;
  for (size_t idx = 0; idx < function.instructions.size(); ++idx) {
    const Instruction &instruction = function.instructions[idx];

    if (instruction.opcode == Opcode::Label) {
      if (current_block.instructions.size() >
          current_block.entry_labels.size()) {
        add_block(current_block);
        current_block = Block();
      }
      current_block.entry_labels.push_back(instruction.labels[0]);
      current_block.instructions.push_back(instruction);
    } else if (instruction.is_jump()) {
      // Jump, branch
      current_block.instructions.push_back(instruction);
      current_block.exit_labels = instruction.labels;
      add_block(current_block);
      current_block = Block();
    } else if (instruction.opcode == Opcode::Ret) {
      // Return: add this block's index to the exiting block list
      current_block.instructions.push_back(instruction);
      const size_t current_block_idx = blocks.size();
      exiting_blocks.insert(current_block_idx);
      current_block.is_exiting = true;
      add_block(current_block);
      current_block = Block();
    } else {
      // Normal instruction
      current_block.instructions.push_back(instruction);
    }
  }
  add_block(current_block);

  compute_edges();
}

void ControlFlowGraph::add_directed_edge(const size_t source,
                                         const size_t target) {
  blocks[source].outgoing_blocks.insert(target);
  blocks[target].incoming_blocks.insert(source);
}

void ControlFlowGraph::compute_edges() {
  std::map<std::string, size_t> label_to_idx;
  for (size_t idx = 0; idx < blocks.size(); ++idx) {
    auto &block = blocks[idx];
    for (const auto &entry_label : block.entry_labels) {
      label_to_idx[entry_label] = idx;
    }
  }

  for (size_t idx = 0; idx < blocks.size(); ++idx) {
    auto &block = blocks[idx];
    for (const auto &exit_label : block.exit_labels) {
      const size_t next_idx = label_to_idx.at(exit_label);
      add_directed_edge(idx, next_idx);
    }
  }

  for (size_t idx = 1; idx < blocks.size(); ++idx) {
    if (!blocks[idx - 1].instructions.back().is_jump()) {
      add_directed_edge(idx - 1, idx);
    }
  }
}

} // namespace bril
