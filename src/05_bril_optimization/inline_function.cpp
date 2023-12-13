

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
  const auto call_instruction = block.instructions[instruction_idx];
  debug_assert(call_instruction.opcode == Opcode::Call,
               "Instruction is not a call");
  const std::string called_function_name = call_instruction.funcs[0];
  std::cerr << "Attempting to inline call to " << called_function_name << " in "
            << function_name << std::endl;

  debug_assert(called_function_name != function_name,
               "Cannot inline a function into itself");

  // Make sure the called function's entry block appears first
  const auto &called_function = get_function(called_function_name);
  debug_assert(called_function.block_labels[0] == called_function.entry_label,
               "Called function does not start with its entry block");
  debug_assert(call_instruction.arguments.size() ==
                   called_function.arguments.size(),
               "Called function has different number of arguments than call "
               "instruction");

  std::cerr << "Inlining call to " << called_function_name << " in "
            << function_name << std::endl;

  const std::string inline_exit_label = function.split_block(
      block_label, instruction_idx, called_function_name + "InlineExit");
  const std::string inline_entry_label =
      function.get_fresh_label(called_function_name + "InlineEntry");

  // Create and map each variable in the called function to a fresh name
  std::unordered_set<std::string> current_variables, current_labels;
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

  std::unordered_map<std::string, std::string> renamed_variables;
  std::unordered_map<std::string, std::string> renamed_labels;
  const auto get_fresh_variable = [&](const std::string &name) {
    if (renamed_variables.count(name) > 0)
      return renamed_variables.at(name);
    size_t idx = 0;
    while (true) {
      const std::string fresh_name = name + "." + std::to_string(idx);
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
    debug_assert(renamed_variables.count(name) > 0, "Variable {} not renamed",
                 name);
    return renamed_variables.at(name);
  };
  const auto get_renamed_label = [&](const std::string &name) {
    debug_assert(renamed_labels.count(name) > 0, "Label {} not renamed", name);
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
    const auto &argument = call_instruction.arguments[i];
    block.instructions.push_back(Instruction::id(
        get_renamed_variable(parameter.name), argument, parameter.type));
  }
  block.instructions.push_back(Instruction::jmp(inline_entry_label));

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

    // Replace the return instruction with an assignment to the return variable,
    // then jump to the exit label
    auto &last_instruction = called_block.instructions.back();
    if (last_instruction.opcode == Opcode::Ret) {
      const std::string returned_value = last_instruction.arguments[0];
      last_instruction =
          Instruction::id(call_instruction.destination, returned_value,
                          called_function.return_type);
      called_block.instructions.push_back(Instruction::jmp(inline_exit_label));
    }

    function.blocks.emplace(new_label, called_block);
    function.block_labels.insert(function.block_labels.begin() + block_idx,
                                 new_label);

    block_idx++;
  }

  auto &exit_block = function.get_block(inline_exit_label);
  auto &calling_instruction = exit_block.instructions[1];
  debug_assert(calling_instruction.opcode == Opcode::Call &&
                   calling_instruction.funcs[0] == called_function_name,
               "Expected exit block to start with the inlining call");
  exit_block.instructions.erase(exit_block.instructions.begin() + 1);
  function.recompute_graph(true);
}

// Given a function and a function to inline, inline all calls to the second
// function in the first
bool Program::inline_function(const std::string &function_name,
                              const std::string &called_function_name) {
  if (function_name == called_function_name)
    return false;
  bool result = false;
  auto &function = get_function(function_name);
  while (true) {
    bool changed = false;
    for (const auto &label : function.block_labels) {
      auto &block = function.get_block(label);
      for (size_t idx = 0; idx < block.instructions.size(); ++idx) {
        auto &instruction = block.instructions[idx];
        if (instruction.opcode == Opcode::Call &&
            instruction.funcs[0] == called_function_name) {
          inline_function_call(function_name, label, idx);
          changed = true;
          break;
        }
      }
      if (changed)
        break;
    }
    if (!changed)
      break;
    result = true;
  }
  return result;
}

} // namespace bril
