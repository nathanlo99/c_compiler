
#include "bril.hpp"

namespace bril {
ControlFlowGraph::ControlFlowGraph(Function function)
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

  const std::string entry_label = function.name + "_entry";
  function.instructions.insert(function.instructions.begin(),
                               Instruction::jmp(entry_label));
  function.instructions.insert(function.instructions.begin() + 1,
                               Instruction::label(entry_label));

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
  // 1. Add edges

  // First, construct a map from label to block index
  for (size_t idx = 0; idx < blocks.size(); ++idx) {
    auto &block = blocks[idx];
    for (const auto &entry_label : block.entry_labels) {
      label_to_block[entry_label] = idx;
    }
  }

  // Then, for each outgoing label, add an edge
  for (size_t idx = 0; idx < blocks.size(); ++idx) {
    auto &block = blocks[idx];
    for (const auto &exit_label : block.exit_labels) {
      runtime_assert(label_to_block.count(exit_label) > 0,
                     "Exit label " + exit_label + " not found in label map");
      const size_t next_idx = label_to_block.at(exit_label);
      add_directed_edge(idx, next_idx);
    }
  }

  // Then, add edges for all the fallthroughs
  for (size_t idx = 1; idx < blocks.size(); ++idx) {
    if (!blocks[idx - 1].instructions.back().is_jump()) {
      add_directed_edge(idx - 1, idx);
    }
  }

  // 2. Canonicalize the label names

  // Loop through and replace all the labels with block indices
  for (auto &block : blocks) {
    // Remove all the labels
    for (size_t i = 0; i < block.instructions.size(); ++i) {
      if (block.instructions[i].opcode == Opcode::Label) {
        block.instructions.erase(block.instructions.begin() + i);
        --i;
      }
    }

    for (auto &instruction : block.instructions) {
      if (instruction.opcode == Opcode::Label)
        continue;
      for (auto &label : instruction.labels) {
        runtime_assert(label_to_block.count(label) > 0,
                       "Label " + label + " not found in label map");
        label = ".bb_" + std::to_string(label_to_block.at(label));
      }
    }
  }

  for (size_t idx = 1; idx < blocks.size(); ++idx) {
    if (!blocks[idx - 1].instructions.back().is_jump()) {
      blocks[idx - 1].instructions.push_back(
          Instruction::jmp(".bb_" + std::to_string(idx)));
    }
  }
}

void ControlFlowGraph::compute_dominators() {
  const size_t num_blocks = blocks.size();
  const std::vector<bool> everything(num_blocks, true);
  raw_dominators = std::vector<std::vector<bool>>(num_blocks, everything);
  raw_dominators[0] = std::vector<bool>(num_blocks, false);
  raw_dominators[0][0] = true;

  while (true) {
    bool changed = false;
    for (size_t i = 1; i < num_blocks; ++i) {
      const auto old_set = raw_dominators[i];
      raw_dominators[i] = everything;
      for (size_t pred : blocks[i].incoming_blocks) {
        for (size_t k = 0; k < num_blocks; ++k)
          raw_dominators[i][k] =
              raw_dominators[i][k] && raw_dominators[pred][k];
      }
      raw_dominators[i][i] = true;
      changed |= old_set != raw_dominators[i];
    }
    if (!changed)
      break;
  }

  // Set up memoized versions of the dominator queries
  dominators = std::vector<std::set<size_t>>(num_blocks);
  immediate_dominators = std::vector<size_t>(num_blocks);
  dominance_frontiers = std::vector<std::set<size_t>>(num_blocks);
  for (size_t i = 0; i < num_blocks; ++i) {
    for (size_t j = 0; j < num_blocks; ++j) {
      if (_dominates(j, i))
        dominators[j].insert(i);
      if (_immediately_dominates(j, i))
        immediate_dominators[j] = i;
      if (_is_in_dominance_frontier(j, i))
        dominance_frontiers[j].insert(i);
    }
  }
}

// Does every path through 'target' pass through 'source'?
bool ControlFlowGraph::_dominates(const size_t source,
                                  const size_t target) const {
  return raw_dominators[target][source];
}

bool ControlFlowGraph::_strictly_dominates(const size_t source,
                                           const size_t target) const {
  return source != target && _dominates(source, target);
}

// 'source' immediately dominates 'target' if 'source' strictly dominates
// 'target', but 'source' does not strictly dominate any other node that
// strictly dominates 'target'
bool ControlFlowGraph::_immediately_dominates(const size_t source,
                                              const size_t target) const {
  if (!_dominates(source, target))
    return false;
  for (size_t k = 0; k < blocks.size(); ++k) {
    if (_strictly_dominates(source, k) && _strictly_dominates(k, target))
      return false;
  }
  return true;
}

// The domination frontier of 'source' contains 'target' if 'source' does
// NOT dominate 'target' but 'source' dominates a predecessor of 'target'
bool ControlFlowGraph::_is_in_dominance_frontier(const size_t source,
                                                 const size_t target) const {
  if (_strictly_dominates(source, target))
    return false;
  for (const size_t pred : blocks[target].incoming_blocks) {
    if (_dominates(source, pred))
      return true;
  }
  return false;
}

} // namespace bril
