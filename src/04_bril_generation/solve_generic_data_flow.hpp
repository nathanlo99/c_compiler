
#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "bril.hpp"

namespace bril {
template <typename T> struct DataFlowResult {
  std::vector<T> in, out;

  DataFlowResult(const bril::ControlFlowGraph &cfg)
      : in(cfg.blocks.size()), out(cfg.blocks.size()) {}
};

template <typename T, typename Merge = std::function<T(const std::vector<T> &)>,
          typename Transfer = std::function<T(T, const Block &)>>
DataFlowResult<T> solve_data_flow(const ControlFlowGraph &graph, const T &init,
                                  const Merge &merge,
                                  const Transfer &transfer) {
  using block_idx_t = size_t;
  using worklist_t = std::vector<block_idx_t>;

  DataFlowResult<T> result(graph);
  worklist_t worklist;
  result.in[0] = init;
  for (block_idx_t i = 0; i < graph.blocks.size(); ++i) {
    worklist.push_back(i);
    result.out[i] = init;
  }

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
    result.out[b] = transfer(result.in[b], block);

    // 3. if out[b] changed, add successors of b to worklist
    if (result.out[b] != old_out) {
      for (block_idx_t succ : block.outgoing_blocks) {
        worklist.push_back(succ);
      }
    }
  }

  return result;
}

} // namespace bril
