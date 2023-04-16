
#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "bril.hpp"

namespace bril {
template <typename T> struct DataFlowResult {
  std::map<std::string, T> in, out;
};

template <typename T, typename Merge = std::function<T(const std::vector<T> &)>,
          typename Transfer = std::function<T(T, const std::string &,
                                              const ControlFlowGraph &)>>
DataFlowResult<T> solve_forward_data_flow(const ControlFlowGraph &graph,
                                          const T &init, const Merge &merge,
                                          const Transfer &transfer) {
  using block_idx_t = std::string;
  using worklist_t = std::vector<block_idx_t>;

  DataFlowResult<T> result;
  worklist_t worklist;
  result.in[graph.entry_label] = init;
  for (const auto &label : graph.block_labels) {
    result.out[label] = init;
    worklist.push_back(label);
  }

  while (!worklist.empty()) {
    const block_idx_t b = worklist.back();
    const Block &block = graph.get_block(b);
    worklist.pop_back();

    // 1. in[b] = merge(out[p] for every pred p of b)
    std::vector<T> arguments;
    for (const block_idx_t &pred : block.incoming_blocks) {
      arguments.push_back(result.out[pred]);
    }
    result.in[b] = merge(arguments);

    // 2. out[b] = transfer(in[b], b)
    const T old_out = result.out[b];
    result.out[b] = transfer(result.in[b], b, graph);

    // 3. if out[b] changed, add successors of b to worklist
    if (result.out[b] != old_out) {
      for (const block_idx_t &succ : block.outgoing_blocks) {
        worklist.push_back(succ);
      }
    }
  }

  return result;
}

template <typename T, typename Merge = std::function<T(const std::vector<T> &)>,
          typename Transfer = std::function<T(T, const std::string &,
                                              const ControlFlowGraph &)>>
DataFlowResult<T> solve_backward_data_flow(const ControlFlowGraph &graph,
                                           const T &init, const Merge &merge,
                                           const Transfer &transfer) {
  using block_idx_t = std::string;
  using worklist_t = std::vector<block_idx_t>;

  DataFlowResult<T> result;
  worklist_t worklist;
  result.out[graph.entry_label] = init;
  for (const auto &label : graph.block_labels) {
    result.in[label] = init;
    worklist.push_back(label);
  }

  while (!worklist.empty()) {
    const block_idx_t b = worklist.back();
    const Block &block = graph.get_block(b);
    worklist.pop_back();

    // 1. out[b] = merge(in[p] for every succ p of b)
    std::vector<T> arguments;
    // TODO: Do we have to inject init here?
    for (const block_idx_t &succ : block.outgoing_blocks) {
      arguments.push_back(result.in[succ]);
    }
    result.out[b] = merge(arguments);

    // 2. in[b] = transfer(out[b], b)
    const T old_in = result.in[b];
    result.in[b] = transfer(result.out[b], b, graph);

    // 3. if in[b] changed, add predecessors of b to worklist
    if (result.in[b] != old_in) {
      for (const block_idx_t &pred : block.incoming_blocks) {
        worklist.push_back(pred);
      }
    }
  }

  return result;
}

struct ReachingDefinitions {
  // (destination, block_label, instruction_idx)
  using def_t = std::tuple<std::string, std::string, size_t>;
  using T = std::set<def_t>;

  static T merge(const std::vector<T> &args) {
    T result;
    for (const auto &arg : args) {
      result.insert(arg.begin(), arg.end());
    }
    return result;
  }

  static T transfer(const T &in, const std::string &block_label,
                    const ControlFlowGraph &graph) {
    std::map<std::string, std::vector<def_t>> definitions;
    for (const auto &definition : in) {
      const auto &[destination, block_label, instruction_idx] = definition;
      definitions[destination].push_back(definition);
    }

    const Block &block = graph.get_block(block_label);
    for (size_t i = 0; i < block.instructions.size(); ++i) {
      const auto &instruction = block.instructions[i];
      const std::string destination = instruction.destination;
      if (destination != "") {
        definitions[destination] = {
            std::make_tuple(destination, block_label, i)};
      }
    }

    T result;
    for (const auto &[variable, defs] : definitions) {
      result.insert(defs.begin(), defs.end());
    }
    return result;
  }

  static DataFlowResult<T> solve(const ControlFlowGraph &graph) {
    T init;
    for (size_t i = 0; i < graph.arguments.size(); ++i) {
      const auto &argument = graph.arguments[i];
      init.emplace(argument.name, "__param", i);
    }
    return solve_forward_data_flow(graph, init, merge, transfer);
  }
};

} // namespace bril
