
#pragma once

#include "bril.hpp"
#include "data_flow.hpp"
#include "mips_instruction.hpp"
#include "util.hpp"
#include <queue>
#include <unordered_set>

namespace bril {

struct LivenessAnalysis
    : BackwardDataFlowPass<std::unordered_set<std::string>> {
  using Result = std::unordered_set<std::string>;

  LivenessAnalysis(const ControlFlowGraph &graph)
      : BackwardDataFlowPass(graph) {}

  Result init() override { return {}; }
  Result merge(const std::vector<Result> &args) override {
    Result result;
    for (const Result &arg : args) {
      result.insert(arg.begin(), arg.end());
    }
    return result;
  }
  Result transfer(const Result &out, const InstructionLocation &,
                  const Instruction &instruction) override {
    Result result = out;
    // Remove the destination first
    if (instruction.destination != "") {
      result.erase(instruction.destination);
    }
    // Add arguments
    result.insert(instruction.arguments.begin(), instruction.arguments.end());
    return result;
  }
};

struct RegisterInterferenceGraph {
  typename LivenessAnalysis::DataFlowResult liveness_data;

  std::unordered_map<std::string, size_t> variable_to_index;
  std::vector<std::string> index_to_variable;

  std::vector<std::unordered_set<size_t>> edges;

  RegisterInterferenceGraph(const ControlFlowGraph &graph)
      : liveness_data(LivenessAnalysis(graph).run()) {
    // TODO: Skip adding edges if the variable is never used
    for (const auto &arg1 : graph.arguments) {
      for (const auto &arg2 : graph.arguments) {
        add_edge(arg1.name, arg2.name);
      }
    }

    graph.for_each_block([&](const Block &block) {
      for (size_t i = 0; i <= block.instructions.size(); ++i) {
        const auto &live_variables =
            liveness_data.data.at(block.entry_label)[i];
        for (const auto &var1 : live_variables)
          for (const auto &var2 : live_variables)
            add_edge(var1, var2);
      }
    });
  }

  void add_variable(const std::string &var) {
    if (variable_to_index.count(var) > 0)
      return;
    const size_t idx = index_to_variable.size();
    variable_to_index[var] = idx;
    index_to_variable.push_back(var);
    edges.emplace_back();
  }

  size_t get_index(const std::string &var) {
    add_variable(var);
    return variable_to_index[var];
  }

  void add_edge(const std::string &var1, const std::string &var2) {
    const size_t idx1 = get_index(var1);
    const size_t idx2 = get_index(var2);
    // NOTE: We do this check after calling get_index so all live variables are
    // present in the graph
    if (var1 == var2)
      return;
    edges[idx1].insert(idx2);
    edges[idx2].insert(idx1);
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const RegisterInterferenceGraph &graph) {
    for (size_t idx = 0; idx < graph.index_to_variable.size(); ++idx) {
      os << graph.index_to_variable[idx] << ": (" << graph.edges[idx].size()
         << ")" << std::endl;
      bool first = true;
      os << "  [";
      for (const size_t neighbor : graph.edges[idx]) {
        if (first) {
          first = false;
        } else {
          os << ", ";
        }
        os << graph.index_to_variable[neighbor];
      }
      os << "]" << std::endl;
    }
    return os;
  }
};

struct VariableLocation {
  enum class Type { Register, Stack } type;
  union {
    Reg reg;
    int offset;
  };
  static VariableLocation register_location(Reg reg) {
    return {.type = Type::Register, .reg = reg};
  }
  static VariableLocation stack_location(int offset) {
    return {.type = Type::Stack, .offset = offset};
  }

  bool in_memory() const { return type == Type::Stack; }

  friend std::ostream &operator<<(std::ostream &os,
                                  const VariableLocation &location) {
    switch (location.type) {
    case Type::Register:
      os << fmt::format("{}", location.reg);
      break;
    case Type::Stack:
      os << location.offset << "($BP)";
      break;
    default:
      unreachable("Invalid VariableLocation type");
      break;
    }
    return os;
  }
};

struct RegisterAllocation {
  std::unordered_map<std::string, Reg> register_allocation;
  std::unordered_map<std::string, int> spilled_variables;
  typename LivenessAnalysis::DataFlowResult liveness_data;

  int next_offset = 0;

  void spill_variable(const std::string &variable) {
    spilled_variables[variable] = next_offset;
    next_offset -= 4;
  }

  bool in_register(const std::string &variable) const {
    return register_allocation.count(variable) > 0;
  }
  bool is_spilled(const std::string &variable) const {
    return spilled_variables.count(variable) > 0;
  }
  Reg get_register(const std::string &variable) const {
    debug_assert(
        in_register(variable),
        "RegisterAllocation::get_register: Variable {} is not in a register",
        variable);
    return register_allocation.at(variable);
  }
  int get_offset(const std::string &variable) const {
    debug_assert(is_spilled(variable),
                 "RegisterAllocation::get_offset: Variable {} is not spilled",
                 variable);
    return spilled_variables.at(variable);
  }

  VariableLocation get_location(const std::string &variable) const {
    debug_assert(in_register(variable) || is_spilled(variable),
                 "Variable {} is not allocated", variable);
    if (in_register(variable)) {
      return VariableLocation::register_location(get_register(variable));
    } else {
      return VariableLocation::stack_location(get_offset(variable));
    }
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const RegisterAllocation &allocation) {
    using util::operator<<;
    os << "Register allocation:" << std::endl;
    for (const auto &[var, reg] : allocation.register_allocation) {
      os << fmt::format("  {} -> {}", var, reg) << std::endl;
    }
    os << "Spilled variables: " << std::endl;
    for (const auto &[var, offset] : allocation.spilled_variables) {
      os << "  " << var << " -> " << offset << "($29)" << std::endl;
    }
    return os;
  }
};

RegisterAllocation
allocate_registers(const ControlFlowGraph &function,
                   const std::vector<Reg> &available_registers);

} // namespace bril
