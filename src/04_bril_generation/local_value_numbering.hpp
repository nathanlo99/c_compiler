
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
  void add_parameter(const std::string &param);
  size_t add_definition(const LocalValueNumber &value, const std::string &dest);
  void update_env(const std::string &variable, const size_t num);
  size_t query_env(const std::string &variable);
  std::string canonical_name(const std::string &variable);

  std::optional<int> fold_constants(const LocalValueNumber &value);

  size_t query_row(const LocalValueNumber &value) const;
  std::string fresh_name(const std::string &current_name) const;

  friend std::ostream &operator<<(std::ostream &os,
                                  const LocalValueTable &table) {
    const size_t num_entries = table.values.size();

    for (size_t i = 0; i < num_entries; ++i) {
      const auto lvn = table.values[i];
      os << i << ":" << std::endl;
      os << "  variable: " << table.canonical_variables[i] << std::endl;
      os << "  value: " << lvn.opcode;
      for (const auto &argument : lvn.arguments) {
        os << " #" << argument;
      }
      os << " (value = " << lvn.value << ")" << std::endl;
    }
    return os;
  }
};

size_t local_value_numbering(Block &block);

} // namespace bril
