
#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "bril.hpp"

namespace bril {
template <typename T> struct DataFlowResult {
  std::vector<T> in, out;

  DataFlowResult(const bril::ControlFlowGraph &cfg, const T &init)
      : in(cfg.blocks.size(), init), out(cfg.blocks.size(), init) {}
};

template <
    typename T, typename Merge = std::function<T(const std::vector<T> &)>,
    typename Transfer = std::function<T(T, size_t, const ControlFlowGraph &)>>
DataFlowResult<T> solve_forward_data_flow(const ControlFlowGraph &graph,
                                          const T &init, const Merge &merge,
                                          const Transfer &transfer) {
  using block_idx_t = size_t;
  using worklist_t = std::vector<block_idx_t>;

  DataFlowResult<T> result(graph, init);
  worklist_t worklist;
  for (block_idx_t i = 0; i < graph.blocks.size(); ++i)
    worklist.push_back(i);

  while (!worklist.empty()) {
    const block_idx_t b = worklist.back();
    const Block &block = graph.blocks[b];
    worklist.pop_back();

    // 1. in[b] = merge(out[p] for every pred p of b)
    std::vector<T> arguments;
    for (const block_idx_t pred : block.incoming_blocks) {
      arguments.push_back(result.out[pred]);
    }
    result.in[b] = merge(arguments);

    // 2. out[b] = transfer(in[b], b)
    const T old_out = result.out[b];
    result.out[b] = transfer(result.in[b], b, graph);

    // 3. if out[b] changed, add successors of b to worklist
    if (result.out[b] != old_out) {
      for (block_idx_t succ : block.outgoing_blocks) {
        worklist.push_back(succ);
      }
    }
  }

  return result;
}

template <
    typename T, typename Merge = std::function<T(const std::vector<T> &)>,
    typename Transfer = std::function<T(T, size_t, const ControlFlowGraph &)>>
DataFlowResult<T> solve_backward_data_flow(const ControlFlowGraph &graph,
                                           const T &init, const Merge &merge,
                                           const Transfer &transfer) {
  using block_idx_t = size_t;
  using worklist_t = std::vector<block_idx_t>;

  DataFlowResult<T> result(graph, init);
  worklist_t worklist;
  for (block_idx_t i = 0; i < graph.blocks.size(); ++i)
    worklist.push_back(i);

  while (!worklist.empty()) {
    const block_idx_t b = worklist.back();
    const Block &block = graph.blocks[b];
    worklist.pop_back();

    // 1. out[b] = merge(in[p] for every pred p of b)
    std::vector<T> arguments;
    for (const block_idx_t succ : block.outgoing_blocks) {
      arguments.push_back(result.in[succ]);
    }
    result.out[b] = merge(arguments);

    // 2. in[b] = transfer(out[b], b)
    const T old_in = result.in[b];
    result.in[b] = transfer(result.out[b], b, graph);

    // 3. if in[b] changed, add successors of b to worklist
    if (result.in[b] != old_in) {
      for (block_idx_t pred : block.incoming_blocks) {
        worklist.push_back(pred);
      }
    }
  }

  return result;
}

struct ReachingDefinitions {
  using def_t = std::pair<size_t, size_t>; // (block_idx, instruction_idx)
  using T = std::set<def_t>;

  const static inline T init = T();

  static T merge(const std::vector<T> &args) {
    T result;
    for (const auto &arg : args) {
      result.insert(arg.begin(), arg.end());
    }
    return result;
  }

  static T transfer(const T &in, const size_t block_idx,
                    const ControlFlowGraph &graph) {
    std::map<std::string, std::vector<std::pair<size_t, size_t>>> definitions;
    for (const auto &pair : in) {
      const auto &[block_idx, instruction_idx] = pair;
      const auto &destination =
          graph.blocks[block_idx].instructions[instruction_idx].destination;
      definitions[destination].push_back(pair);
    }

    const Block &block = graph.blocks[block_idx];
    for (size_t i = 0; i < block.instructions.size(); ++i) {
      const auto &instruction = block.instructions[i];
      const std::string destination = instruction.destination;
      if (destination != "")
        definitions[destination] = {std::make_pair(block_idx, i)};
    }

    T result;
    for (const auto &[variable, defs] : definitions) {
      result.insert(defs.begin(), defs.end());
    }
    return result;
  }

  static DataFlowResult<T> solve(const ControlFlowGraph &graph) {
    return solve_forward_data_flow(graph, init, merge, transfer);
  }
};

} // namespace bril
