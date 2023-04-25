
#pragma once

#include "bril.hpp"

namespace bril {

template <typename Data> struct ForwardDataFlowPass {
  using InResult = typename Data::InResult;
  using OutResult = typename Data::OutResult;

  const ControlFlowGraph &graph;
  ForwardDataFlowPass(const ControlFlowGraph &graph) : graph(graph) {}
  virtual ~ForwardDataFlowPass() {}

  virtual InResult init() = 0;
  virtual InResult merge(const std::vector<OutResult> &outs) = 0;
  virtual OutResult transfer(const InResult &in, const std::string &label,
                             Data &data) = 0;

  std::unordered_map<std::string, Data> run() {
    std::unordered_map<std::string, Data> result;
    std::unordered_set<std::string> worklist;

    for (const auto &label : graph.block_labels) {
      result.insert(std::make_pair(label, Data(graph.get_block(label))));
      worklist.insert(label);
    }
    result.at(graph.entry_label).in = init();

    while (!worklist.empty()) {
      const std::string b = *worklist.begin();
      worklist.erase(b);
      const Block &block = graph.get_block(b);
      auto &entry = result.at(b);

      // 1. in[b] = merge(out[p] for every pred p of b)
      std::vector<OutResult> arguments;
      arguments.reserve(block.incoming_blocks.size());
      for (const std::string &pred : block.incoming_blocks) {
        arguments.push_back(result[pred].out);
      }
      entry.in = merge(arguments);

      // 2. out[b] = transfer(in[b], b)
      const OutResult old_out = entry.out;
      entry.out = transfer(entry.in, b, entry);

      // 3. if out[b] changed, add successors of b to worklist
      if (entry.out != old_out) {
        for (const std::string &succ : block.outgoing_blocks) {
          worklist.insert(succ);
        }
      }
    }

    return result;
  }
};

template <typename Data> struct BackwardDataFlowPass {
  using InResult = typename Data::InResult;
  using OutResult = typename Data::OutResult;

  const ControlFlowGraph &graph;
  BackwardDataFlowPass(const ControlFlowGraph &graph) : graph(graph) {}
  virtual ~BackwardDataFlowPass() {}

  virtual OutResult init() = 0;
  virtual OutResult merge(const std::vector<InResult> &args) = 0;
  virtual InResult transfer(const OutResult &in, const std::string &label,
                            Data &data) = 0;

  std::unordered_map<std::string, Data> run() {
    std::unordered_map<std::string, Data> result;
    std::unordered_set<std::string> worklist;

    for (const auto &label : graph.block_labels) {
      result.insert(std::make_pair(label, Data(graph.get_block(label))));
      worklist.insert(label);
    }
    for (const auto &exit_label : graph.exiting_blocks)
      result.at(exit_label).out = init();

    while (!worklist.empty()) {
      const std::string b = *worklist.begin();
      worklist.erase(b);
      const Block &block = graph.get_block(b);
      auto &entry = result.at(b);

      // 1. out[b] = merge(in[p] for every succ p of b)
      std::vector<OutResult> arguments;
      arguments.reserve(block.outgoing_blocks.size());
      for (const std::string &succ : block.outgoing_blocks) {
        arguments.push_back(result.at(succ).in);
      }
      entry.out = merge(arguments);

      // 2. in[b] = transfer(out[b], b)
      const InResult old_in = entry.in;
      entry.in = transfer(entry.out, b, entry);

      // 3. if in[b] changed, add successors of b to worklist
      if (entry.in != old_in) {
        for (const std::string &pred : block.incoming_blocks) {
          worklist.insert(pred);
        }
      }
    }

    return result;
  }
};

} // namespace bril
