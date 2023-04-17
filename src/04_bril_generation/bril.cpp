
#include "bril.hpp"

namespace bril {

ControlFlowGraph::ControlFlowGraph(const Function &function)
    : name(function.name), arguments(function.arguments),
      return_type(function.return_type) {

  std::map<std::string, std::string> canonical_label_name;

  // Loop over the instructions and create blocks:
  // - Labels start new blocks, and fallthrough from the previous block
  // - Jumps end blocks
  Block current_block;
  entry_label = "." + function.name.substr(1) + "_entry";
  current_block.entry_label = entry_label;
  std::map<std::string, std::set<std::string>> entry_labels;
  for (const auto &instruction : function.instructions) {
    if (instruction.opcode == Opcode::Label) {
      const auto &label = instruction.labels[0];
      if (current_block.has_instructions()) {
        // Fallthrough from previous block
        current_block.instructions.push_back(Instruction::jmp(label));
        current_block.exit_labels = {label};
        add_block(current_block);
        current_block = Block();
      }
      if (current_block.entry_label == "") {
        current_block.entry_label = label;
      }
      canonical_label_name[label] = current_block.entry_label;
      entry_labels[current_block.entry_label].insert(label);
      current_block.instructions.push_back(instruction);
    } else if (instruction.is_jump()) {
      current_block.instructions.push_back(instruction);
      current_block.exit_labels = instruction.labels;
      if (instruction.opcode == Opcode::Ret) {
        exiting_blocks.insert(current_block.entry_label);
        current_block.is_exiting = true;
      }
      add_block(current_block);
      current_block = Block();
    } else {
      current_block.instructions.push_back(instruction);
    }
  }
  add_block(current_block);

  // Remove redundant labels
  for (auto &[label, block] : blocks) {
    for (auto &exit_label : block.exit_labels) {
      exit_label = canonical_label_name[exit_label];
    }

    // Remove redundant labels
    while (block.instructions.size() > 1 &&
           block.instructions[1].opcode == Opcode::Label)
      block.instructions.erase(block.instructions.begin() + 1);

    for (auto &instruction : block.instructions) {
      for (auto &label : instruction.labels) {
        label = canonical_label_name[label];
      }
    }
  }

  for (const auto &[label, block] : blocks) {
    for (const auto &exit_label : block.exit_labels) {
      runtime_assert(blocks.count(exit_label) > 0,
                     "Exit label " + exit_label + " not found in label map");
      add_edge(label, exit_label);
    }
  }

  // Ensure that the entry block has no predecessors, since this breaks SSA
  // conversion later
  if (blocks.at(entry_label).incoming_blocks.size() > 0) {
    const auto old_entry_label = entry_label;
    const auto new_entry_label = "." + function.name.substr(1) + "_entry2";
    entry_label = new_entry_label;

    Block new_block;
    block_labels.insert(block_labels.begin(), new_entry_label);
    new_block.entry_label = new_entry_label;
    new_block.instructions.push_back(Instruction::label(new_entry_label));
    new_block.instructions.push_back(Instruction::jmp(old_entry_label));
    new_block.exit_labels = {old_entry_label};
    blocks.emplace(new_entry_label, new_block);

    add_edge(new_entry_label, old_entry_label);
  }

  compute_dominators();
}

void ControlFlowGraph::compute_edges() {
  for (auto &[label, block] : blocks) {
    block.incoming_blocks.clear();
    block.outgoing_blocks.clear();
  }
  for (auto &[label, block] : blocks) {
    for (const auto &instruction : block.instructions) {
      if (instruction.is_jump()) {
        for (const auto &exit_label : instruction.labels) {
          add_edge(label, exit_label);
        }
      }
    }
  }
}

void ControlFlowGraph::add_edge(const std::string &source,
                                const std::string &target) {
  get_block(source).outgoing_blocks.insert(target);
  get_block(target).incoming_blocks.insert(source);
  is_graph_dirty = true;
}

void ControlFlowGraph::remove_edge(const std::string &source,
                                   const std::string &target) {
  runtime_assert(blocks.count(source) > 0, "No block with label " + source);
  runtime_assert(blocks.count(target) > 0, "No block with label " + target);
  runtime_assert(get_block(source).outgoing_blocks.count(target) > 0,
                 "No edge between '" + source + "' and '" + target + "'");
  runtime_assert(get_block(target).incoming_blocks.count(source) > 0,
                 "No edge between '" + source + "' and '" + target + "'");

  get_block(source).outgoing_blocks.erase(target);
  get_block(target).incoming_blocks.erase(source);
  is_graph_dirty = true;
}

void ControlFlowGraph::add_block(Block block) {
  if (block.instructions.empty())
    return;
  block_labels.push_back(block.entry_label);
  blocks.emplace(block.entry_label, block);
}

void ControlFlowGraph::remove_block(const std::string &block_label) {
  std::cerr << "Removing block " << block_label << std::endl;
  runtime_assert(blocks.count(block_label) > 0,
                 "No block with label " + block_label);

  // If there are any jumps with this block as a target, throw an exception
  for (const auto &[label, block] : blocks) {
    for (const auto &instruction : block.instructions) {
      if (!instruction.is_jump())
        continue;

      for (const auto &exit_label : instruction.labels) {
        if (exit_label == block_label) {
          throw std::runtime_error(
              "Cannot remove block " + block_label +
              " because it is the target of a jump instruction");
        }
      }
    }
  }

  // If there are any phi nodes with this block as a target, remove them
  for (auto &[label, block] : blocks) {
    for (auto &instruction : block.instructions) {
      if (instruction.opcode != Opcode::Phi)
        continue;

      const auto it = std::find(instruction.labels.begin(),
                                instruction.labels.end(), block_label);
      if (it == instruction.labels.end())
        continue;
      const size_t idx = it - instruction.labels.begin();
      instruction.labels.erase(instruction.labels.begin() + idx);
      instruction.arguments.erase(instruction.arguments.begin() + idx);
    }
  }

  // Graph bookkeeping
  const Block block = get_block(block_label);
  runtime_assert(block.incoming_blocks.empty(),
                 "Cannot remove block with incoming edges");
  for (const auto &outgoing_block : block.outgoing_blocks) {
    get_block(outgoing_block).incoming_blocks.erase(block_label);
    is_graph_dirty = true;
  }

  // Remove the actual block from the block list
  blocks.erase(block_label);
  // Remove the block's label from the list of block labels
  block_labels.erase(
      std::find(block_labels.begin(), block_labels.end(), block_label));
  // Remove the block from the set of exiting blocks
  exiting_blocks.erase(block_label);

  is_graph_dirty = true;
}

void ControlFlowGraph::compute_dominators() {
  // Setup relevant sets of labels: all blocks and non-entry blocks
  std::set<std::string> non_entry_blocks;
  for (const auto &label : block_labels) {
    if (label != entry_label)
      non_entry_blocks.insert(label);
  }

  // Initialize the dominator matrix
  for (const auto &label : non_entry_blocks) {
    for (const auto &other_label : block_labels) {
      raw_dominators[label][other_label] = true;
    }
  }
  for (const auto &label : non_entry_blocks) {
    raw_dominators[entry_label][label] = false;
  }
  raw_dominators[entry_label][entry_label] = true;

  while (true) {
    bool changed = false;
    for (const auto &i : non_entry_blocks) {
      const auto old_set = raw_dominators[i];
      for (const std::string &pred : blocks.at(i).incoming_blocks) {
        for (const auto &k : block_labels)
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
  dominators.clear();
  immediate_dominators.clear();
  dominance_frontiers.clear();
  for (const auto &i : block_labels) {
    for (const auto &j : block_labels) {
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
bool ControlFlowGraph::_dominates(const std::string &source,
                                  const std::string &target) const {
  return raw_dominators.at(target).at(source);
}

bool ControlFlowGraph::_strictly_dominates(const std::string &source,
                                           const std::string &target) const {
  return source != target && _dominates(source, target);
}

// 'source' immediately dominates 'target' if 'source' strictly dominates
// 'target', but 'source' does not strictly dominate any other node that
// strictly dominates 'target'
bool ControlFlowGraph::_immediately_dominates(const std::string &source,
                                              const std::string &target) const {
  if (!_dominates(source, target))
    return false;
  for (const auto &[k, _] : blocks) {
    if (_strictly_dominates(source, k) && _strictly_dominates(k, target))
      return false;
  }
  return true;
}

// The domination frontier of 'source' contains 'target' if 'source' does
// NOT dominate 'target' but 'source' dominates a predecessor of 'target'
bool ControlFlowGraph::_is_in_dominance_frontier(
    const std::string &source, const std::string &target) const {
  if (_strictly_dominates(source, target))
    return false;
  for (const std::string &pred : get_block(target).incoming_blocks) {
    if (_dominates(source, pred))
      return true;
  }
  return false;
}

} // namespace bril
