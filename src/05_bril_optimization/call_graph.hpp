
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

  void compute_topological_order() {}

  friend std::ostream &operator<<(std::ostream &os, const CallGraph &graph) {
    using util::operator<<;
    os << "Function edges: " << std::endl;
    for (const auto &[function, called_functions] : graph.graph) {
      os << "  " << function << ": " << called_functions << std::endl;
    }
    os << "Strongly connected components: " << graph.components << std::endl;
    os << "Component graph: " << std::endl;
    for (size_t i = 0; i < graph.component_graph.size(); ++i)
      os << "  " << i << ": " << graph.component_graph[i] << std::endl;
    return os;
  }
};

} // namespace bril
