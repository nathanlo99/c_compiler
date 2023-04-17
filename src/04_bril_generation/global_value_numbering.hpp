
#pragma once

#include "bril.hpp"
#include "util.hpp"
#include <algorithm>

namespace bril {

struct GVNValue {
  Opcode opcode;
  int value = 42069;
  std::vector<size_t> arguments;
  std::vector<std::string> labels;

  Type type;

  GVNValue(const Opcode _opcode, const std::vector<size_t> &_arguments,
           const std::vector<std::string> &labels, const Type type)
      : opcode(_opcode), arguments(_arguments), labels(labels), type(type) {
    runtime_assert(opcode != Opcode::Const,
                   "Constant GVNValue should use other constructor");
    runtime_assert(opcode != Opcode::Call, "Cannot create GVNValue for call");

    if (opcode != Opcode::Phi)
      runtime_assert(labels.empty(), "Labels should only be used for phis");

    // If the operation is commutative, sort the arguments
    static const std::set<Opcode> commutative_ops = {
        Opcode::Add,
        Opcode::Mul,
        Opcode::Eq,
        Opcode::Ne,
    };
    static const std::map<Opcode, Opcode> switchable_ops = {
        std::make_pair(Opcode::Gt, Opcode::Lt),
        std::make_pair(Opcode::Ge, Opcode::Le),
    };

    if (commutative_ops.count(opcode) > 0) {
      runtime_assert(arguments.size() == 2, "Expected binary operation");
      std::sort(arguments.begin(), arguments.end());
    } else if (switchable_ops.count(opcode) > 0) {
      runtime_assert(arguments.size() == 2, "Expected binary operation");
      std::swap(arguments[0], arguments[1]);
      opcode = switchable_ops.at(opcode);
    } else if (opcode == Opcode::Phi) {
      runtime_assert(arguments.size() == labels.size(),
                     "Arguments and labels should be the same size");
      // Sort the labels but maintain the same order for the arguments
      std::vector<std::pair<std::string, size_t>> pairs;
      pairs.reserve(labels.size());
      for (size_t i = 0; i < labels.size(); i++) {
        pairs.emplace_back(labels[i], arguments[i]);
      }
      std::sort(pairs.begin(), pairs.end());
      for (size_t i = 0; i < labels.size(); i++) {
        this->labels[i] = pairs[i].first;
        this->arguments[i] = pairs[i].second;
      }
    }
  }

  GVNValue(const int value, const Type type)
      : opcode(Opcode::Const), value(value), type(type) {}

  bool operator==(const GVNValue &other) const {
    return opcode == other.opcode && value == other.value &&
           arguments == other.arguments && labels == other.labels &&
           type == other.type;
  }

  friend std::ostream &operator<<(std::ostream &os, const GVNValue &value) {
    using util::operator<<;
    os << "GVNValue(" << value.opcode;
    if (value.opcode == Opcode::Const) {
      os << ", " << value.value << ", " << value.type << ")";
    } else if (value.opcode == Opcode::Phi) {
      os << ", " << value.arguments << ", " << value.labels << ", "
         << value.type << ")";
    } else {
      os << ", " << value.arguments << ", " << value.type << ")";
    }
    return os;
  }
};

struct GVNTable {
  static constexpr size_t NOT_FOUND = -1;

  // A map from variables to value numbers
  std::map<std::string, size_t> variable_to_value_number;
  // A vector of expressions (GVNValue's)
  std::vector<GVNValue> expressions;
  // A map from value numbers to canonical variable name
  std::vector<std::string> canonical_variables;

  void insert_axiom(const std::string &name, const Type type) {
    const size_t idx = expressions.size();
    variable_to_value_number[name] = idx;
    expressions.push_back(GVNValue(Opcode::Id, {idx}, {}, type));
    canonical_variables.push_back(name);
  }

  void insert_parameters(const std::vector<Variable> &parameters) {
    for (const auto &param : parameters)
      insert_axiom(param.name, param.type);
  }

  GVNValue create_value(const Instruction &instruction) const;

  GVNValue simplify(const GVNValue &value) const;

  size_t query_variable(const std::string &variable) const {
    runtime_assert(variable_to_value_number.count(variable) > 0,
                   "Variable " + variable + " not found in GVNTable");
    return variable_to_value_number.at(variable);
  }

  Instruction value_to_instruction(const std::string &destination,
                                   const GVNValue &value) const {
    if (value.opcode == Opcode::Const)
      return Instruction::constant(destination, value.value, value.type);

    std::vector<std::string> arguments;
    arguments.reserve(value.arguments.size());
    for (const size_t arg : value.arguments) {
      arguments.push_back(canonical_variables[arg]);
    }
    return Instruction(value.opcode, destination, value.type, arguments, {},
                       value.labels);
  }

  size_t query(const GVNValue &value) {
    const auto it = std::find(expressions.begin(), expressions.end(), value);
    if (it != expressions.end())
      return it - expressions.begin();
    return NOT_FOUND;
  }

  size_t query_or_insert(const std::string &destination,
                         const GVNValue &value) {
    const auto simplified_value = simplify(value);
    const size_t present_idx = query(simplified_value);
    if (present_idx != NOT_FOUND) {
      variable_to_value_number[destination] = present_idx;
      return present_idx;
    }

    const size_t idx = expressions.size();
    expressions.push_back(simplified_value);
    canonical_variables.push_back(destination);
    variable_to_value_number[destination] = idx;
    return idx;
  }
};

struct GlobalValueNumberingPass {
  ControlFlowGraph &cfg;
  GVNTable table;
  GlobalValueNumberingPass(ControlFlowGraph &cfg) : cfg(cfg) {}

  void run_pass() {
    runtime_assert(cfg.is_in_ssa_form(),
                   "CFG passed to GVN must be in SSA form");
    runtime_assert(!cfg.uses_pointers(),
                   "CFG passed to GVN must not use pointers");
    table.insert_parameters(cfg.arguments);
    process_block(cfg.entry_label);
    cfg.recompute_graph();
  }

  void process_block(const std::string &label);
};

inline size_t global_value_numbering(ControlFlowGraph &cfg) {
  if (!cfg.is_in_ssa_form() || cfg.uses_pointers())
    return 0;
  // std::cerr << cfg << std::endl;
  GlobalValueNumberingPass(cfg).run_pass();
  return 0;
}

} // namespace bril
