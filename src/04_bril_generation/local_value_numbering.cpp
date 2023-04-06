
#include "local_value_numbering.hpp"
#include "bril.hpp"
#include "util.hpp"

#include <functional>
#include <optional>

namespace bril {

LocalValueNumber::LocalValueNumber(const Opcode opcode,
                                   const std::vector<size_t> &arguments)
    : opcode(opcode), arguments(arguments) {
  runtime_assert(opcode != Opcode::Const,
                 "Const LVN should use other constructor");
  // Canonicalize arguments of commutative operations
  if (opcode == Opcode::Add || opcode == Opcode::Mul)
    std::sort(this->arguments.begin(), this->arguments.end());
}

LocalValueNumber::LocalValueNumber(const int value)
    : opcode(Opcode::Const), value(value) {}

std::optional<int>
LocalValueTable::fold_constants(const LocalValueNumber &value) const {
  using BinaryFunc = std::function<int(int, int)>;
  const std::map<Opcode, BinaryFunc> foldable_ops = {
      std::make_pair(Opcode::Add, [](int a, int b) { return a + b; }),
      std::make_pair(Opcode::Sub, [](int a, int b) { return a - b; }),
      std::make_pair(Opcode::Mul, [](int a, int b) { return a * b; }),
      std::make_pair(Opcode::Div, [](int a, int b) { return a / b; }),
      std::make_pair(Opcode::Mod, [](int a, int b) { return a % b; }),
      std::make_pair(Opcode::Lt, [](int a, int b) { return a < b; }),
      std::make_pair(Opcode::Le, [](int a, int b) { return a <= b; }),
      std::make_pair(Opcode::Gt, [](int a, int b) { return a > b; }),
      std::make_pair(Opcode::Ge, [](int a, int b) { return a >= b; }),
      std::make_pair(Opcode::Eq, [](int a, int b) { return a == b; }),
      std::make_pair(Opcode::Ne, [](int a, int b) { return a != b; }),
  };
  // func --> func(x, x), if this is a constant expression
  const std::map<Opcode, int> cancellable_ops = {
      std::make_pair(Opcode::Sub, 0), std::make_pair(Opcode::Div, 1),
      std::make_pair(Opcode::Mod, 0), std::make_pair(Opcode::Lt, 0),
      std::make_pair(Opcode::Le, 1),  std::make_pair(Opcode::Gt, 0),
      std::make_pair(Opcode::Ge, 1),  std::make_pair(Opcode::Eq, 1),
      std::make_pair(Opcode::Ne, 0),
  };

  if (value.opcode == Opcode::Const)
    return value.value;
  if (foldable_ops.count(value.opcode) == 0)
    return std::nullopt;
  runtime_assert(value.arguments.size() == 2,
                 "Expected foldable opcode to have two arguments");

  const LocalValueNumber lhs_value = values[value.arguments[0]];
  const LocalValueNumber rhs_value = values[value.arguments[1]];

  const bool lhs_is_const = lhs_value.opcode == Opcode::Const;
  const bool rhs_is_const = rhs_value.opcode == Opcode::Const;
  const int lhs = lhs_value.value;
  const int rhs = rhs_value.value;

  const bool all_constants = lhs_is_const && rhs_is_const;

  if (!all_constants) {
    // There are only a few folding techniques that work if one or more
    // arguments are not constants

    // Cancellation:
    if (cancellable_ops.count(value.opcode) > 0 &&
        value.arguments[0] == value.arguments[1]) {
      return cancellable_ops.at(value.opcode);
    }

    // 0 + x == x
    // 0 * x == 0
    // 1 * x == x
    // 0 / x == 0
    // TODO: Find a convenient way to represent variable transformations
    if (lhs_is_const) {
      if (lhs == 0 &&
          (value.opcode == Opcode::Mul || value.opcode == Opcode::Div ||
           value.opcode == Opcode::Mod))
        return 0;
    }

    // x + 0 == x
    // x - 0 == x
    // x * 0 == 0
    // x * 1 == x
    // x / 0 == BAD
    // x / 1 == x
    // x % 1 == 0
    // x % 0 == BAD
    if (rhs_is_const) {
      if (rhs == 0 && value.opcode == Opcode::Mul)
        return 0;
      if (rhs == 1 && value.opcode == Opcode::Mod)
        return 0;
    }

    return std::nullopt;
  }

  const int result = foldable_ops.at(value.opcode)(lhs, rhs);
  return result;
}

size_t LocalValueTable::query_row(const LocalValueNumber &value) const {
  if (value.opcode == Opcode::Id)
    return value.arguments[0];
  const auto it = std::find(values.begin(), values.end(), value);
  if (it == values.end())
    return NOT_FOUND;
  const size_t idx = it - values.begin();
  return idx;
}

std::string LocalValueTable::canonical_name(const std::string &variable) const {
  const size_t idx = env.at(variable);
  return canonical_variables[idx];
}

std::string LocalValueTable::fresh_name(const std::string &current_name) const {
  static size_t next_idx = 0;
  const size_t idx = next_idx++;
  return "lvn_" + std::to_string(idx) + "_" + current_name;
}

size_t local_value_numbering(Block &block) {
  // Pessimistically assume nothing is possible if we have pointers
  // TODO: More careful analysis
  if (block.uses_pointers())
    return 0;

  // Compute the last indices every destination is written to
  std::map<std::string, size_t> last_write;
  std::set<std::string> read_before_written;
  for (size_t i = 0; i < block.instructions.size(); ++i) {
    const auto &instruction = block.instructions[i];
    const std::string destination = instruction.destination;
    for (const auto &argument : instruction.arguments) {
      if (last_write.count(argument) == 0) {
        read_before_written.insert(argument);
      }
    }
    if (destination != "") {
      last_write[destination] = i;
    }
  }

  LocalValueTable table;
  table.last_write = last_write;

  for (const auto &var : read_before_written) {
    const size_t num = table.values.size();
    const LocalValueNumber value(Opcode::Id, {num});
    table.values.push_back(value);
    table.canonical_variables.push_back(var);
    table.env[var] = num;
  }

  for (size_t i = 0; i < block.instructions.size(); ++i) {
    auto &instruction = block.instructions[i];

    if (instruction.destination == "" || instruction.opcode == Opcode::Call) {
      // If the instruction is an effect operation, or a call, then simply
      // replace the arguments by their canonical variables
      for (auto &argument : instruction.arguments) {
        argument = table.canonical_name(argument);
      }
      continue;
    }

    // Construct value
    std::vector<size_t> arguments;
    for (const auto &argument : instruction.arguments)
      arguments.push_back(table.env.at(argument));
    const LocalValueNumber value =
        (instruction.opcode == Opcode::Const)
            ? LocalValueNumber(instruction.value)
            : LocalValueNumber(instruction.opcode, arguments);
    const size_t idx = table.query_row(value);

    if (idx != table.NOT_FOUND) {
      // If the value is already in the table, then replace the instruction with
      // an id
      const std::string destination = instruction.destination;
      table.env[destination] = idx;
      const bool entry_is_const = table.values[idx].opcode == Opcode::Const;
      if (entry_is_const) {
        const int value = table.values[idx].value;
        instruction = bril::Instruction(destination, value, instruction.type);
      } else {
        const std::string var = table.canonical_variables[idx];
        instruction = bril::Instruction::id(destination, var, instruction.type);
      }
      continue;
    }

    if (instruction.destination != "") {
      const std::string original_destination = instruction.destination;
      const bool dest_overwritten = last_write.at(original_destination) > i;

      const std::string fresh_name =
          dest_overwritten ? table.fresh_name(original_destination)
                           : original_destination;

      const auto folded_result = table.fold_constants(value);

      if (folded_result.has_value()) {
        const LocalValueNumber folded_value(folded_result.value());
        const size_t num = table.values.size();
        instruction = bril::Instruction(fresh_name, folded_result.value(),
                                        instruction.type);
        table.values.push_back(folded_value);
        table.canonical_variables.push_back(fresh_name);
        table.env[original_destination] = num;
      } else {
        const size_t num = table.values.size();
        instruction.destination = fresh_name;
        table.values.push_back(value);
        table.canonical_variables.push_back(fresh_name);
        table.env[original_destination] = num;
      }
    }

    // Canonicalize all the arguments as well
    for (auto &argument : instruction.arguments) {
      argument = table.canonical_name(argument);
    }
  }

  return 0;
}

/*

lvn(block):
  env (var2num): a mapping from variables to their row index
  value_to_num : a mapping from value to their row index

  canonical_variables : a mapping from row index to canonical variable
  constants     : a mapping from row index to constant values


  1. For each variable read before being written to,
    a) Add them to the table, and env[var] = num
    b) canonical_variables[num] = var

  2. For each instruction,
    1. Convert all the arguments into row numbers
    2. For each non-call value operation,
      a) Construct a value using the opcode and the converted arguments
      b) If the value is already in the table, (do id-folding here)
        - env[dest] = num
        - Replace the instruction with either a copy or a constant, depending on
          whether the num is in constants or values
      c) Otherwise, if the result produces a result, give it a number:
        - Add the destination to table (say, at num)
        - If the opcode is const, add it to the constant table
        - If the instruction is not the last write, create a fresh name
        - Let var be the unused destination name
        - canonical_variables[num] = var
        - Set the instruction's destination to var
        - If the value is valid, then try folding it and place the result into
          the constant table, otherwise place it into the variable table
      d) In all cases, update the argument variables to canonical names

*/

} // namespace bril
