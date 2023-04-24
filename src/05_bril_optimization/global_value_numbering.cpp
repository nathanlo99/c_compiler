
#include "global_value_numbering.hpp"
#include <optional>

namespace bril {

GVNValue GVNTable::create_value(const Instruction &instruction) const {
  if (instruction.opcode == Opcode::Const)
    return GVNValue(instruction.value, instruction.type);
  std::vector<size_t> arguments;
  arguments.reserve(instruction.arguments.size());
  for (const auto &argument : instruction.arguments) {
    arguments.push_back(query_variable(argument));
  }
  const auto value = GVNValue(instruction.opcode, arguments, instruction.labels,
                              instruction.type);
  return simplify(value);
}

bool GVNTable::is_associative(const Opcode opcode) {
  return opcode == Opcode::Add || opcode == Opcode::Mul;
}

bool GVNTable::is_commutative(const Opcode opcode) {
  return opcode == Opcode::Add || opcode == Opcode::Mul ||
         opcode == Opcode::Eq || opcode == Opcode::Ne;
}

std::pair<size_t, size_t> GVNTable::get_complexity_key(const size_t idx) const {
  const size_t complexity = (get_opcode(idx) == Opcode::Const) ? 0 : 1;
  return std::make_pair(complexity, idx);
}

Opcode GVNTable::get_opcode(const size_t idx) const {
  debug_assert(idx < expressions.size(), "Invalid index");
  return expressions[idx].opcode;
}

std::optional<GVNValue> GVNTable::simplify_binary(const Type type,
                                                  const Opcode opcode,
                                                  const size_t lhs,
                                                  const size_t rhs) const {
  // Assume the operands are already simplified, and their order is
  // canonicalized so the rhs is always the less complex one

  // Constant folding
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
  if (foldable_ops.count(opcode) == 0)
    return std::nullopt;

  const GVNValue &lhs_value = expressions[lhs];
  const GVNValue &rhs_value = expressions[rhs];
  const bool lhs_is_const = lhs_value.opcode == Opcode::Const;
  const bool rhs_is_const = rhs_value.opcode == Opcode::Const;
  const int lhs_integer = lhs_value.value;
  const int rhs_integer = rhs_value.value;

  const bool all_constants = lhs_is_const && rhs_is_const;

  if (!all_constants) {
    // If the two arguments are the same, we might be able to simplify
    if (cancellable_ops.count(opcode) > 0 && lhs == rhs) {
      const int result = cancellable_ops.at(opcode);
      return GVNValue(result, type);
    }

    std::map<Opcode, Opcode> reverse_operation = {
        std::make_pair(Opcode::Add, Opcode::Sub),
        std::make_pair(Opcode::Sub, Opcode::Add),
        // NOTE: We don't do this for multiplication since (a / b) * b != a
        // But, (a * b) / b == a
        std::make_pair(Opcode::Div, Opcode::Mul),
    };
    // (a OP b) OP' b --> a      if OP and OP' are inverses
    const auto reverse_it = reverse_operation.find(opcode);
    if (reverse_it != reverse_operation.end()) {
      const Opcode reverse_opcode = reverse_it->second;
      if (lhs_value.opcode == reverse_opcode && lhs_value.arguments[1] == rhs) {
        return expressions[lhs_value.arguments[0]];
      }
    }

    // (a * b) % b == 0
    if (opcode == Opcode::Mod && lhs_value.opcode == Opcode::Mul &&
        lhs_value.arguments[1] == rhs) {
      return GVNValue(0, type);
    }

    // x + 0 == x
    // x - 0 == x
    // x * 0 == 0
    // x * 1 == x
    // x * 2 == x + x
    // x * -1 == 0 - x (TODO)
    // x / 1 == x
    // x % 1 == 0
    if (rhs_is_const) {
      if (rhs_integer == 0 && opcode == Opcode::Add)
        return lhs_value;
      if (rhs_integer == 0 && opcode == Opcode::Sub)
        return lhs_value;
      if (rhs_integer == 0 && opcode == Opcode::Mul)
        return GVNValue(0, type);
      if (rhs_integer == 1 && opcode == Opcode::Mul)
        return lhs_value;
      if (rhs == 1 && opcode == Opcode::Div)
        return lhs_value;
      if (rhs == 1 && opcode == Opcode::Mod)
        return GVNValue(0, type);
    }
    return std::nullopt;
  }

  const auto result = foldable_ops.at(opcode)(lhs_integer, rhs_integer);
  if (!result.has_value())
    return std::nullopt;
  return GVNValue(result.value(), type);
}

GVNValue GVNTable::simplify(const GVNValue &value) const {
  // Simple cases
  if (value.opcode == Opcode::Id)
    return expressions[value.arguments[0]];
  if (value.opcode == Opcode::Const)
    return value;

  // Phi simplification
  if (value.opcode == Opcode::Phi) {
    // If all the arguments are the same, we can just use that
    const std::set<size_t> unique_arguments(value.arguments.begin(),
                                            value.arguments.end());
    if (unique_arguments.size() == 1)
      return expressions[*unique_arguments.begin()];
    return value;
  }

  if (value.arguments.size() != 2)
    return value;

  GVNValue result = value;
  do {
    const Opcode opcode = result.opcode;

    // If the operation is commutative, canonicalize the arguments
    if (is_commutative(opcode) && get_complexity_key(result.arguments[0]) <
                                      get_complexity_key(result.arguments[1])) {
      std::swap(result.arguments[0], result.arguments[1]);
    }

    // Try constant folding
    if (const auto folded_result =
            simplify_binary(result.type, result.opcode, result.arguments[0],
                            result.arguments[1]);
        folded_result.has_value())
      return folded_result.value();

    return result;
  } while (true);
}

struct GVNPhiValue {
  std::vector<std::string> arguments;
  std::vector<std::string> labels;

