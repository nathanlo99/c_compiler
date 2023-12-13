
#include "bril.hpp"
#include "timer.hpp"
#include "util.hpp"

namespace bril {

ControlFlowGraph::ControlFlowGraph(const Function &function)
    : name(function.name), arguments(function.arguments),
      return_type(function.return_type) {

  ScopedTimer timer("CFG: " + function.name);
  std::unordered_map<std::string, std::string> canonical_label_name;

  // Loop over the instructions and create blocks:
  // - Labels start new blocks, and fallthrough from the previous block
  // - Jumps end blocks
  Block current_block;
  entry_label = function.name + "Entry";
  current_block.entry_label = entry_label;
  std::unordered_map<std::string, std::unordered_set<std::string>> entry_labels;
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
      }
      // If we see a jump, but the block has no entry label, then there's no way
      // of entering this block: it's dead anyway, so throw it away
      //
      // This happens, for example, when we return out at the end of an if
      // block: the jump corresponding to the return statement immediately
      // precedes the jump out of the if block, so the second jump is never
      // executed
      if (current_block.entry_label != "") {
        add_block(current_block);
      }
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
  for_each_block([&](const Block &block) {
    for (const auto &exit_label : block.exit_labels) {
      debug_assert(blocks.count(exit_label) > 0,
                   "Exit label {} not found in label map", exit_label);
      add_edge(block.entry_label, exit_label);
    }
  });

  // Ensure that the entry block has no predecessors, since this breaks SSA
  // conversion later
  if (blocks.at(entry_label).incoming_blocks.size() > 0) {
    const auto old_entry_label = entry_label;
    const auto new_entry_label = get_fresh_label(old_entry_label);
    entry_label = new_entry_label;

    Block new_block;
    block_labels.insert(block_labels.begin(), new_entry_label);
    new_block.entry_label = new_entry_label;
    // new_block.instructions.push_back(Instruction::label(new_entry_label));
    new_block.instructions.push_back(Instruction::jmp(old_entry_label));
    blocks.emplace(new_entry_label, new_block);

    add_edge(new_entry_label, old_entry_label);
  }

  compute_dominators();
}

void ControlFlowGraph::compute_edges() {
  exiting_blocks.clear();
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
      if (instruction.opcode == Opcode::Ret)
        exiting_blocks.insert(label);
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
  debug_assert(blocks.count(source) > 0, "No block with label {}", source);
  debug_assert(blocks.count(target) > 0, "No block with label {}", target);
  debug_assert(get_block(source).outgoing_blocks.count(target) > 0,
               "No edge between '{}' and '{}'", source, target);
  debug_assert(get_block(target).incoming_blocks.count(source) > 0,
               "No edge between '{}' and '{}'", source, target);

  get_block(source).outgoing_blocks.erase(target);
  get_block(target).incoming_blocks.erase(source);
  is_graph_dirty = true;
}

void ControlFlowGraph::add_block(const Block &block) {
  if (block.instructions.empty())
    return;
  block_labels.push_back(block.entry_label);
  blocks.emplace(block.entry_label, block);
}

void ControlFlowGraph::remove_block(const std::string &block_label) {
  // std::cerr << "Removing block " << block_label << std::endl;
  debug_assert(blocks.count(block_label) > 0, "No block with label {}",
               block_label);

  // If there are any jumps with this block as a target, throw an exception
  for_each_instruction([&](const Instruction &instruction) {
    if (!instruction.is_jump())
      return;
    for (const auto &exit_label : instruction.labels) {
      debug_assert(exit_label != block_label,
                   "Cannot remove block {} because it is the target of a jump "
                   "instruction",
                   block_label);
    }
  });

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
  debug_assert(block.incoming_blocks.empty(),
               "Cannot remove block with incoming edges");
  for (const auto &outgoing_block : block.outgoing_blocks) {
    get_block(outgoing_block).incoming_blocks.erase(block_label);
  }

  // Remove the actual block from the block list
  blocks.erase(block_label);
  // Remove the block's label from the list of block labels
  block_labels.erase(
      std::find(block_labels.begin(), block_labels.end(), block_label));
  // Remove the block from the set of exiting blocks
  exiting_blocks.erase(block_label);

  recompute_graph(true);
}

