
#include "liveness_analysis.hpp"

namespace bril {

RegisterAllocation
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
