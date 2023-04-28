
#pragma once

#include "bril.hpp"
#include "data_flow/alias_analysis.hpp"

namespace bril {

inline size_t promote_memory_to_registers(ControlFlowGraph &function) {
  size_t result = 0;

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
      const auto destination = instruction.destination;
      const auto &locations_in = alias_data.get_data_in(label, i);
      const auto &locations_out = alias_data.get_data_out(label, i);

      switch (instruction.opcode) {
      case Opcode::Id: {
        debug_assert(locations_out.count(destination) > 0,
                     "Missing location of {} in id instruction", destination);
        const auto &possible_locations = locations_out.at(destination);
        if (possible_locations.size() == 1) {
          const auto &location = *possible_locations.begin();
          if (location.type != MemoryLocation::Type::Address)
            continue;
          const auto &variable = location.name;
          instruction = Instruction::addressof(destination, variable);
          result++;
        }
      } break;

      case Opcode::Store: {
        debug_assert(locations_in.count(instruction.arguments[0]) > 0,
                     "Missing location of {} in store instruction",
                     instruction.arguments[0]);
        const auto &possible_locations =
            locations_in.at(instruction.arguments[0]);
        if (possible_locations.size() == 1) {
          const auto &location = *possible_locations.begin();
          if (location.type != MemoryLocation::Type::Address)
            continue;
          const auto &variable = location.name;
          instruction = Instruction::id(variable, instruction.arguments[1],
                                        instruction.type);
          result++;
        }
      } break;

      case Opcode::Load: {
        debug_assert(locations_out.count(instruction.arguments[0]) > 0,
                     "Missing location of {} in load instruction",
                     instruction.arguments[0]);
        const auto &possible_locations =
            locations_out.at(instruction.arguments[0]);
        if (possible_locations.size() == 1) {
          const auto &location = *possible_locations.begin();
          if (location.type != MemoryLocation::Type::Address)
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
