
#include "ssa_copy_propagation.hpp"

namespace bril {

size_t ssa_copy_propagation(ControlFlowGraph &function) {
  if (!function.is_in_ssa_form())
    return 0;

  // dest -> immediate source, for every `dest = id source`. Resolved lazily
  // per use below (via resolve()), so insertion order doesn't matter.
  std::unordered_map<std::string, std::string> id_source;
  function.for_each_instruction([&](const Instruction &instruction) {
    if (instruction.opcode == Opcode::Id)
      id_source[instruction.destination] = instruction.arguments[0];
  });
  if (id_source.empty())
    return 0;

  const auto resolve = [&](const std::string &name) {
    std::string current = name;
    while (id_source.contains(current))
      current = id_source.at(current);
    return current;
  };

  size_t num_substituted = 0;
  function.for_each_instruction([&](Instruction &instruction) {
    for (auto &argument : instruction.arguments) {
      const std::string resolved = resolve(argument);
      if (resolved != argument) {
        argument = resolved;
        ++num_substituted;
      }
    }
  });

  return num_substituted;
}

} // namespace bril