void ControlFlowGraph::combine_blocks(const std::string &source,
                                      const std::string &target) {
  std::cerr << "Combining blocks " << source << " and " << target << std::endl;
  debug_assert(blocks.count(source) > 0, "No block with label {}", source);
  debug_assert(blocks.count(target) > 0, "No block with label {}", target);
  auto &source_block = get_block(source);
  auto &target_block = get_block(target);
  debug_assert(source_block.outgoing_blocks.count(target) > 0,
               "No edge between '{}' and '{}'", source, target);
  debug_assert(target_block.incoming_blocks.count(source) > 0,
               "No edge between '{}' and '{}'", source, target);
  debug_assert(source_block.outgoing_blocks.size() == 1,
               "Source block has multiple exit labels");
  debug_assert(target_block.incoming_blocks.size() == 1,
               "Target block has multiple incoming blocks");

  // Make sure the last instruction in the source block is a jump to target
  const auto last_instruction = source_block.instructions.back();
  debug_assert(last_instruction.is_jump(),
               "Last instruction in source block is not a jump");
  debug_assert(last_instruction.labels[0] == target,
               "Jump in source block does not target target block");

  // We want to keep the source block, so we remove the last instruction (which
  // must be a jump), and add the instructions from the target block
  source_block.instructions.pop_back();
  for (const auto &instruction : target_block.instructions) {
    if (instruction.opcode == Opcode::Label) {
      continue;
    } else if (instruction.opcode == Opcode::Phi) {
      // If we encounter a phi node in the target block, it should only have
      // one argument, so we can just replace it with the argument
      debug_assert(instruction.arguments.size() == 1,
                   "Phi node in target block has multiple arguments");
      debug_assert(instruction.labels == std::vector<std::string>({source}),
                   "Phi node in target block has the wrong labels");
      const std::string &argument = instruction.arguments[0];
      source_block.instructions.push_back(
          Instruction::id(instruction.destination, argument, instruction.type));
    } else {
      source_block.instructions.push_back(instruction);
    }
  }

  // If target block is an exit block, make source block an exit block
  if (exiting_blocks.count(target) > 0) {
    exiting_blocks.insert(source);
  }

  blocks.erase(target);
  block_labels.erase(
      std::find(block_labels.begin(), block_labels.end(), target));
  exiting_blocks.erase(target);

  recompute_graph(true);
}

// Splits the given block so that the given instruction idx becomes the first
// instruction in a new block
std::string ControlFlowGraph::split_block(const std::string &block_label,
                                          const size_t instruction_idx,
                                          const std::string &new_label_hint) {
  debug_assert(blocks.count(block_label) > 0, "No block with label {}",
               block_label);
  auto &block = get_block(block_label);
  debug_assert(instruction_idx < block.instructions.size(),
               "Cannot split block at the last instruction");

  // Create a new block
  std::string new_block_label = get_fresh_label(new_label_hint);
  Block new_block;
  new_block.entry_label = new_block_label;
  new_block.instructions.push_back(Instruction::label(new_block_label));
  new_block.instructions.insert(new_block.instructions.end(),
                                block.instructions.begin() + instruction_idx,
                                block.instructions.end());
  block.instructions.erase(block.instructions.begin() + instruction_idx,
                           block.instructions.end());
  block.instructions.push_back(Instruction::jmp(new_block_label));
  blocks[new_block_label] = new_block;

  const auto label_it =
      std::find(block_labels.begin(), block_labels.end(), block_label);
  block_labels.insert(label_it + 1, new_block_label);

  recompute_graph(true);

  return new_block_label;
}

void ControlFlowGraph::rename_label(const std::string &old_label,
                                    const std::string &new_label) {
  if (old_label == new_label)
    return;
  debug_assert(blocks.count(old_label) > 0,
               "Cannot rename non-existent label '{}'", old_label);
  debug_assert(blocks.count(new_label) == 0,
               "Cannot rename label to an existing label '{}'", new_label);
  if (entry_label == old_label)
    entry_label = new_label;

  for (const auto &label : block_labels) {
    auto &block = get_block(label);
    for (auto &instruction : block.instructions) {
      const auto it = std::find(instruction.labels.begin(),
                                instruction.labels.end(), old_label);
      if (it != instruction.labels.end())
        *it = new_label;
    }
  }

  auto &block = get_block(old_label);
  block.entry_label = new_label;
  blocks[new_label] = block;
  blocks.erase(old_label);
  const auto it =
      std::find(block_labels.begin(), block_labels.end(), old_label);
  *it = new_label;

  recompute_graph(true);
}

