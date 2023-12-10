
#pragma once

#include "ast_node.hpp"
#include "bril_instruction.hpp"
#include "util.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace bril {

struct Function {
  std::string name;
  std::vector<bril::Variable> arguments;
  Type return_type;
  std::vector<bril::Instruction> instructions;

  Function(const std::string &name, const std::vector<Variable> &arguments,
           const Type return_type)
      : name(name), arguments(arguments), return_type(return_type) {}
};

struct Block {
  std::string entry_label;
  std::vector<Instruction> instructions;
  std::vector<std::string> exit_labels;

  std::unordered_set<std::string> incoming_blocks;
  std::unordered_set<std::string> outgoing_blocks;

  // Insert an instruction at the beginning of the block, after any labels.
  void prepend(const Instruction &instruction) {
    auto it = instructions.begin();
    while (it != instructions.end() && it->opcode == Opcode::Label)
      ++it;
    instructions.insert(it, instruction);
  }

  template <typename Pred> bool all_of(const Pred &pred) const {
    return std::all_of(instructions.begin(), instructions.end(), pred);
  }
  template <typename Pred> bool any_of(const Pred &pred) const {
    return std::any_of(instructions.begin(), instructions.end(), pred);
  }

  // Returns true if the block has any instructions other than labels.
  bool has_instructions() const {
    return any_of([](const auto &instruction) {
      return instruction.opcode != Opcode::Label;
    });
  }

  bool uses_pointers() const {
    return any_of(
        [](const auto &instruction) { return instruction.uses_memory(); });
  }

  bool has_loads_or_stores() const {
    return any_of(
        [](const auto &instruction) { return instruction.is_load_or_store(); });
  }

  friend std::ostream &operator<<(std::ostream &os, const Block &block) {
    using util::operator<<;
    if (!block.incoming_blocks.empty()) {
      os << "incoming_blocks: " << block.incoming_blocks << std::endl;
    }
    if (!block.outgoing_blocks.empty()) {
      os << "outgoing_blocks: " << block.outgoing_blocks << std::endl;
    }

    os << "instructions: " << std::endl;
    for (const auto &instruction : block.instructions) {
      if (instruction.opcode == Opcode::Label)
        os << instruction.labels[0] << ":" << std::endl;
      else
        os << "  " << instruction << std::endl;
    }
    return os;
  }
};

// Stores the CFG for a single procedure
struct ControlFlowGraph {
  std::string name;
  std::vector<bril::Variable> arguments;
  Type return_type;

  std::vector<std::string> block_labels;
  std::string entry_label;
  std::unordered_map<std::string, Block> blocks;
  std::unordered_set<std::string> exiting_blocks;

  // Dominator data structures
  // std::unordered_map<std::string, std::unordered_map<std::string, bool>>
  //     raw_dominators;
  std::unordered_map<std::string, std::unordered_set<std::string>> dominators;
  std::unordered_map<std::string, std::string> immediate_dominators;
  std::unordered_map<std::string, std::unordered_set<std::string>>
      dominance_frontiers;

  // True if the graph has been modified since the last time dominator data was
  // computed
  bool is_graph_dirty = true;

  // Construct a CFG from a function
  explicit ControlFlowGraph(const Function &function);

  std::string get_fresh_label(const std::string &prefix) const {
    if (blocks.count(prefix) == 0)
      return prefix;

    size_t idx = 0;
    while (true) {
      const std::string label = prefix + std::to_string(idx);
      if (blocks.count(label) == 0)
        return label;
      ++idx;
    }
  }

  Block &get_block(const std::string &block_label) {
    debug_assert(blocks.count(block_label) > 0, "Block not found: {}",
                 block_label);
    return blocks.at(block_label);
  }
  const Block &get_block(const std::string &block_label) const {
    debug_assert(blocks.count(block_label) > 0, "Block not found: {}",
                 block_label);
    return blocks.at(block_label);
  }
  void add_block(const Block &block);
  void remove_block(const std::string &block_label);
  void combine_blocks(const std::string &source, const std::string &target);
  std::string split_block(const std::string &block_label,
                          const size_t instruction_idx,
                          const std::string &new_label_hint = "splitLabel");

  void rename_label(const std::string &old_label, const std::string &new_label);

