
#include "bril_to_mips_generator.hpp"

namespace bril {

bool BRILToMIPSGenerator::remove_globally_unused_writes() {
  bool result = false;
  while (true) {
    bool changed = false;
    // Whenever a register is written to, but never read, remove the write
    // NOTE: We ensure that the register 3 is always globally read, since it is
    // the return value of the function
    std::set<Reg> read_registers = {Reg::R3, Reg::R31};
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
          debug_assert(i + 1 < instructions.size(), "Lis not followed by word");
          instructions[i + 1] = MIPSInstruction::comment("  (removed lis)");
        }
        instructions[i] = MIPSInstruction::comment(fmt::format(
            "Removing globally unused write to {}", written_register.value()));
        changed = true;
      }
    }

    result |= changed;
    if (!changed)
      break;
  }
  return result;
}

bool BRILToMIPSGenerator::remove_locally_unused_writes() {
  bool changed = false;
  for (size_t i = 0; i < instructions.size(); ++i) {
    auto &instruction = instructions[i];
    const auto dest_optional = instruction.written_register();
    if (!dest_optional.has_value())
      continue;
    const Reg dest = dest_optional.value();

    // These registers' values are read by the callers of the function, so we
    // cannot remove writes to them
    if (dest == Reg::R31 || dest == Reg::R30 || dest == Reg::R29)
      continue;

    bool read = false;

    // If the register is written to, but never read, remove the write
    for (size_t j = i + 1; j < instructions.size(); ++j) {
      const auto &other_instruction = instructions[j];

      // If we reach a jump, the register is read if and only if it's the return
      // value of the function
      if (other_instruction.is_jump() ||
          other_instruction.opcode == ::Opcode::Label) {
        // If we're exiting the function and the register is not $3, the
        // register is not read
        if (other_instruction.opcode == ::Opcode::Jr && dest != Reg::R3) {
          read = false;
        } else {
          read = true;
        }
        break;
      }
      // The register was read before it was written to again: the write cannot
      // be removed
      if (other_instruction.read_registers().count(dest) > 0) {
        read = true;
        break;
      }
      if (other_instruction.written_register() == dest)
        break;
    }

    if (!read) {
      if (instructions[i].opcode == ::Opcode::Lis) {
        debug_assert(i + 1 < instructions.size(), "Lis not followed by word");
        instructions[i + 1] = MIPSInstruction::comment("^");
      }
      instructions[i] = MIPSInstruction::comment(
          fmt::format("Removing locally unused write to {}", dest));
      changed = true;
    }
  }
  return changed;
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

bool BRILToMIPSGenerator::collapse_moves() {
  bool changed = false;
  // If a register is copied, replace all uses of the source register with the
  // destination register, until the source or destination register is
  // overwritten
  for (size_t i = 0; i < instructions.size(); ++i) {
    auto &instruction = instructions[i];
    if (instruction.opcode == ::Opcode::Sub && instruction.t == Reg::R0)
      instruction = MIPSInstruction::add(instruction.d, instruction.s, Reg::R0);
    if (instruction.opcode != ::Opcode::Add)
      continue;
    if (instruction.s == Reg::R0 && instruction.t != Reg::R0) {
      std::swap(instruction.s, instruction.t);
      changed = true;
    }
    // Now if there is a 0, it must be in the t register
    if (instruction.t != Reg::R0)
      continue;

    const Reg src = instruction.s, dest = instruction.d;
    if (src == dest) {
      instructions[i] = MIPSInstruction::comment("Removing move to self");
      changed = true;
      continue;
    }

    const std::set<::Opcode> substitutable_opcodes = {
        ::Opcode::Add, ::Opcode::Sub,  ::Opcode::Mult, ::Opcode::Multu,
        ::Opcode::Div, ::Opcode::Divu, ::Opcode::Slt,  ::Opcode::Sltu,
        ::Opcode::Beq, ::Opcode::Bne,  ::Opcode::Sw,   ::Opcode::Lw,
    };
    for (size_t j = i + 1; j < instructions.size(); ++j) {
      auto &other_instruction = instructions[j];
      if (other_instruction.is_jump() ||
          other_instruction.opcode == ::Opcode::Label)
        break;

      // Substitute arguments
      if (substitutable_opcodes.count(other_instruction.opcode) > 0) {
        changed |= other_instruction.substitute_arguments(dest, src);
      }

      // If the source register or destination register is written to, break
      const auto written_register = other_instruction.written_register();
      if (written_register == src || written_register == dest)
        break;
    }
  }
  return changed;
}

} // namespace bril
