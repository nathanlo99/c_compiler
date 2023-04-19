
#include "bril_to_mips_generator.hpp"

namespace bril {

bool BRILToMIPSGenerator::remove_unused_writes() {
  bool result = false;
  while (true) {
    bool changed = false;
    // Whenever a register is written to, but never read, remove the write
    std::set<size_t> read_registers;
    // First, gather the set of registers that are read
    for (size_t i = 0; i < instructions.size(); ++i) {
      const auto &instruction = instructions[i];
      const auto instruction_reads = instruction.read_registers();
      read_registers.insert(instruction_reads.begin(), instruction_reads.end());
    }
    // Then, remove any writes to registers that are not read
    for (size_t i = 0; i < instructions.size(); ++i) {
      auto &instruction = instructions[i];
      const auto written_register = instruction.written_register();
      if (!written_register.has_value())
        continue;
      if (read_registers.count(written_register.value()) == 0) {
        if (instructions[i].opcode == ::Opcode::Lis) {
          runtime_assert(i + 1 < instructions.size(),
                         "Lis not followed by word");
          instructions[i + 1] = MIPSInstruction::comment("^");
        }
        instructions[i] =
            MIPSInstruction::comment("Removing unused write to register " +
                                     std::to_string(written_register.value()));

        changed = true;
      }
    }

    result |= changed;
    if (!changed)
      break;
  }
  return result;
}

bool BRILToMIPSGenerator::remove_fallthrough_jumps() {
  // Whenever an unconditional jump is followed by the label it jumps to,
  // remove it
  bool changed = false;
  for (size_t i = 0; i + 1 < instructions.size(); ++i) {
    const auto &this_instruction = instructions[i];
    if (this_instruction.opcode != ::Opcode::Beq ||
        this_instruction.s != this_instruction.t)
      continue;
    size_t j = i + 1;
    while (j < instructions.size() &&
           instructions[j].opcode == ::Opcode::Comment) {
      ++j;
    }
    const auto &next_instruction = instructions[j];
    if (next_instruction.opcode != ::Opcode::Label ||
        this_instruction.string_value != next_instruction.string_value)
      continue;
    instructions[i] = MIPSInstruction::comment(
        "jmp " + next_instruction.string_value + "; (fallthrough)");
    changed = true;
  }
  return changed;
}

bool BRILToMIPSGenerator::remove_unused_labels() {
  bool changed = false;
  std::set<std::string> used_labels;
  const std::set<::Opcode> used_opcodes = {::Opcode::Beq, ::Opcode::Bne,
                                           ::Opcode::Word};
  for (size_t i = 0; i < instructions.size(); ++i) {
    const auto &instruction = instructions[i];
    if (used_opcodes.count(instruction.opcode) == 0 ||
        instruction.string_value == "")
      continue;
    used_labels.insert(instruction.string_value);
  }

  for (size_t i = 0; i < instructions.size(); ++i) {
    if (instructions[i].opcode == ::Opcode::Label &&
        used_labels.count(instructions[i].string_value) == 0) {
      instructions[i] = MIPSInstruction::comment("Removed unused label " +
                                                 instructions[i].string_value);
      changed = true;
    }
  }
  return changed;
}

} // namespace bril
