
#pragma once

#include "ast_node.hpp"
#include "types.hpp"

#include <map>
#include <string>

struct SymbolTable {
  std::map<std::string, std::map<std::string, Type>> types;
  std::map<std::string, std::vector<Type>> argument_types;
  std::map<std::string, Type> return_types;

  mutable std::string current_procedure = "";

  void add_parameter(const std::string &procedure, const Variable &variable) {
    types[procedure][variable.name] = variable.type;
    argument_types[procedure].push_back(variable.type);
    // TODO: Add offsets and things
  }

  void set_return_type(const std::string &procedure, const Type type) {
    return_types[procedure] = type;
  }

  void add_variable(const std::string &procedure, const Variable &variable) {
    types[procedure][variable.name] = variable.type;
    // TODO: Add offsets and things
  }

  Type get_variable_type(const Variable &variable) const {
    runtime_assert(types.count(current_procedure) > 0,
                   "Unknown procedure " + current_procedure);
    runtime_assert(types.at(current_procedure).count(variable.name) > 0,
                   "Unknown variable " + variable.name + " in procedure " +
                       current_procedure);
    return types.at(current_procedure).at(variable.name);
  }

  Type get_return_type(const std::string &procedure_name) const {
    return return_types.at(procedure_name);
  }

  void enter_procedure(const std::string &name) { current_procedure = name; }
  void leave_procedure() { current_procedure.clear(); }

  friend std::ostream &operator<<(std::ostream &os, const SymbolTable &table) {
    for (const auto &[procedure, table] : table.types) {
      os << "In procedure " << procedure << ": " << std::endl;
      for (const auto &[variable, type] : table) {
        os << "  " << variable << ": " << type_to_string(type) << std::endl;
      }
      os << std::endl;
    }
    return os;
  }
};
