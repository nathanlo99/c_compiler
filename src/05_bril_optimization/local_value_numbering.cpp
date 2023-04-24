
#include "local_value_numbering.hpp"
#include "bril.hpp"
#include "util.hpp"

#include <functional>
#include <optional>

namespace bril {

LocalValueNumber::LocalValueNumber(const Opcode opcode,
                                   const std::vector<size_t> &arguments,
                                   const Type type)
    : opcode(opcode), arguments(arguments), type(type) {
  debug_assert(opcode != Opcode::Const,
               "Const LVN should use other constructor");
  // Operations OP for which [a OP b === b OP a], we canonicalize the arguments
  static const std::set<Opcode> commutative_operations = {
      Opcode::Add,
      Opcode::Mul,
      Opcode::Eq,
      Opcode::Ne,
  };
  // Operations OP for which there is a canonical OP' for which
  // [a OP b = b OP' a], we canonicalize the opcode
  static const std::map<Opcode, Opcode> switch_order = {
      std::make_pair(Opcode::Gt, Opcode::Lt),
      std::make_pair(Opcode::Ge, Opcode::Le),
  };

  // Canonicalize arguments of commutative operations
  if (commutative_operations.count(opcode) > 0) {
    debug_assert(this->arguments.size() == 2,
                 "Expected binary expression in commutative operation");
    std::sort(this->arguments.begin(), this->arguments.end());
  } else if (switch_order.count(opcode) > 0) {
    debug_assert(this->arguments.size() == 2,
                 "Expected binary expression in switchable LVN");
    const auto switched_opcode = switch_order.at(opcode);
    this->opcode = switched_opcode;
    std::swap(this->arguments[0], this->arguments[1]);
  }
}

LocalValueNumber::LocalValueNumber(const int value, const Type type)
    : opcode(Opcode::Const), value(value), type(type) {}

std::optional<int>
LocalValueTable::fold_constants(const LocalValueNumber &value) const {
  const static std::set<Type> foldable_types = {Type::Int};
  if (foldable_types.count(value.type) == 0)
    return std::nullopt;
  using BinaryFunc = std::function<std::optional<int>(int, int)>;
  const std::map<Opcode, BinaryFunc> foldable_ops = {
      std::make_pair(Opcode::Add, [](int a, int b) { return a + b; }),
      std::make_pair(Opcode::Sub, [](int a, int b) { return a - b; }),
      std::make_pair(Opcode::Mul, [](int a, int b) { return a * b; }),
      std::make_pair(Opcode::Div,
                     [](int a, int b) -> std::optional<int> {
                       return (b == 0) ? std::nullopt
                                       : std::make_optional(a / b);
                     }),
      std::make_pair(Opcode::Mod,
                     [](int a, int b) -> std::optional<int> {
                       return (b == 0) ? std::nullopt
                                       : std::make_optional(a % b);
                     }),
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
  debug_assert(value.arguments.size() == 2,
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

  return foldable_ops.at(value.opcode)(lhs, rhs);
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
  debug_assert(env.count(variable) > 0,
               "Variable {} was not present in the table", variable);
  const size_t idx = env.at(variable);
  return canonical_variables[idx];
}

std::string LocalValueTable::fresh_name(const std::string &current_name) const {
  static size_t next_idx = 0;
  const size_t idx = next_idx++;
  return "lvn_" + std::to_string(idx) + "_" + current_name;
}

size_t local_value_numbering(ControlFlowGraph &graph, Block &block) {
  if (block.has_loads_or_stores())
    return 0;

  LocalValueTable table;

  // Compute the last indices every destination is written to
  std::set<std::string> read_before_written;
  std::map<std::string, Type> types;
  for (size_t i = 0; i < block.instructions.size(); ++i) {
    const auto &instruction = block.instructions[i];
    const std::string destination = instruction.destination;

    for (const auto &argument : instruction.arguments) {
      if (table.last_write.count(argument) == 0) {
        read_before_written.insert(argument);
      }
    }
    if (destination != "") {
      table.last_write[destination] = i;
      types[destination] = instruction.type;
    }
  }

  for (const auto &var : read_before_written) {
    const Type type = types[var];
    const size_t num = table.values.size();
    const LocalValueNumber value(Opcode::Id, {num}, type);
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

      // The destinations for function calls still need to be inserted as
      // standalone values in the table, because they may still appear as
      // arguments and make us cry
      if (instruction.opcode == Opcode::Call) {
        const std::string destination = instruction.destination;
        const Type type = instruction.type;
        const size_t num = table.values.size();
        const LocalValueNumber value(Opcode::Id, {num}, type);
        table.values.push_back(value);
        table.canonical_variables.push_back(destination);
        table.env[destination] = num;
      }

      // If the instruction is a branch, and the arguments are constants, then
      // resolve the branch
      if (instruction.opcode == Opcode::Br) {
        if (instruction.labels[0] == instruction.labels[1]) {
          std::cerr << "LVN: Resolving the branch " << instruction
                    << " since the targets are the same" << std::endl;
          instruction = bril::Instruction::jmp(instruction.labels[0]);
          graph.is_graph_dirty = true;
          continue;
        }

        // Otherwise, the branch can be resolved if the condition is a constant
        const std::string cond = instruction.arguments[0];
        const size_t cond_idx = table.env.at(cond);
        const LocalValueNumber cond_value = table.values[cond_idx];
        if (cond_value.opcode != Opcode::Const)
          continue;

        const bool cond_value_bool = cond_value.value != 0;
        std::cerr << "LVN: Resolving the branch " << instruction
                  << " since the condition is always "
                  << (cond_value_bool ? "true" : "false") << std::endl;
        debug_assert(instruction.labels.size() == 2,
                     "Branch instruction should have 2 labels");
        const std::string target = instruction.labels[cond_value_bool ? 0 : 1];

        instruction = bril::Instruction::jmp(target);
        graph.is_graph_dirty = true;
      }

      continue;
    }

    // Construct value
    std::vector<size_t> arguments;
    for (const auto &argument : instruction.arguments) {
      debug_assert(table.env.count(argument) > 0,
                   "Argument {} not found in env", argument);
      arguments.push_back(table.env.at(argument));
    }
    const LocalValueNumber value =
        instruction.opcode == Opcode::Const
            ? LocalValueNumber(instruction.value, instruction.type)
            : LocalValueNumber(instruction.opcode, arguments, instruction.type);
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
      debug_assert(table.last_write.count(original_destination) > 0,
                   "Destination {} not in last_write", original_destination);
      const bool dest_overwritten =
          table.last_write.at(original_destination) > i;

      const std::string fresh_name =
          dest_overwritten ? table.fresh_name(original_destination)
                           : original_destination;

      const auto folded_result = table.fold_constants(value);

      if (folded_result.has_value()) {
        const LocalValueNumber folded_value(folded_result.value(), Type::Int);
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

} // namespace bril
