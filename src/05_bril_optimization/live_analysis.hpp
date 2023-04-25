
#pragma once

#include "bril.hpp"
#include "data_flow.hpp"
#include "mips_instruction.hpp"
#include "util.hpp"
#include <queue>
#include <unordered_set>

namespace bril {

struct LivenessData {
  using InResult = std::unordered_set<std::string>;
  using OutResult = std::unordered_set<std::string>;

  InResult in;
  OutResult out;

  // Stores the set of live instructions **before** every instruction
  std::vector<std::unordered_set<std::string>> live_variables;

  LivenessData(const Block &block)
      : in(), out(), live_variables(block.instructions.size() + 1) {}
};

struct LivenessAnalysis : BackwardDataFlowPass<LivenessData> {
  LivenessAnalysis(const ControlFlowGraph &graph)
      : BackwardDataFlowPass(graph) {}
  ~LivenessAnalysis() = default;

  // By default, no variables are live at the end of the function
  OutResult init() override { return {}; }

  // The live variables at the end of a block are the union of the live
  // variables at the beginning of its successors
  OutResult merge(const std::vector<InResult> &args) override {
    OutResult result;
    for (const InResult &arg : args) {
      result.insert(arg.begin(), arg.end());
    }
    return result;
  }

  InResult transfer(const OutResult &out, const std::string &label,
                    LivenessData &data) override {
    const Block &block = graph.get_block(label);
    const size_t num_instructions = block.instructions.size();

    data.live_variables[num_instructions] = out;
    for (int idx = num_instructions - 1; idx >= 0; --idx) {
      const auto &instruction = block.instructions[idx];
      data.live_variables[idx] = data.live_variables[idx + 1];

      // NOTE: It's important to remove the destination first, since
      // the destination may also be an argument, and in this case, the variable
      // is live before the instruction
      if (instruction.destination != "") {
        data.live_variables[idx].erase(instruction.destination);
      }
      data.live_variables[idx].insert(instruction.arguments.begin(),
                                      instruction.arguments.end());
    }

    return data.live_variables[0];
  }
};

struct RegisterInterferenceGraph {
  std::unordered_map<std::string, LivenessData> liveness_data;

  std::unordered_map<std::string, size_t> variable_to_index;
  std::vector<std::string> index_to_variable;

  std::vector<std::unordered_set<size_t>> edges;

  RegisterInterferenceGraph(const ControlFlowGraph &graph) {
    LivenessAnalysis analysis(graph);
    liveness_data = analysis.run();
    // TODO: Skip adding edges if the variable is never used
    for (const auto &arg1 : graph.arguments) {
      for (const auto &arg2 : graph.arguments) {
        add_edge(arg1.name, arg2.name);
      }
    }

    graph.for_each_block([&](const Block &block) {
      const auto &live_variables =
          liveness_data.at(block.entry_label).live_variables;
      for (const auto &live_set : live_variables)
        for (const auto &var1 : live_set)
          for (const auto &var2 : live_set)
            add_edge(var1, var2);
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
  std::unordered_map<std::string, LivenessData> liveness_data;

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
    std::stringstream ss;
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

inline RegisterAllocation
allocate_registers(const ControlFlowGraph &function,
                   const std::vector<Reg> &available_registers) {
  RegisterInterferenceGraph graph(function);
  std::vector<size_t> node_stack;
  std::unordered_set<size_t> processed_nodes;
  std::vector<std::unordered_set<size_t>> edges = graph.edges;

  // First, gather all variables from the function for which we take addresses,
  // since these always have to be spilled to memory
  std::unordered_set<std::string> addressed_variables;
  function.for_each_instruction([&](const Instruction &instruction) {
    if (instruction.opcode == Opcode::AddressOf) {
      addressed_variables.insert(instruction.arguments[0]);
      const size_t arg_idx =
          graph.variable_to_index.at(instruction.arguments[0]);
      for (const size_t neighbour : graph.edges[arg_idx]) {
        edges[neighbour].erase(arg_idx);
      }
      edges[arg_idx].clear();
      processed_nodes.insert(arg_idx);
    }
  });

  // Queue of nodes sorted by degree (degree, node)
  std::priority_queue<std::pair<size_t, size_t>,
                      std::vector<std::pair<size_t, size_t>>,
                      std::greater<std::pair<size_t, size_t>>>
      queue;
  for (size_t idx = 0; idx < graph.index_to_variable.size(); ++idx) {
    queue.emplace(edges[idx].size(), idx);
  }

  while (!queue.empty()) {
    const auto [degree, node] = queue.top();
    queue.pop();
    if (processed_nodes.count(node) > 0)
      continue;
    processed_nodes.insert(node);
    node_stack.push_back(node);

    // Remove all edges from the node
    for (const size_t neighbour : edges[node]) {
      edges[neighbour].erase(node);
      queue.emplace(edges[neighbour].size(), neighbour);
    }
    edges[node].clear();
  }

  // Reverse the node stack so we colour nodes in reverse order
  std::reverse(node_stack.begin(), node_stack.end());

  RegisterAllocation result;
  std::vector<std::set<Reg>> node_available_registers(
      graph.index_to_variable.size());
  for (size_t i = 0; i < node_available_registers.size(); ++i) {
    if (addressed_variables.count(graph.index_to_variable[i]) == 0)
      node_available_registers[i].insert(available_registers.begin(),
                                         available_registers.end());
  }

  for (const size_t node : node_stack) {
    const std::string &var = graph.index_to_variable[node];
    if (addressed_variables.count(var) > 0) {
      result.spill_variable(var);
      continue;
    }

    const std::set<Reg> &available_neighbours = node_available_registers[node];
    if (available_neighbours.empty()) {
      result.spill_variable(var);
    } else {
      const Reg reg = *available_neighbours.begin();
      result.register_allocation[var] = reg;
      // Update available registers for neighbours
      for (const size_t neighbour : graph.edges[node]) {
        node_available_registers[neighbour].erase(reg);
      }
    }
  }
  result.liveness_data = graph.liveness_data;
  return result;
}

} // namespace bril