  GVNPhiValue(const std::vector<std::string> &arguments,
              const std::vector<std::string> &labels)
      : arguments(arguments), labels(labels) {
    // Sort the labels and maintain the same order for the arguments
    std::vector<std::pair<std::string, std::string>> pairs;
    pairs.reserve(arguments.size());
    for (size_t i = 0; i < arguments.size(); ++i)
      pairs.emplace_back(labels[i], arguments[i]);
    std::sort(pairs.begin(), pairs.end());
    for (size_t i = 0; i < arguments.size(); ++i) {
      this->arguments[i] = pairs[i].second;
      this->labels[i] = pairs[i].first;
    }
  }

  bool operator==(const GVNPhiValue &other) const {
    return arguments == other.arguments && labels == other.labels;
  }
};

void GlobalValueNumberingPass::process_block(const std::string &label) {
  auto &block = function.get_block(label);
  // std::cerr << "Processing block " << label << ":" << std::endl;

  const GVNTable old_table = table;

  // First, handle the phi instructions separately
  std::vector<GVNPhiValue> phi_values;
  std::vector<std::string> phi_variables;
  for (auto &instruction : block.instructions) {
    if (instruction.opcode != Opcode::Phi)
      continue;
    const auto destination = instruction.destination;
    table.insert_axiom(destination, instruction.type);
    std::vector<std::string> arguments;
    arguments.reserve(instruction.arguments.size());
    std::set<std::string> argument_set;
    for (const auto &argument : instruction.arguments) {
      const auto it = table.variable_to_value_number.find(argument);
      const std::string canonical_argument =
          it != table.variable_to_value_number.end()
              ? table.canonical_variables[it->second]
              : argument;
      arguments.push_back(canonical_argument);
      argument_set.insert(canonical_argument);
    }
    if (argument_set.size() == 1) {
      instruction =
          Instruction::id(destination, *argument_set.begin(), instruction.type);
      continue;
    }

    const GVNPhiValue value(arguments, instruction.labels);
    const auto it = std::find(phi_values.begin(), phi_values.end(), value);
    if (it == phi_values.end()) {
      phi_values.push_back(value);
      phi_variables.push_back(destination);
      continue;
    }

    const size_t idx = it - phi_values.begin();
    const std::string canonical_variable = phi_variables[idx];
    instruction =
        Instruction::id(destination, canonical_variable, instruction.type);
  }

  for (auto &instruction : block.instructions) {
    const auto destination = instruction.destination;
    if (instruction.opcode == Opcode::Phi)
      continue;

    if (instruction.opcode == Opcode::Call) {
      for (auto &argument : instruction.arguments) {
        argument = table.canonical_variables[table.query_variable(argument)];
      }
      table.insert_axiom(destination, instruction.type);
      continue;
    }

    if (destination == "") {
      // This is a pure instruction, so just canonicalize the arguments
      for (auto &argument : instruction.arguments) {
        argument = table.canonical_variables[table.query_variable(argument)];
      }

      if (instruction.opcode == Opcode::Br) {
        const auto &cond_expr =
            table.expressions[table.query_variable(instruction.arguments[0])];
        if (cond_expr.opcode != Opcode::Const)
          continue;
        const bool cond = cond_expr.value != 0;
        const auto &target = instruction.labels[cond ? 0 : 1];
        instruction = Instruction::jmp(target);
        function.is_graph_dirty = true;
      }

      continue;
    }

    const auto value = table.create_value(instruction);
    const size_t value_number = table.query_or_insert(destination, value);
    // std::cerr << "Value number for " << destination << " with value " <<
    // value << " is " << value_number << std::endl;

    // If the value was already present, replace it with a copy
    if (value_number != table.expressions.size() - 1) {
      instruction =
          Instruction::id(destination, table.canonical_variables[value_number],
                          instruction.type);
    } else {
      instruction = table.value_to_instruction(destination, value);
    }
  }

  for (const auto &successor : block.outgoing_blocks) {
    auto &succ = function.get_block(successor);
    for (auto &phi_instruction : succ.instructions) {
      if (phi_instruction.opcode != Opcode::Phi)
        continue;
      const auto it = std::find(phi_instruction.labels.begin(),
                                phi_instruction.labels.end(), label);
      if (it == phi_instruction.labels.end())
        continue;
      const size_t idx = it - phi_instruction.labels.begin();
      const auto argument = phi_instruction.arguments[idx];
      const auto value_number = table.query_variable(argument);
      phi_instruction.arguments[idx] = table.canonical_variables[value_number];
    }
  }

  for (const auto &other_label : function.block_labels) {
    const auto immediate_dominator = function.immediate_dominator(other_label);
    if (other_label != block.entry_label &&
        block.entry_label == immediate_dominator) {
      process_block(other_label);
    }
  }

  table = old_table;
}

} // namespace bril
