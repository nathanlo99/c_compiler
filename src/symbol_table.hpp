
#pragma once

#include "ast_base.hpp"
#include "types.hpp"
#include "util.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

struct SymbolTable {
  std::map<std::string, std::map<std::string, Type>> types;
  std::map<std::string, std::vector<Variable>> arguments;
  std::map<std::string, Type> return_types;
  std::map<std::string, std::set<std::string>> used_variables;

  mutable std::string current_procedure = "";

  void add_parameter(const std::string &procedure, const Variable &variable) {
    types[procedure][variable.name] = variable.type;
    arguments[procedure].push_back(variable);
    // TODO: Add offsets and things
  }

  void set_return_type(const std::string &procedure, const Type type) {
    return_types[procedure] = type;
  }

  void add_variable(const std::string &procedure, const Variable &variable) {
    types[procedure][variable.name] = variable.type;
    // TODO: Add offsets and things
  }

  void remove_parameter(const std::string &procedure,
                        const Variable &variable) {
    auto &argument_list = arguments[procedure];
    auto it = std::find(argument_list.begin(), argument_list.end(), variable);
    argument_list.erase(it);
    remove_variable(procedure, variable);
  }

  void remove_variable(const std::string &procedure, const Variable &variable) {
    auto symbol_table = types[procedure];
    symbol_table.erase(variable.name);
    // TODO: Remove offsets too
  }

  void record_variable_read(const Variable &variable) {
    used_variables[current_procedure].insert(variable.name);
  }

  void record_variable_write(const Variable &variable) {
    used_variables[current_procedure].insert(variable.name);
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

  bool is_variable_used(const std::string &procedure,
                        const std::string &variable) const {
    return used_variables.count(procedure) > 0 &&
           used_variables.at(procedure).count(variable) > 0;
  }

  friend std::ostream &operator<<(std::ostream &os, const SymbolTable &table) {
    for (const auto &[procedure, symbol_table] : table.types) {
      os << "In procedure " << procedure << ": " << std::endl;
      for (const auto &[variable, type] : symbol_table) {
        const bool is_used = table.is_variable_used(procedure, variable);
        os << "  " << variable << ": " << type_to_string(type) << " "
           << (is_used ? "(used)" : "(unused)") << std::endl;
      }
      os << std::endl;
    }
    return os;
  }
};
