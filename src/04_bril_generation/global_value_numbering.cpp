
#include "global_value_numbering.hpp"

namespace bril {

GVNValue GVNTable::create_value(const Instruction &instruction) const {
  if (instruction.opcode == Opcode::Const)
    return GVNValue(instruction.value, instruction.type);
  std::vector<size_t> arguments;
  arguments.reserve(instruction.arguments.size());
  for (const auto &argument : instruction.arguments) {
    const size_t row_idx = query_variable(argument);
    arguments.push_back(row_idx);
  }
  return GVNValue(instruction.opcode, arguments, instruction.labels,
                  instruction.type);
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

    // If there is only one argument, we can just use that
    if (value.arguments.size() == 1)
      return expressions[value.arguments[0]];

    return value;
  }

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
  if (foldable_ops.count(value.opcode) == 0)
    return value;
  runtime_assert(value.arguments.size() == 2, "Expected binary operation");

  const GVNValue &lhs_value = expressions[value.arguments[0]];
  const GVNValue &rhs_value = expressions[value.arguments[1]];
  const bool lhs_is_const = lhs_value.opcode == Opcode::Const;
  const bool rhs_is_const = rhs_value.opcode == Opcode::Const;
  const int lhs = lhs_value.value;
  const int rhs = rhs_value.value;

  const bool all_constants = lhs_is_const && rhs_is_const;

  if (!all_constants) {
    if (cancellable_ops.count(value.opcode) > 0 &&
        value.arguments[0] == value.arguments[1]) {
      const int result = cancellable_ops.at(value.opcode);
      return GVNValue(result, value.type);
    }
    // TODO: there are more simplifications
    return value;
  }

  const auto result = foldable_ops.at(value.opcode)(lhs, rhs);
  if (!result.has_value())
    return value;
  return GVNValue(result.value(), value.type);
}

void GlobalValueNumberingPass::process_block(const std::string &label) {
  auto &block = cfg.get_block(label);
  std::cout << "Processing block " << label << ":" << std::endl;

  for (auto &instruction : block.instructions) {
    const auto destination = instruction.destination;
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
        cfg.is_graph_dirty = true;
      }

      continue;
    }

    const auto value = table.create_value(instruction);
    const size_t value_number = table.query_or_insert(destination, value);
    std::cerr << "Value number for " << destination << " with value " << value
              << " is " << value_number << std::endl;

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
    auto &succ = cfg.get_block(successor);
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

  for (const auto &other_label : cfg.block_labels) {
    if (other_label != block.entry_label &&
        cfg._immediately_dominates(block.entry_label, other_label)) {
      process_block(other_label);
    }
  }
}

} // namespace bril