// Dominators
void ControlFlowGraph::compute_dominators() {
  dominators.clear();
  immediate_dominators.clear();
  dominance_frontiers.clear();

  const size_t num_labels = block_labels.size();

  // Create a mapping from labels to their indices in the block_labels vector
  std::unordered_map<std::string, size_t> label_to_index;
  for (size_t i = 0; i < num_labels; i++) {
    label_to_index[block_labels[i]] = i;
  }

  // Initialize the dominator matrix
  std::vector<std::vector<bool>> dominator_matrix;
  dominator_matrix.resize(num_labels);
  // Initially, every non-entry block is dominated by every other block
  for (size_t i = 1; i < num_labels; i++) {
    dominator_matrix[i].resize(num_labels, true);
  }
  // ... and the entry block is only dominated by itself
  dominator_matrix[0] = std::vector<bool>(num_labels, false);
  dominator_matrix[0][0] = true;

  while (true) {
    bool changed = false;
    for (size_t i = 1; i < num_labels; ++i) {
      const auto old_set = dominator_matrix[i];
      for (const std::string &pred :
           get_block(block_labels[i]).incoming_blocks) {
        const size_t pred_index = label_to_index[pred];
        // dominator_matrix[i] &= dominator_matrix[pred]
        for (size_t k = 0; k < num_labels; k++) {
          dominator_matrix[i][k] =
              dominator_matrix[i][k] && dominator_matrix[pred_index][k];
        }
      }
      dominator_matrix[i][i] = true;
      changed |= old_set != dominator_matrix[i];
    }
    if (!changed)
      break;
  }

  // Set up memoized versions of the dominator queries
  const auto dominates = [&](const size_t source, const size_t target) {
    return dominator_matrix[target][source];
  };
  const auto strictly_dominates = [&](const size_t source,
                                      const size_t target) {
    return source != target && dominates(source, target);
  };

  const auto immediately_dominates = [&](const size_t source,
                                         const size_t target) -> bool {
    if (source == target || !dominates(source, target))
      return false;
    for (size_t label = 0; label < num_labels; ++label) {
      if (strictly_dominates(source, label) &&
          strictly_dominates(label, target))
        return false;
    }
    return true;
  };

  // The domination frontier of 'source' contains 'target' if 'source' does
  // NOT dominate 'target' but 'source' dominates a predecessor of 'target'
  const auto is_in_dominance_frontier = [&](const size_t source,
                                            const size_t target) -> bool {
    if (dominates(source, target))
      return false;
    const std::string target_label = block_labels[target];
    for (const std::string &pred_label :
         get_block(target_label).incoming_blocks) {
      const size_t pred = label_to_index.at(pred_label);
      if (dominates(source, pred))
        return true;
    }
    return false;
  };

  for (size_t i = 0; i < num_labels; ++i) {
    const std::string label = block_labels[i];
    for (size_t j = 0; j < num_labels; ++j) {
      const std::string other_label = block_labels[j];
      if (dominator_matrix[i][j])
        dominators[label].insert(other_label);
    }
  }

  immediate_dominators[entry_label] = "(none)";
  for (size_t i = 0; i < num_labels; ++i) {
    const std::string label = block_labels[i];
    for (const std::string &other_label : dominators.at(label)) {
      const size_t j = label_to_index.at(other_label);
      if (immediately_dominates(j, i))
        immediate_dominators[label] = other_label;
    }
  }
  for (size_t i = 0; i < num_labels; ++i) {
    const std::string label = block_labels[i];
    for (size_t j = 0; j < num_labels; ++j) {
      const std::string other_label = block_labels[j];
      if (is_in_dominance_frontier(j, i))
        dominance_frontiers[other_label].insert(label);
    }
  }
}

std::string
ControlFlowGraph::immediate_dominator(const std::string &label) const {
  if (immediate_dominators.count(label) == 0)
    return "(none)";
  return immediate_dominators.at(label);
}

std::unordered_set<std::string>
ControlFlowGraph::dominance_frontier(const std::string &label) const {
  if (dominance_frontiers.count(label) == 0)
    return {};
  return dominance_frontiers.at(label);
}

} // namespace bril
