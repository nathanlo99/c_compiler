

#include "bril.hpp"

namespace bril {

// Given a function call instruction, inline the function call
// NOTE: This function does not make any effort to determine whether the
// inlining is a good idea
void Program::inline_function_call(const std::string &function_name,
                                   const std::string &block_label,
                                   const size_t instruction_idx) {
  auto &function = get_function(function_name);
  auto &block = function.get_block(block_label);
  runtime_assert(instruction_idx < block.instructions.size(),
                 "Instruction index out of bounds");
  const auto &instruction = block.instructions[instruction_idx];
  runtime_assert(instruction.opcode == Opcode::Call,
                 "Instruction is not a call");
  const std::string &called_function_name = instruction.funcs[0];
  runtime_assert(called_function_name != function_name,
                 "Cannot inline a function into itself");

  // Make sure the called function has an expected form: its entry block should
  // appear first, and it should have a single exit block, which appears last in
  // the list of block labels
  const auto &called_function = get_function(called_function_name);
  runtime_assert(called_function.block_labels[0] == called_function.entry_label,
                 "Called function does not start with its entry block");
  runtime_assert(called_function.exiting_blocks ==
                     std::set<std::string>{called_function.block_labels.back()},
                 "Called function does not end with its unique exit block");

  const std::string split_label = function.split_block(
      block_label, instruction_idx, called_function_name + "Inline");
}

/*
@f(x: int, y: int): int {
  z: int = add x y;
.bb1:
  ret z;
}

BEFORE:
calling_block:
  ...
  d: int = call @f a b c;
  ...

AFTER:
calling_block:
  ...
  a: int = id x;
  b: int = id y;
  jmp inline_f_entry;
inline_f_entry:
  z: int = add x y;
.bb1:
  d: int = id z;
  jmp inline_f_exit;
inline_f_exit:
  ...
*/

} // namespace bril
