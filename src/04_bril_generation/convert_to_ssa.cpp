
#include "bril.hpp"

namespace bril {

// Convert a Bril function which doesn't use pointers, to SSA form
void bril::ControlFlowGraph::convert_to_ssa() {
  // Cannot (yet) convert function with memory accesses to SSA form
  if (uses_pointers())
    return;

  std::cerr << "Converting function " << name << " to SSA form" << std::endl;

  // 0. Gather variables and the blocks they're defined in
  std::map<std::string, std::set<size_t>> defs;
  std::map<std::string, size_t> num_defs;
  for (size_t block_idx = 0; block_idx < blocks.size(); block_idx++) {
    const auto &block = blocks[block_idx];
    for (const auto &instruction : block.instructions) {
      if (instruction.destination != "") {
        defs[instruction.destination].insert(block_idx);
        num_defs[instruction.destination] += 1;
      }
    }
  }

  for (const auto &[var, blocks_with_var] : defs) {
    std::cerr << "Variable " << var << " is defined " << num_defs[var]
              << " times, in blocks: ";
    for (const auto &block : blocks_with_var) {
      std::cerr << block << " ";
    }
    std::cerr << std::endl;
  }

  // 1. Add phi nodes
  for (const auto &[var, blocks_with_var] : defs) {
    if (num_defs[var] <= 1)
      continue;
    std::cerr << "Adding phi nodes for variable " << var << std::endl;

    std::set<size_t> queue = blocks_with_var;
    std::set<size_t> has_def = blocks_with_var;
    while (!queue.empty()) {
      const size_t block_idx = *queue.begin();
      queue.erase(queue.begin());

      for (const auto &frontier_idx : dominance_frontiers[block_idx]) {
        if (has_def.count(frontier_idx) > 0)
          continue;

        // TODO: Add a phi node
        std::cerr << "Adding phi node for variable " << var << " in block "
                  << frontier_idx << std::endl;
        std::vector<std::string> arguments;
        std::vector<std::string> labels;
        for (const size_t pred : blocks[frontier_idx].incoming_blocks) {
          arguments.push_back(var);
          labels.push_back(".bb_" + std::to_string(pred));
        }
        const Instruction phi_node = Instruction::phi(var, arguments, labels);
        blocks[frontier_idx].instructions.insert(
            blocks[frontier_idx].instructions.begin(), phi_node);

        queue.insert(frontier_idx);
        has_def.insert(frontier_idx);
      }
    }
  }

  std::map<std::string, std::vector<std::string>> definitions;
  std::map<std::string, size_t> next_idx;
  for (auto &argument : arguments) {
    const auto old_name = argument.name;
    const auto new_name = old_name + ".0";
    argument.name = new_name;
    definitions[old_name] = {new_name};
    next_idx[old_name] = 1;
  }
  rename_variables(0, definitions, next_idx);
}

void ControlFlowGraph::rename_variables(
    const size_t block_idx,
    std::map<std::string, std::vector<std::string>> definitions,
    std::map<std::string, size_t> &next_idx) {

  std::cerr << "Renaming variables in block " << block_idx << std::endl;

  Block &block = blocks[block_idx];
  for (size_t i = 0; i < block.instructions.size(); ++i) {
    auto &instruction = block.instructions[i];
    if (instruction.opcode != Opcode::Phi) {
      for (auto &argument : instruction.arguments) {
        runtime_assert(definitions.count(argument) > 0,
                       "Variable " + argument + " not defined");
        argument = definitions[argument].back();
      }
    }

    if (instruction.destination != "") {
      const std::string new_name =
          instruction.destination + "." +
          std::to_string(next_idx[instruction.destination]);
      next_idx[instruction.destination] += 1;
      definitions[instruction.destination].push_back(new_name);
      instruction.destination = new_name;
    }
  }

  for (const size_t succ : block.outgoing_blocks) {
    for (auto &instruction : blocks[succ].instructions) {
      if (instruction.opcode != Opcode::Phi)
        continue;
      for (size_t i = 0; i < instruction.arguments.size(); ++i) {
        if (instruction.labels[i] == ".bb_" + std::to_string(block_idx)) {
          instruction.arguments[i] =
              definitions[instruction.destination].back();
        }
      }
    }
  }

  for (size_t succ = 0; succ < blocks.size(); ++succ) {
    if (_immediately_dominates(block_idx, succ))
      rename_variables(succ, definitions, next_idx);
  }
}

/*
Pseudocode:

- for every variable v:
  - define def[v] = { blocks that define v }
  - for every block B in def[v]:
    - for every block B' in the dominance frontier DF(B):
      - if we don't already have a phi node for v in B':
        - add one
        - add B' to def[v]

stack[v] is a stack of variable names (for every variable v)

def rename(block):
  for instr in block:
    replace each argument to instr with stack[old_name].top()

    replace instr's destination with a new name
    push that new name onto stack[old_name]

  for each successor S of block,
    for each phi node P in S:
      if P is for a variable v, make the edge from S read from stack[v]

  for each child B of block:
    rename(B)

  pop all the names we've just pushed onto the stack

*/

} // namespace bril
