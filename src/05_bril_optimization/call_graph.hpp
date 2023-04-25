
#pragma once

#include "bril.hpp"
#include "util.hpp"

namespace bril {

struct CallGraph {
  const Program &program;
  std::unordered_map<std::string, std::unordered_set<std::string>> graph;

  using StronglyConnectedComponent = std::unordered_set<std::string>;
  std::unordered_map<std::string, size_t> function_to_component;
  std::vector<StronglyConnectedComponent> components;
  std::vector<std::unordered_set<size_t>> component_graph;

  CallGraph(const Program &program) : program(program) {
    compute_call_edges();
    compute_strongly_connected_components();
    compute_topological_order();
  }

  void compute_call_edges() {
    program.for_each_function([&](const ControlFlowGraph &function) {
      graph.insert({function.name, {}});
    });

    program.for_each_function([&](const ControlFlowGraph &function) {
      function.for_each_instruction([&](const Instruction &instruction) {
        if (instruction.opcode == Opcode::Call) {
          const auto &called_function_name = instruction.funcs[0];
          graph[function.name].insert(called_function_name);
        }
      });
    });
  }

  void compute_strongly_connected_components() {
    // Tarjan's algorithm
    size_t next_idx = 0;
    std::unordered_map<std::string, size_t> indices;
    std::unordered_map<std::string, size_t> low_links;
    std::unordered_set<std::string> stack_set;
    std::vector<std::string> stack;

    const std::function<void(const std::string &)> dfs =
        [&](const std::string &node) {
          stack.push_back(node);
          stack_set.insert(node);
          indices[node] = low_links[node] = next_idx++;

          for (const auto &next : graph.at(node)) {
            if (indices.count(next) == 0)
              dfs(next);
            if (stack_set.count(next) > 0)
              low_links.at(node) =
                  std::min(low_links.at(node), low_links.at(next));
          }

          if (indices.at(node) == low_links.at(node)) {
            StronglyConnectedComponent component;
            std::string top;
            do {
              top = stack.back();
              stack.pop_back();
              stack_set.erase(top);
              component.insert(top);
              function_to_component[top] = components.size();
            } while (top != node);
            components.push_back(component);
          }
        };

    for (const auto &[node, edges] : graph) {
      if (indices.count(node) == 0)
        dfs(node);
    }

    component_graph.resize(components.size());
    for (const auto &[node, edges] : graph) {
      const auto &component = function_to_component.at(node);
      for (const auto &next : edges) {
        const auto &next_component = function_to_component.at(next);
        if (component != next_component)
          component_graph[component].insert(next_component);
      }
    }
  }

  void compute_topological_order() {
    std::vector<size_t> in_degree(component_graph.size(), 0);
    std::vector<size_t> topological_order;
    topological_order.reserve(component_graph.size());

    // Initialize a queue of nodes with in-degree 0
    std::queue<size_t> queue;
    for (const auto &neighbours : component_graph)
      for (const auto &neighbour : neighbours)
        ++in_degree[neighbour];
    for (size_t i = 0; i < in_degree.size(); ++i)
      if (in_degree[i] == 0)
        queue.push(i);

    // While the stack is not empty, pop a node and add it to the topological
    // order; decrement the in-degree of its neighbours and update the stack
    while (!queue.empty()) {
      const size_t node = queue.front();
      queue.pop();
      topological_order.push_back(node);
      for (const auto &neighbour : component_graph[node]) {
        --in_degree[neighbour];
        if (in_degree[neighbour] == 0)
          queue.push(neighbour);
      }
    }

    // Since we performed a SCC decomposition, the component graph is acyclic
    // and thus the topological sort should produce an ordering of the SCCs
    debug_assert(topological_order.size() == component_graph.size(),
                 "The strongly-connected component graph is not acyclic");

    // Reverse the order so that the functions which call no other functions are
    // first
    std::reverse(topological_order.begin(), topological_order.end());
    std::vector<StronglyConnectedComponent> ordered_components;
    for (const auto &node : topological_order)
      ordered_components.push_back(components[node]);
    components = ordered_components;
  }

  friend std::ostream &operator<<(std::ostream &os, const CallGraph &graph) {
    using util::operator<<;
    os << "Function edges: " << std::endl;
    for (const auto &[function, called_functions] : graph.graph) {
      os << "  " << function << ": " << called_functions << std::endl;
    }
    os << "Component graph: " << std::endl;
    for (size_t i = 0; i < graph.component_graph.size(); ++i) {
      os << "  " << i << ": " << graph.components[i] << std::endl;
      os << "  - edges: " << graph.component_graph[i] << std::endl;
    }
    return os;
  }
};

} // namespace bril
