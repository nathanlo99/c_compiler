

#include "bril.hpp"
#include "util.hpp"

namespace bril {

// Given a function call instruction, inline the function call
// NOTE: This function does not make any effort to determine whether the
// inlining is a good idea
void Program::inline_function_call(const std::string &function_name,
                                   const std::string &block_label,
                                   const size_t instruction_idx) {
  auto &function = get_function(function_name);
  auto &block = function.get_block(block_label);
  debug_assert(instruction_idx < block.instructions.size(),
               "Instruction index out of bounds");
  const auto instruction = block.instructions[instruction_idx];
  debug_assert(instruction.opcode == Opcode::Call, "Instruction is not a call");
  const std::string called_function_name = instruction.funcs[0];
  debug_assert(called_function_name != function_name,
               "Cannot inline a function into itself");

  // Make sure the called function has an expected form: its entry block should
  // appear first, and it should have a single exit block, which appears last in
  // the list of block labels
  const auto &called_function = get_function(called_function_name);
  debug_assert(called_function.block_labels[0] == called_function.entry_label,
               "Called function does not start with its entry block");
  debug_assert(called_function.exiting_blocks ==
                   std::set<std::string>{called_function.block_labels.back()},
               "Called function does not end with its unique exit block");
  debug_assert(instruction.arguments.size() == called_function.arguments.size(),
               "Called function has different number of arguments than call "
               "instruction");

  std::cerr << "Inlining function call to " << called_function_name
            << std::endl;

  const std::string inline_exit_label = function.split_block(
      block_label, instruction_idx, called_function_name + "InlineExit");
  const std::string inline_entry_label =
      function.get_fresh_label(called_function_name + "InlineEntry");

  // Create and map each variable in the called function to a fresh name
  std::set<std::string> current_variables, current_labels;
  for (const auto &block_label : function.block_labels) {
    const auto &block = function.get_block(block_label);
    for (const auto &instruction : block.instructions) {
      for (const auto &argument : instruction.arguments)
        current_variables.insert(argument);
      if (instruction.destination != "")
        current_variables.insert(instruction.destination);
      for (const auto &label : instruction.labels)
        current_labels.insert(label);
    }
  }
  for (const auto &block_label : called_function.block_labels) {
    const auto &block = called_function.get_block(block_label);
    for (const auto &instruction : block.instructions) {
      for (const auto &argument : instruction.arguments)
        current_variables.insert(argument);
      if (instruction.destination != "")
        current_variables.insert(instruction.destination);
      for (const auto &label : instruction.labels)
        current_labels.insert(label);
    }
  }

  std::map<std::string, std::string> renamed_variables;
  std::map<std::string, std::string> renamed_labels;
  const auto get_fresh_variable = [&](const std::string &name) {
    if (renamed_variables.count(name) > 0)
      return renamed_variables.at(name);
    size_t idx = 0;
    while (true) {
      std::string fresh_name = name + "." + std::to_string(idx);
      if (current_variables.count(fresh_name) == 0) {
        current_variables.insert(fresh_name);
        renamed_variables[name] = fresh_name;
        return fresh_name;
      }
      idx++;
    }
  };
  const auto get_fresh_label = [&](const std::string &name) {
    if (renamed_labels.count(name) > 0)
      return renamed_labels.at(name);
    size_t idx = 0;
    while (true) {
      std::string fresh_name = name + "." + std::to_string(idx);
      if (current_labels.count(fresh_name) == 0) {
        current_labels.insert(fresh_name);
        renamed_labels[name] = fresh_name;
        return fresh_name;
      }
      idx++;
    }
  };
  const auto get_renamed_variable = [&](const std::string &name) {
    debug_assert(renamed_variables.count(name) > 0,
                 "Variable " + name + " not renamed");
    return renamed_variables.at(name);
  };
  const auto get_renamed_label = [&](const std::string &name) {
    debug_assert(renamed_labels.count(name) > 0,
                 "Label " + name + " not renamed");
    return renamed_labels.at(name);
  };

  renamed_labels[called_function.entry_label] = inline_entry_label;
  for (const auto &parameter : called_function.arguments)
    get_fresh_variable(parameter.name);
  for (const auto &called_label : called_function.block_labels) {
    const auto &called_block = called_function.get_block(called_label);
    get_fresh_label(called_block.entry_label);
    for (const auto &instruction : called_block.instructions) {
      for (const auto &argument : instruction.arguments)
        get_fresh_variable(argument);
      if (instruction.destination != "")
        get_fresh_variable(instruction.destination);
      for (const auto &label : instruction.labels)
        get_fresh_label(label);
    }
  }

  // Copy over arguments
  block.instructions.pop_back(); // Pop the old jump to the split label
  for (size_t i = 0; i < called_function.arguments.size(); ++i) {
    const auto &parameter = called_function.arguments[i];
    const auto &argument = instruction.arguments[i];
    block.instructions.push_back(Instruction::id(
        get_renamed_variable(parameter.name), argument, parameter.type));
  }
  block.instructions.push_back(Instruction::jmp(inline_entry_label));

  // Determine the return variable
  std::string return_variable = "(unknown)";
  called_function.for_each_instruction([&](const Instruction &instruction) {
    if (instruction.opcode == Opcode::Ret) {
      debug_assert(return_variable == "(unknown)",
                   "Called function has multiple return instructions");
      return_variable = instruction.arguments[0];
    }
  });

  // Add the called function's blocks to the current function
  size_t block_idx = std::find(function.block_labels.begin(),
                               function.block_labels.end(), inline_exit_label) -
                     function.block_labels.begin();
  for (const auto &called_label : called_function.block_labels) {
    auto called_block = called_function.get_block(called_label);
    const std::string new_label = get_renamed_label(called_label);
    called_block.entry_label = new_label;

    // Rename variables and labels
    for (auto &instruction : called_block.instructions) {
      for (auto &argument : instruction.arguments)
        argument = get_renamed_variable(argument);
      for (auto &label : instruction.labels)
        label = get_renamed_label(label);
      if (instruction.destination != "")
        instruction.destination = get_renamed_variable(instruction.destination);
    }

    // Remove the return instruction
    if (called_block.instructions.back().opcode == Opcode::Ret)
      called_block.instructions.back() = Instruction::jmp(inline_exit_label);

    function.blocks.emplace(new_label, called_block);
    function.block_labels.insert(function.block_labels.begin() + block_idx,
                                 new_label);

    block_idx++;
  }

  debug_assert(return_variable != "(unknown)",
               "Called function does not have a return instruction");
  auto &exit_block = function.get_block(inline_exit_label);
  auto &calling_instruction = exit_block.instructions[1];
  debug_assert(calling_instruction.opcode == Opcode::Call &&
                   calling_instruction.funcs[0] == called_function_name,
               "Expected exit block to start with the inlining call");
  calling_instruction = Instruction::id(calling_instruction.destination,
                                        renamed_variables.at(return_variable),
                                        called_function.return_type);
  function.is_graph_dirty = true;
  function.recompute_graph();
}

} // namespace bril
