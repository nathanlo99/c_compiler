
#pragma once

#include "bril.hpp"
#include "data_flow.hpp"

namespace bril {

struct LivenessData {
  using InResult = std::set<std::string>;
  using OutResult = std::set<std::string>;

  InResult in;
  OutResult out;

  // Stores the set of live instructions **before** every instruction
  std::vector<std::set<std::string>> live_variables;

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

} // namespace bril
