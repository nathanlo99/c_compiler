
#pragma once

#include "bril.hpp"

namespace bril {

struct InstructionLocation {
  std::string label;
  size_t instruction_idx;
  InstructionLocation(const std::string &label, const size_t instruction_idx)
      : label(label), instruction_idx(instruction_idx) {}
};

// Stores data-flow results which change per-instruction
template <typename Result> struct InstructionDataFlowResult {
  std::unordered_map<std::string, std::vector<Result>> data;

  InstructionDataFlowResult() = default;

  void init_block(const std::string &label, const Block &block) {
    data[label] = std::vector<Result>(block.instructions.size() + 1);
  }

  const Result &get_block_in(const std::string &label) const {
    return data.at(label).front();
  }
  const Result &get_block_out(const std::string &label) const {
    return data.at(label).back();
  }
  const Result &get_data_in(const std::string &label, const size_t idx) const {
    return data.at(label)[idx];
  }
  const Result &get_data_out(const std::string &label, const size_t idx) const {
    return data.at(label)[idx + 1];
  }
  bool set_data(const std::string &label, const size_t idx,
                const Result &result) {
    debug_assert(data.count(label) > 0, "Block not initialized");
    debug_assert(idx < data.at(label).size(),
                 "Invalid instruction index {} >= {}", idx,
                 data.at(label).size());
    auto &entry = data.at(label)[idx];
    if (entry == result)
      return false;
    entry = result;
    return true;
  }
  bool set_data_in(const std::string &label, const size_t idx,
                   const Result &result) {
    return set_data(label, idx, result);
  }
  bool set_data_out(const std::string &label, const size_t idx,
                    const Result &result) {
    return set_data(label, idx + 1, result);
  }
  bool set_block_in(const std::string &label, const Result &result) {
    return set_data(label, 0, result);
  }
  bool set_block_out(const std::string &label, const Result &result) {
    return set_data(label, data.at(label).size() - 1, result);
  }
};

template <typename Result> struct ForwardDataFlowPass {
  using DataFlowResult = InstructionDataFlowResult<Result>;
  const ControlFlowGraph &function;

  ForwardDataFlowPass(const ControlFlowGraph &function) : function(function) {}
  virtual ~ForwardDataFlowPass() {}

  virtual Result init() = 0;
  virtual Result merge(const std::vector<Result> &outs) = 0;
  virtual Result transfer(const Result &in, const InstructionLocation &location,
                          const Instruction &instruction) = 0;

  DataFlowResult run() {
    DataFlowResult result;
    std::queue<std::string> worklist;

    for (const auto &label : function.block_labels) {
      result.init_block(label, function.get_block(label));
      worklist.push(label);
    }
    result.set_block_in(function.entry_label, init());

    while (!worklist.empty()) {
      const std::string label = worklist.front();
      worklist.pop();
      const Block &block = function.get_block(label);

      // 1. in[b] = merge(out[p] for every pred p of b)
      if (label != function.entry_label) {
        std::vector<Result> arguments;
        arguments.reserve(block.incoming_blocks.size());
        for (const std::string &pred : block.incoming_blocks) {
          arguments.push_back(result.get_block_out(pred));
        }
        result.set_block_in(label, merge(arguments));
      }

      // 2. For every instruction I in the block, out[I] = transfer(in[I], I)
      bool changed = false;
      for (size_t i = 0; i < block.instructions.size(); ++i) {
        const Instruction &instruction = block.instructions[i];
        const Result &in = result.get_data_in(label, i);
        changed |= result.set_data_out(
            label, i, transfer(in, InstructionLocation(label, i), instruction));
      }

      // 3. If out[b] changed, add all successors of b to the worklist
      if (changed) {
        for (const std::string &succ : block.outgoing_blocks) {
          worklist.push(succ);
        }
      }
    }

    return result;
  }
};

template <typename Result> struct BackwardDataFlowPass {
  using DataFlowResult = InstructionDataFlowResult<Result>;
  const ControlFlowGraph &function;

  BackwardDataFlowPass(const ControlFlowGraph &function) : function(function) {}
  virtual ~BackwardDataFlowPass() {}

  virtual Result init() = 0;
  virtual Result merge(const std::vector<Result> &outs) = 0;
  virtual Result transfer(const Result &out,
                          const InstructionLocation &location,
                          const Instruction &instruction) = 0;

  DataFlowResult run() {
    DataFlowResult result;
    std::queue<std::string> worklist;

    for (const auto &label : function.block_labels) {
      result.init_block(label, function.get_block(label));
      worklist.push(label);
    }
    for (const auto &exit_label : function.exiting_blocks) {
      result.set_block_out(exit_label, init());
    }

    while (!worklist.empty()) {
      const std::string label = worklist.front();
      worklist.pop();
      const Block &block = function.get_block(label);

      // 1. out[b] = merge(in[p] for every succ p of b)
      if (function.exiting_blocks.count(label) == 0) {
        std::vector<Result> arguments;
        arguments.reserve(block.outgoing_blocks.size());
        for (const std::string &pred : block.outgoing_blocks) {
          arguments.push_back(result.get_block_in(pred));
        }
        result.set_block_out(label, merge(arguments));
      }

      // 2. For every instruction I in the block, in[I] = transfer(out[I], I)
      bool changed = false;
      for (int i = block.instructions.size() - 1; i >= 0; --i) {
        const Instruction &instruction = block.instructions[i];
        const Result &out = result.get_data_out(label, i);
        changed |= result.set_data_in(
            label, i,
            transfer(out, InstructionLocation(label, i), instruction));
      }

      // 3. If in[b] changed, add all predecessors of b to the worklist
      if (changed) {
        for (const std::string &pred : block.incoming_blocks) {
          worklist.push(pred);
        }
      }
    }

    return result;
  }
};

} // namespace bril
