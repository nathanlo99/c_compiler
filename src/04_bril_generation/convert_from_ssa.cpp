
#include "bril.hpp"

namespace bril {

void ControlFlowGraph::convert_from_ssa() {
  // For each phi node, make a new variable, and add a copy instruction to each
  // predecessor with the corresponding argument

  for (const auto &label : block_labels) {
    auto &block = get_block(label);
    // NOTE: We loop with indices in case we need to remove phi nodes
    for (size_t i = 0; i < block.instructions.size(); i++) {
      auto &phi_node = block.instructions[i];
      if (phi_node.opcode != Opcode::Phi)
        continue;
      const auto &destination = phi_node.destination;
      const auto &new_destination = "from_ssa." + destination;
      const auto &arguments = phi_node.arguments;
      for (size_t j = 0; j < arguments.size(); j++) {
        const auto &argument = arguments[j];
        const auto &predecessor = phi_node.labels[j];
        auto &predecessor_block = get_block(predecessor);
        // Insert a copy before the last instruction
        predecessor_block.instructions.insert(
            predecessor_block.instructions.end() - 1,
            Instruction::id(new_destination, argument, phi_node.type));
      }
      phi_node = Instruction::id(destination, new_destination, phi_node.type);
    }
  }
}

} // namespace bril