  bool all_of_blocks(const std::function<bool(const Block &)> &pred) const {
    return std::all_of(blocks.begin(), blocks.end(),
                       [&pred](const auto &pair) { return pred(pair.second); });
  }
  bool any_of_blocks(const std::function<bool(const Block &)> &pred) const {
    return std::any_of(blocks.begin(), blocks.end(),
                       [&pred](const auto &pair) { return pred(pair.second); });
  }
  bool any_of_instructions(
      const std::function<bool(const Instruction &)> &pred) const {
    return any_of_blocks(
        [&pred](const Block &block) { return block.any_of(pred); });
  }

  bool uses_pointers() const {
    return any_of_blocks(
        [](const auto &block) { return block.uses_pointers(); });
  }

  bool uses_print() const {
    return any_of_instructions([](const Instruction &instruction) {
      return instruction.opcode == Opcode::Print;
    });
  }

  bool uses_heap() const {
    return any_of_instructions([](const Instruction &instruction) {
      return instruction.opcode == Opcode::Alloc ||
             instruction.opcode == Opcode::Free;
    });
  }

  bool has_phi_instructions() const {
    return any_of_instructions([](const Instruction &instruction) {
      return instruction.opcode == Opcode::Phi;
    });
  }

  size_t num_instructions() const {
    size_t num_instructions = 0;
    for (const auto &[label, block] : blocks) {
      num_instructions += block.instructions.size();
    }
    return num_instructions;
  }

  size_t num_labels() const { return block_labels.size(); }

  // Convert the CFG to SSA form, if it has no memory accesses
  void convert_to_ssa();
  void convert_from_ssa();
  bool is_in_ssa_form() const;
  void rename_variables(
      const std::string &block_label,
      std::unordered_map<std::string, std::vector<std::string>> definitions,
      std::unordered_map<std::string, size_t> &next_idx);

  // Applies a local pass to each block in the CFG and returns the number of
  // removed lines
  template <typename Func> size_t apply_local_pass(const Func &func) {
    size_t num_removed_lines = 0;
    for (const auto &label : block_labels) {
      auto &block = get_block(label);
      num_removed_lines += func(*this, block);
    }
    recompute_graph();
    return num_removed_lines;
  }

  template <typename Func> void for_each_block(const Func &func) const {
    for (const auto &label : block_labels) {
      const auto &block = get_block(label);
      func(block);
    }
  }

  template <typename Func> void for_each_instruction(const Func &func) const {
    for (const auto &label : block_labels) {
      const auto &block = get_block(label);
      for (const auto &instruction : block.instructions) {
        func(instruction);
      }
    }
  }

  template <typename Func> void for_each_block(const Func &func) {
    for (const auto &label : block_labels) {
      auto &block = get_block(label);
      func(block);
    }
  }

  template <typename Func> void for_each_instruction(const Func &func) {
    for (const auto &label : block_labels) {
      auto &block = get_block(label);
      for (auto &instruction : block.instructions) {
        func(instruction);
      }
    }
  }

  std::vector<Instruction> flatten() const {
    std::vector<Instruction> instructions;
    for (const auto &label : block_labels) {
      const auto &block = get_block(label);
      instructions.insert(instructions.end(), block.instructions.begin(),
                          block.instructions.end());
    }
    return instructions;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const ControlFlowGraph &graph) {
    using util::operator<<;
    const std::string separator = std::string(80, '-');
    os << "CFG for " << graph.name << "(";

    bool first = true;
    for (const auto &argument : graph.arguments) {
      if (first)
        first = false;
      else
        os << ", ";
      os << argument.name << ": " << argument.type;
    }
    os << ") : " << graph.return_type << std::endl;

    graph.for_each_block([&](const Block &block) {
      os << separator << std::endl;
      os << "label: " << block.entry_label << std::endl;
      os << "immediate dominator: "
         << graph.immediate_dominator(block.entry_label) << std::endl;
      os << block;
    });
    os << separator << std::endl;
    os << "exiting blocks: " << graph.exiting_blocks << std::endl;
    os << separator << std::endl;
    return os;
  }

  void add_edge(const std::string &source, const std::string &target);
  void remove_edge(const std::string &source, const std::string &target);

  void compute_edges();
  void compute_dominators();
  void recompute_graph(const bool force = false) {
    if (!force && !is_graph_dirty)
      return;
    compute_edges();
    compute_dominators();
    is_graph_dirty = false;
  }

