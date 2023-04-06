
#pragma once

#include "bril.hpp"

namespace bril {

struct LocalValueNumber {
  Opcode opcode;
  std::vector<size_t> arguments;
  int value = 42069;

  // Construct and canonicalize
  LocalValueNumber(const Opcode opcode, const std::vector<size_t> &arguments);
  LocalValueNumber(const int value);

  bool operator==(const LocalValueNumber &other) const {
    return opcode == other.opcode && arguments == other.arguments &&
           value == other.value;
  }
};

struct LocalValueTable {
  std::vector<LocalValueNumber> values;
  std::vector<std::string> canonical_variables;
  std::map<std::string, size_t> env;

  std::map<std::string, size_t> last_write;

  static inline size_t NOT_FOUND = -1;
  std::string canonical_name(const std::string &variable) const;
  std::optional<int> fold_constants(const LocalValueNumber &value) const;

  size_t query_row(const LocalValueNumber &value) const;
  std::string fresh_name(const std::string &current_name) const;

  friend std::ostream &operator<<(std::ostream &os,
                                  const LocalValueTable &table) {
    const size_t num_entries = table.values.size();

    for (size_t i = 0; i < num_entries; ++i) {
      const auto lvn = table.values[i];
      os << "index: " << i << ", variable: " << table.canonical_variables[i]
         << ", ";
      if (lvn.opcode == Opcode::Const) {
        os << "value: const " << lvn.value << std::endl;
      } else {
        os << "value: " << lvn.opcode;
        for (const auto &argument : lvn.arguments) {
          os << " ." << argument;
        }
        os << std::endl;
      }
    }
    return os;
  }
};

size_t local_value_numbering(Block &block);

} // namespace bril
