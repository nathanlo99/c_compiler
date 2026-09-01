
#include "global_value_numbering.hpp"
#include <limits>
#include <optional>

namespace bril {

GVNValue GVNTable::create_value(const Instruction &instruction) const {
  if (instruction.opcode == Opcode::Const)
    return GVNValue(instruction.value, instruction.type);
  std::vector<size_t> arguments;
  arguments.reserve(instruction.arguments.size());
  for (const auto &argument : instruction.arguments) {
    arguments.push_back(query_variable(argument).idx);
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
                                                  const size_t lhs_idx,
                                                  const size_t rhs_idx) const {
  // Assume the operands are already simplified, and their order is
  // canonicalized so the rhs is always the less complex one

  // Constant folding
  using BinaryFunc = std::function<std::optional<int>(int, int)>;
  const std::unordered_map<Opcode, BinaryFunc> foldable_ops = {
      // Signed overflow is UB (references/spec.txt); still needs *some*
      // fixed value when folding one, so: unsigned-cast wraparound, since
      // plain signed overflow is UB in our own C++ too.
      std::make_pair(Opcode::Add,
                     [](int a, int b) {
                       return static_cast<int>(static_cast<unsigned>(a) +
                                               static_cast<unsigned>(b));
                     }),
      std::make_pair(Opcode::Sub,
                     [](int a, int b) {
                       return static_cast<int>(static_cast<unsigned>(a) -
                                               static_cast<unsigned>(b));
                     }),
      std::make_pair(Opcode::Mul,
                     [](int a, int b) {
                       return static_cast<int>(static_cast<unsigned>(a) *
                                               static_cast<unsigned>(b));
                     }),
      std::make_pair(
          Opcode::Div,
          [](int a, int b) -> std::optional<int> {
            // b == 0: division by zero. a == INT_MIN && b == -1: the one
            // case INT_MIN/-1 doesn't fit in int either -- also UB, and
            // undefined on real MIPS hardware too. Leave both unfolded.
            if (b == 0 || (a == std::numeric_limits<int>::min() && b == -1))
              return std::nullopt;
            return a / b;
          }),
      std::make_pair(
          Opcode::Mod,
          [](int a, int b) -> std::optional<int> {
            if (b == 0 || (a == std::numeric_limits<int>::min() && b == -1))
              return std::nullopt;
            return a % b;
          }),
      std::make_pair(Opcode::Lt, [](int a, int b) { return a < b; }),
      std::make_pair(Opcode::Le, [](int a, int b) { return a <= b; }),
      std::make_pair(Opcode::Gt, [](int a, int b) { return a > b; }),
      std::make_pair(Opcode::Ge, [](int a, int b) { return a >= b; }),
      std::make_pair(Opcode::Eq, [](int a, int b) { return a == b; }),
      std::make_pair(Opcode::Ne, [](int a, int b) { return a != b; }),
  };
  // func --> func(x, x), if this is a constant expression
  const std::unordered_map<Opcode, int> cancellable_ops = {
      std::make_pair(Opcode::Sub, 0), std::make_pair(Opcode::Div, 1),
      std::make_pair(Opcode::Mod, 0), std::make_pair(Opcode::Lt, 0),
      std::make_pair(Opcode::Le, 1),  std::make_pair(Opcode::Gt, 0),
      std::make_pair(Opcode::Ge, 1),  std::make_pair(Opcode::Eq, 1),
      std::make_pair(Opcode::Ne, 0),
  };
  if (!foldable_ops.contains(opcode))
    return std::nullopt;

  const GVNValue &lhs_value = expressions[lhs_idx];
  const GVNValue &rhs_value = expressions[rhs_idx];
  const bool lhs_is_const = lhs_value.opcode == Opcode::Const;
  const bool rhs_is_const = rhs_value.opcode == Opcode::Const;
  const int lhs_integer = lhs_value.value;
  const int rhs_integer = rhs_value.value;

  const bool all_constants = lhs_is_const && rhs_is_const;

  if (!all_constants) {
    // If the two arguments are the same, we might be able to simplify
    if (cancellable_ops.contains(opcode) && lhs_idx == rhs_idx) {
      const int result = cancellable_ops.at(opcode);
      return GVNValue(result, type);
    }

    std::unordered_map<Opcode, Opcode> reverse_operation = {
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
      if (lhs_value.opcode == reverse_opcode &&
          lhs_value.arguments[1] == rhs_idx) {
        return expressions[lhs_value.arguments[0]];
      }
      // (b OP a) OP' b --> a      if OP is also commutative
      if (is_commutative(reverse_opcode) && lhs_value.opcode == reverse_opcode &&
          lhs_value.arguments[0] == rhs_idx) {
        return expressions[lhs_value.arguments[1]];
      }
    }

    // (a * b) % b == 0
    if (opcode == Opcode::Mod && lhs_value.opcode == Opcode::Mul &&
        (lhs_value.arguments[1] == rhs_idx || lhs_value.arguments[0] == rhs_idx)) {
      return GVNValue(0, type);
    }

    // (x + x) / 2 == x, (x + x) % 2 == 0 -- recovers the multiplicative
    // structure the x*2==x+x rewrite below already erased. Without this,
    // `b*2/2` folds to `b+b` then gets stuck: the two rules never see each
    // other, since by the time this division is simplified, `b*2` has
    // already been permanently rewritten to `b+b` wherever it was computed.
    if (rhs_is_const && rhs_integer == 2 && lhs_value.opcode == Opcode::Add &&
        lhs_value.arguments[0] == lhs_value.arguments[1]) {
      if (opcode == Opcode::Div)
        return expressions[lhs_value.arguments[0]];
      if (opcode == Opcode::Mod)
        return GVNValue(0, type);
    }

    // x + 0 == x
    // x - 0 == x
    // x * 0 == 0
    // x * 1 == x
    // x * 2 == x + x
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
      if (rhs_integer == 2 && opcode == Opcode::Mul)
        return GVNValue(Opcode::Add, {lhs_idx, lhs_idx}, {}, type);
      if (rhs_integer == 1 && opcode == Opcode::Div)
        return lhs_value;
      if (rhs_integer == 1 && opcode == Opcode::Mod)
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
    const std::unordered_set<size_t> unique_arguments(value.arguments.begin(),
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

std::optional<std::vector<Instruction>>
GVNTable::simplify_with_synthesis(const Instruction &instruction) const {
  // x * -1 == 0 - x
  if (instruction.opcode != Opcode::Mul)
    return std::nullopt;
  debug_assert(instruction.arguments.size() == 2, "Mul expects 2 arguments");

  // Canonicalized, so rhs is always the -1 if either operand is (matches
  // simplify_binary's own convention). all-constant case (rhs is -1 *and*
  // lhs is also const) is excluded -- let simplify_binary fold that
  // directly instead of routing it through here.
  const auto [lhs, rhs] = sort_commutative_operands(instruction);
  if (lhs.value.opcode == Opcode::Const || !rhs.value.is_constant(-1))
    return std::nullopt;

  // Placeholder names, wired up within this list only -- the caller assigns
  // real (interned) names before anything is written to the block.
  return std::vector<Instruction>{
      Instruction::constant("_gvn0", 0, instruction.type),
      Instruction::sub("_gvn1", "_gvn0", lhs.canonical_name),
  };
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

  bool operator==(const GVNPhiValue &other) const = default;
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
    std::unordered_set<std::string> argument_set;
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

  for (size_t i = 0; i < block.instructions.size(); ++i) {
    // Copy, not reference: synthesis below may insert into block.instructions
    // ahead of this position, which can reallocate and invalidate a
    // reference obtained before that happened.
    Instruction instruction = block.instructions[i];
    const auto destination = instruction.destination;
    if (instruction.opcode == Opcode::Phi)
      continue;

    if (instruction.opcode == Opcode::Call) {
      for (auto &argument : instruction.arguments) {
        argument = table.query_variable(argument).canonical_name;
      }
      table.insert_axiom(destination, instruction.type);
      block.instructions[i] = instruction;
      continue;
    }

    if (destination == "") {
      // This is a pure instruction, so just canonicalize the arguments
      for (auto &argument : instruction.arguments) {
        argument = table.query_variable(argument).canonical_name;
      }

      if (instruction.opcode == Opcode::Br) {
        const auto &cond_expr = table.query_variable(instruction.arguments[0]).value;
        if (cond_expr.opcode == Opcode::Const) {
          const bool cond = cond_expr.value != 0;
          const auto &target = instruction.labels[cond ? 0 : 1];
          instruction = Instruction::jmp(target);
          function.is_graph_dirty = true;
        }
      }

      block.instructions[i] = instruction;
      continue;
    }

    if (const auto synthesized = table.simplify_with_synthesis(instruction);
        synthesized.has_value()) {
      // Feed each synthesized instruction through the same value-numbering
      // pipeline as any real instruction, substituting this rewrite's own
      // placeholder names for their real (interned or freshly-materialized)
      // ones as we go.
      std::unordered_map<std::string, std::string> local_rename;
      const auto &candidates = synthesized.value();
      for (size_t k = 0; k < candidates.size(); ++k) {
        Instruction candidate = candidates[k];
        for (auto &argument : candidate.arguments) {
          if (const auto it = local_rename.find(argument);
              it != local_rename.end())
            argument = it->second;
        }

        const bool is_last = (k + 1 == candidates.size());
        if (!is_last) {
          const auto value = table.create_value(candidate);
          const size_t existing = table.query(value);
          if (existing != GVNTable::NOT_FOUND) {
            local_rename[candidate.destination] =
                table.canonical_variables[existing];
            continue; // dedup hit -- nothing to materialize
          }
          const std::string real_name =
              fmt::format("gvn_{}", table.expressions.size());
          table.query_or_insert(real_name, value);
          local_rename[candidate.destination] = real_name;
          candidate.destination = real_name;
          block.instructions.insert(block.instructions.begin() + i, candidate);
          ++i;
        } else {
          candidate.destination = destination; // always the original name
          const auto value = table.create_value(candidate);
          const auto [value_number, is_new] = table.query_or_insert(destination, value);
          if (!is_new) {
            block.instructions[i] = Instruction::id(
                destination, table.canonical_variables[value_number],
                candidate.type);
          } else {
            block.instructions[i] = table.value_to_instruction(destination, value);
          }
        }
      }
      continue;
    }

    const auto value = table.create_value(instruction);
    const auto [value_number, is_new] = table.query_or_insert(destination, value);

    // If the value was already present, replace it with a copy
    if (!is_new) {
      instruction =
          Instruction::id(destination, table.canonical_variables[value_number],
                          instruction.type);
    } else {
      instruction = table.value_to_instruction(destination, value);
    }
    block.instructions[i] = instruction;
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
      phi_instruction.arguments[idx] = table.query_variable(argument).canonical_name;
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
