
#include "bril.hpp"

namespace bril {

// Convert a Bril function which doesn't use pointers, to SSA form
void ControlFlowGraph::convert_to_ssa() {
  // Cannot (yet) convert function with memory accesses to SSA form
  if (uses_pointers())
    return;

  // 0. Gather variables and the blocks they're defined in
  std::map<std::string, std::set<std::string>> defs;
  std::map<std::string, size_t> num_defs;
  std::map<std::string, Type> types;
  for (const auto &[entry_label, block] : blocks) {
    for (const auto &instruction : block.instructions) {
      if (instruction.destination != "") {
        defs[instruction.destination].insert(entry_label);
        num_defs[instruction.destination] += 1;
        types[instruction.destination] = instruction.type;
      }
    }
  }
  for (const auto &argument : arguments) {
    defs[argument.name].insert(entry_label);
    num_defs[argument.name] += 1;
    types[argument.name] = argument.type;
  }

  // 1. Add phi nodes
  for (const auto &[var, blocks_with_var] : defs) {
    if (num_defs[var] <= 1)
      continue;

    std::set<std::string> queue = blocks_with_var;
    std::set<std::string> has_def = blocks_with_var;
    std::set<std::string> has_phi;
    while (!queue.empty()) {
      const std::string block_label = *queue.begin();
      queue.erase(queue.begin());

      for (const std::string &frontier_label :
           dominance_frontiers[block_label]) {
        if (has_phi.count(frontier_label) > 0)
          continue;
        auto &frontier_block = blocks[frontier_label];

        std::vector<std::string> arguments;
        std::vector<std::string> labels;
        for (const std::string &pred : frontier_block.incoming_blocks) {
          arguments.push_back(var);
          labels.push_back(pred);
        }
        const Instruction phi_node =
            Instruction::phi(var, types[var], arguments, labels);
        frontier_block.prepend(phi_node);
        has_phi.insert(frontier_label);

        queue.insert(frontier_label);
        has_def.insert(frontier_label);
      }
    }
  }

  std::map<std::string, std::vector<std::string>> definitions;
  std::map<std::string, size_t> next_idx;
  for (const auto &argument : arguments) {
    definitions[argument.name] = {argument.name};
  }
  rename_variables(entry_label, definitions, next_idx);
}

void ControlFlowGraph::rename_variables(
    const std::string &block_label,
    std::map<std::string, std::vector<std::string>> definitions,
    std::map<std::string, size_t> &next_idx) {

  Block &block = blocks[block_label];

  // First, rename phi-node destinations
  for (auto &instruction : block.instructions) {
    if (instruction.opcode != Opcode::Phi)
      continue;
    const std::string new_name =
        instruction.destination + "." +
        std::to_string(next_idx[instruction.destination]);
    next_idx[instruction.destination] += 1;
    definitions[instruction.destination].push_back(new_name);
    instruction.destination = new_name;
  }

  for (auto &instruction : block.instructions) {
    if (instruction.opcode == Opcode::Phi)
      continue;
    for (auto &argument : instruction.arguments) {
      runtime_assert(definitions.count(argument) > 0,
                     "Variable " + argument + " not defined");
      argument = definitions[argument].back();
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

  for (const std::string &succ : block.outgoing_blocks) {
    for (auto &instruction : blocks[succ].instructions) {
      if (instruction.opcode != Opcode::Phi)
        continue;
      const std::string target_label = block_label;
      const auto it = std::find(instruction.labels.begin(),
                                instruction.labels.end(), target_label);
      runtime_assert(it != instruction.labels.end(),
                     "Label " + target_label + " not found in phi node");
      const size_t idx = it - instruction.labels.begin();
      const std::string old_argument = instruction.arguments[idx];
      if (definitions[old_argument].empty()) {
        instruction.arguments[idx] = "__undefined";
      } else {
        instruction.arguments[idx] = definitions[old_argument].back();
      }
    }
  }

  for (const auto &other_label : block_labels) {
    if (other_label != block_label &&
        _immediately_dominates(block_label, other_label)) {
      rename_variables(other_label, definitions, next_idx);
    }
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
