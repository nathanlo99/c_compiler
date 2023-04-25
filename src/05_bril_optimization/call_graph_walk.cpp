
#include "call_graph_walk.hpp"
#include "bril.hpp"
#include "run_optimization.hpp"

namespace bril {

bool optimize_call_graph(Program &program) {
  bool result = false;
  CallGraph call_graph(program);
  const auto should_inline = [&](const ControlFlowGraph &function) {
    if (call_graph.graph.at(function.name).count(function.name) > 0)
      return false;
    return function.num_instructions() < 10 || function.num_labels() < 5;
  };

  for (size_t component = 0; component < call_graph.component_graph.size();
       ++component) {
    const auto &function_names = call_graph.components[component];
    const auto &component_graph = call_graph.component_graph[component];

    std::unordered_set<std::string> to_inline;
    for (const size_t neighbour : component_graph) {
      const auto &neighbour_function_names = call_graph.components[neighbour];
      for (const auto &called_function_name : neighbour_function_names) {
        const auto &called_function =
            program.get_function(called_function_name);
        if (should_inline(called_function)) {
          to_inline.insert(called_function_name);
        }
      }
    }

    while (true) {
      bool changed = false;
      for (const std::string &inline_function : to_inline) {
        for (const auto &function_name : function_names) {
          changed |= program.inline_function(function_name, inline_function);
        }
        run_optimization_passes(program);
      }
      result |= changed;
      if (!changed)
        break;
    }
    run_optimization_passes(program);
  }

  return result;
}

} // namespace bril
