
#include "bril.hpp"

namespace bril {
ControlFlowGraph::ControlFlowGraph(const Function &function)
    : name(function.name), arguments(function.arguments),
      return_type(function.return_type) {
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
  compute_dominators();
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
      runtime_assert(label_to_idx.count(exit_label) > 0,
                     "Exit label " + exit_label + " not found in label map");
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

void ControlFlowGraph::compute_dominators() {
  const size_t num_blocks = blocks.size();
  const std::vector<bool> everything(num_blocks, true);
  dominators = std::vector<std::vector<bool>>(num_blocks, everything);
  dominators[0] = std::vector<bool>(num_blocks, false);
  dominators[0][0] = true;

  while (true) {
    bool changed = false;
    for (size_t i = 1; i < num_blocks; ++i) {
      const auto old_set = dominators[i];
      dominators[i] = everything;
      for (size_t pred : blocks[i].incoming_blocks) {
        for (size_t k = 0; k < num_blocks; ++k)
          dominators[i][k] = dominators[i][k] && dominators[pred][k];
      }
      dominators[i][i] = true;
      changed |= old_set != dominators[i];
    }
    if (!changed)
      break;
  }
}

} // namespace bril
