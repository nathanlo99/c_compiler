
#include "mips_generator.hpp"

bool MIPSGenerator::peephole_optimize() {
  bool changed = false;
  const MIPSInstruction push_3_0 = MIPSInstruction::sw(Reg::R3, -4, Reg::SP);
  const MIPSInstruction push_3_1 = MIPSInstruction::sub(Reg::SP, Reg::SP, Reg::R4);
  const MIPSInstruction pop_5_0 = MIPSInstruction::add(Reg::SP, Reg::SP, Reg::R4);
  const MIPSInstruction pop_5_1 = MIPSInstruction::lw(Reg::R5, -4, Reg::SP);
  for (size_t i = 0; i + 1 < instructions.size(); ++i) {
    const bool is_push_3 =
        instructions[i] == push_3_0 && instructions[i + 1] == push_3_1;
    if (!is_push_3)
      continue;
    for (size_t j = i + 2; j + 1 < instructions.size(); ++j) {
      const bool is_pop_5 =
          instructions[j] == pop_5_0 && instructions[j + 1] == pop_5_1;
      const auto &instruction = instructions[j];

      if (is_pop_5) {
        instructions[i] = MIPSInstruction::add(Reg::R5, Reg::R3, Reg::R0);
        instructions[i].comment_value = "Peephole optimization: push 3, pop 5";
        instructions.erase(instructions.begin() + j + 1);
        instructions.erase(instructions.begin() + j);
        instructions.erase(instructions.begin() + i + 1);
        changed = true;
        break;
      }

      // Otherwise, if the instruction is a jump, label, branch, or uses 5, we
      // can't optimize
      if (instruction.opcode == Opcode::Jalr ||
          instruction.opcode == Opcode::Jr ||
          instruction.opcode == Opcode::Label ||
          instruction.opcode == Opcode::Beq ||
          instruction.opcode == Opcode::Bne || instruction.d == Reg::R5 ||
          instruction.s == Reg::R5 || instruction.t == Reg::R5) {
        break;
      }
    }
  }
  return changed;
}

bool MIPSGenerator::optimize_moves() {
  bool changed = false;
  for (size_t i = 0; i < instructions.size(); ++i) {
    auto &copy_instruction = instructions[i];
    // LHS move: add $d, $s, $0 (essentially $d = $s)
    const bool is_lhs_move = (copy_instruction.opcode == Opcode::Add ||
                              copy_instruction.opcode == Opcode::Sub) &&
                             copy_instruction.t == Reg::R0;
    // RHS move: add $d, $0, $t (essentially $d = $t)
    const bool is_rhs_move = (copy_instruction.opcode == Opcode::Add ||
                              copy_instruction.opcode == Opcode::Sub) &&
                             copy_instruction.s == Reg::R0;
    if (!is_lhs_move && !is_rhs_move) // Not a move
      continue;
    if (is_lhs_move && is_rhs_move) // Can't optimize $d = $0
      continue;
    copy_instruction.comment_value += " (move)";

    const Reg destination = copy_instruction.d;
    const Reg source = is_lhs_move ? copy_instruction.s : copy_instruction.t;

    for (size_t j = i + 1; j < instructions.size(); ++j) {
      auto &use_instruction = instructions[j];
      // If the instruction is a jump, label, branch, we can't optimize
      if (use_instruction.opcode == Opcode::Jalr ||
          use_instruction.opcode == Opcode::Jr ||
          use_instruction.opcode == Opcode::Label ||
          use_instruction.opcode == Opcode::Beq ||
          use_instruction.opcode == Opcode::Bne) {
        break;
      }

      // Uses are okay, writes are bad
      if (use_instruction.s == destination) {
        use_instruction.s = source;
        use_instruction.comment_value += fmt::format(" (moved $s <- {}", source, destination);
      }
      if (use_instruction.t == destination) {
        use_instruction.t = source;
        use_instruction.comment_value  += fmt::format(" (moved $t <- {}", source, destination);
      }
      if (use_instruction.d == source || use_instruction.d == destination)
        break;
    }
  }
  return changed;
}
