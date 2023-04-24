
#pragma once

#include "bril.hpp"
#include "util.hpp"

namespace bril {

inline void canonicalize_names(ControlFlowGraph &function) {
  size_t next_variable_idx = 0;
  std::map<std::string, std::string> renamed_variables;
  size_t next_label_idx = 0;
  std::map<std::string, std::string> renamed_labels;

  const auto insert_parameter = [&](const std::string &arg) {
    renamed_variables[arg] = arg;
  };
  const auto insert_entry_label = [&](const std::string &entry_label) {
    renamed_labels[entry_label] = entry_label;
  };
  const auto insert_variable = [&](const std::string &var) {
    if (renamed_variables.count(var) == 0)
      renamed_variables[var] = "%" + std::to_string(next_variable_idx++);
  };
  const auto insert_label = [&](const std::string &label) {
    if (renamed_labels.count(label) == 0)
      renamed_labels[label] = ".L" + std::to_string(next_label_idx++);
  };

  for (const auto &argument : function.arguments)
    insert_parameter(argument.name);
  insert_entry_label(function.entry_label);
  for (const auto &block_label : function.block_labels) {
    insert_label(block_label);
    const auto &block = function.get_block(block_label);
    for (const auto &instruction : block.instructions) {
      for (const auto &argument : instruction.arguments)
        insert_variable(argument);
      if (instruction.destination != "")
        insert_variable(instruction.destination);
    }
  }

  for (const auto &block_label : function.block_labels) {
    auto &block = function.get_block(block_label);
    block.entry_label = renamed_labels.at(block.entry_label);
    for (auto &instruction : block.instructions) {
      for (auto &argument : instruction.arguments)
        argument = renamed_variables.at(argument);
      if (instruction.destination != "")
        instruction.destination = renamed_variables.at(instruction.destination);
      for (auto &label : instruction.labels)
        label = renamed_labels.at(label);
    }
  }
  std::map<std::string, Block> new_blocks;
  std::vector<std::string> new_block_labels;
  std::set<std::string> new_exit_blocks;
  for (const auto &label : function.block_labels) {
    new_blocks[renamed_labels.at(label)] = function.get_block(label);
    new_block_labels.push_back(renamed_labels.at(label));
  }
  for (const auto &label : function.exiting_blocks)
    new_exit_blocks.insert(renamed_labels.at(label));
  function.blocks = new_blocks;
  function.block_labels = new_block_labels;
  function.entry_label = renamed_labels.at(function.entry_label);
  function.exiting_blocks = new_exit_blocks;

  function.is_graph_dirty = true;
  function.recompute_graph();
}

} // namespace bril