  std::string immediate_dominator(const std::string &label) const;
  std::unordered_set<std::string>
  dominance_frontier(const std::string &label) const;
};

struct Program {
  std::map<std::string, ControlFlowGraph> functions;

  const ControlFlowGraph &wain() const {
    debug_assert(functions.count("wain") > 0, "wain not found");
    return functions.at("wain");
  }

  ControlFlowGraph &get_function(const std::string &name) {
    debug_assert(functions.count(name) > 0, "Function {} not found", name);
    return functions.at(name);
  }
  const ControlFlowGraph &get_function(const std::string &name) const {
    debug_assert(functions.count(name) > 0, "Function {} not found", name);
    return functions.at(name);
  }

  void convert_to_ssa() {
    for_each_function(
        [](ControlFlowGraph &function) { function.convert_to_ssa(); });
  }
  void convert_from_ssa() {
    for_each_function(
        [](ControlFlowGraph &function) { function.convert_from_ssa(); });
  }

  bool has_phi_instructions() const {
    return std::any_of(
        functions.begin(), functions.end(),
        [](const auto &pair) { return pair.second.has_phi_instructions(); });
  }
  bool uses_heap() const {
    return std::any_of(
        functions.begin(), functions.end(),
        [](const auto &pair) { return pair.second.uses_heap(); });
  }
  bool uses_print() const {
    return std::any_of(
        functions.begin(), functions.end(),
        [](const auto &pair) { return pair.second.uses_print(); });
  }

  void inline_function_call(const std::string &function_name,
                            const std::string &block_label,
                            const size_t instruction_idx);
  bool inline_function(const std::string &function_name,
                       const std::string &called_function_name);

  friend std::ostream &operator<<(std::ostream &os, const Program &program) {
    program.for_each_function(
        [&os](const auto &function) { os << function << std::endl; });
    return os;
  }

  void print_flattened(std::ostream &os) const {
    using util::operator<<;
    for (const auto &[name, function] : functions) {
      os << "@" << name << "(";
      bool first = true;
      for (const auto &argument : function.arguments) {
        if (first)
          first = false;
        else
          os << ", ";
        os << argument.name << ": " << argument.type;
      }
      os << ") : " << function.return_type << " {" << std::endl;
      for (const auto &instruction : function.flatten()) {
        if (instruction.opcode == Opcode::Label) {
          const auto label = instruction.labels[0];
          const auto padding = 50 - label.size();
          os << instruction.labels[0] << ":" << std::string(padding, ' ')
             << "preds = " << function.get_block(label).incoming_blocks
             << ", dominators = " << function.dominators.at(label) << std::endl;
        } else {
          os << "  " << instruction << std::endl;
        }
      }
      os << "}\n" << std::endl;
    }
  }

  template <typename Func> size_t apply_pass(const Func &func) {
    return func(*this);
  }

  template <typename Func> size_t apply_global_pass(const Func &func) {
    size_t num_removed_lines = 0;
    for (auto &[name, function] : functions)
      num_removed_lines += func(function);
    return num_removed_lines;
  }

  template <typename Func> size_t apply_local_pass(const Func &func) {
    size_t num_removed_lines = 0;
    for (auto &[name, function] : functions)
      num_removed_lines += function.apply_local_pass(func);
    return num_removed_lines;
  }

  template <typename Func> void for_each_function(const Func &func) const {
    for (auto &[name, function] : functions)
      func(function);
  }

  template <typename Func> void for_each_block(const Func &func) const {
    for_each_function([&](const ControlFlowGraph &function) {
      function.for_each_block(func);
    });
  }

  template <typename Func> void for_each_instruction(const Func &func) const {
    for_each_block([&](const Block &block) {
      for (const auto &instruction : block.instructions)
        func(instruction);
    });
  }

  template <typename Func> void for_each_function(const Func &func) {
    for (auto &[name, function] : functions)
      func(function);
  }

  template <typename Func> void for_each_block(const Func &func) {
    for_each_function(
        [&](ControlFlowGraph &function) { function.for_each_block(func); });
  }

  template <typename Func> void for_each_instruction(const Func &func) {
    for_each_block([&](Block &block) {
      for (auto &instruction : block.instructions)
        func(instruction);
    });
  }
};

} // namespace bril
