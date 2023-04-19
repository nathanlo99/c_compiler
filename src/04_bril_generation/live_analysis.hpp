
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

struct RegisterInterferenceGraph {
  std::map<std::string, size_t> register_to_index;
  std::vector<std::string> index_to_register;

  std::vector<std::set<size_t>> edges;

  RegisterInterferenceGraph(const ControlFlowGraph &graph) {
    LivenessAnalysis analysis(graph);
    const auto liveness_data = analysis.run();

    for (const auto &[label, block] : graph.blocks) {
      const auto &live_variables = liveness_data.at(label).live_variables;
      for (const auto &live_set : live_variables) {
        for (const auto &var1 : live_set) {
          for (const auto &var2 : live_set) {
            add_edge(var1, var2);
          }
        }
      }
    }
  }

  void add_variable(const std::string &var) {
    runtime_assert(register_to_index.count(var) == 0,
                   "Variable " + var +
                       " already exists in register interference graph");
    const size_t idx = index_to_register.size();
    register_to_index[var] = idx;
    index_to_register.push_back(var);
    edges.push_back({});
  }

  size_t get_index(const std::string &var) {
    if (register_to_index.count(var) == 0) {
      const size_t idx = index_to_register.size();
      add_variable(var);
      return idx;
    }
    return register_to_index[var];
  }

  void add_edge(const std::string &var1, const std::string &var2) {
    const size_t idx1 = get_index(var1);
    const size_t idx2 = get_index(var2);
    // NOTE: We do this check after calling get_index so all live variables are
    // present in the graph
    if (var1 == var2)
      return;
    edges[idx1].insert(idx2);
    edges[idx2].insert(idx1);
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const RegisterInterferenceGraph &graph) {
    for (size_t idx = 0; idx < graph.index_to_register.size(); ++idx) {
      os << graph.index_to_register[idx] << ": (" << graph.edges[idx].size()
         << ")" << std::endl;
      bool first = true;
      os << "  [";
      for (const size_t neighbor : graph.edges[idx]) {
        if (first) {
          first = false;
        } else {
          os << ", ";
        }
        os << graph.index_to_register[neighbor];
      }
      os << "]" << std::endl;
    }
    return os;
  }
};

} // namespace bril
