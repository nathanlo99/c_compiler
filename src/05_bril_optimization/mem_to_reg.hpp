
#pragma once

#include "bril.hpp"
#include "data_flow/alias_analysis.hpp"

namespace bril {

inline size_t promote_memory_to_registers(ControlFlowGraph &function) {
  size_t result = 0;
  // TODO: Implement this function

  // 1. Compute aliasing information
  const auto alias_data = MayAliasAnalysis(function).run();

  // 2. For each memory access which can only be one value, replace it with a
  // copy of the original value
  // 3. For every memory store of a known value, replace it with a variable
  // assignment
  for (const auto &label : function.block_labels) {
    auto &block = function.get_block(label);
    for (size_t i = 0; i < block.instructions.size(); ++i) {
      auto &instruction = block.instructions[i];
      const auto &locations_in = alias_data.get_data_in(label, i);
      const auto &locations_out = alias_data.get_data_out(label, i);

      switch (instruction.opcode) {
      case Opcode::Id: {
        const auto &possible_locations =
            locations_out.at(instruction.destination);
        if (possible_locations.size() == 1) {
          const auto &location = *possible_locations.begin();
          if (location.type == MemoryLocation::Type::Allocation)
            continue;
          const auto &variable = location.name;
          instruction =
              Instruction::addressof(instruction.destination, variable);
          result++;
        }
      } break;

      case Opcode::Store: {
        const auto &possible_locations =
            locations_in.at(instruction.arguments[0]);
        if (possible_locations.size() == 1) {
          const auto &location = *possible_locations.begin();
          if (location.type == MemoryLocation::Type::Allocation)
            continue;
          const auto &variable = location.name;
          instruction = Instruction::id(variable, instruction.arguments[1],
                                        instruction.type);
          result++;
        }
      } break;

      case Opcode::Load: {
        const auto &possible_locations =
            locations_out.at(instruction.arguments[0]);
        if (possible_locations.size() == 1) {
          const auto &location = *possible_locations.begin();
          if (location.type == MemoryLocation::Type::Allocation)
            continue;
          const auto &variable = location.name;
          instruction = Instruction::id(instruction.destination, variable,
                                        instruction.type);
          result++;
        }
      } break;

      default:
        break;
      }
    }
  }

  return result;
}

} // namespace bril
