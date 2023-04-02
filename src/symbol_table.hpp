
#pragma once

#include "ast_base.hpp"
#include "types.hpp"
#include "util.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

struct ProcedureTable {
  std::string name;
  std::map<std::string, Type> types;
  std::vector<Variable> arguments;
  Type return_type;
  std::set<std::string> used_variables;

  std::map<std::string, int> offsets;
  int next_offset = 0;

  ProcedureTable(const std::string &name) : name(name) {}

  void add_parameter(const Variable &variable) {
    arguments.push_back(variable);
    add_variable(variable);
  }

  void set_return_type(const Type type) { return_type = type; }

  void add_variable(const Variable &variable) {
    types[variable.name] = variable.type;
    offsets[variable.name] = next_offset;
    next_offset -= 4;
  }

  void remove_parameter(const Variable &variable) {
    auto it = std::find(arguments.begin(), arguments.end(), variable);
    arguments.erase(it);
    remove_variable(variable);
  }

  void remove_variable(const Variable &variable) {
    types.erase(variable.name);
    offsets.erase(variable.name);
  }

  void record_variable_read(const Variable &variable) {
    used_variables.insert(variable.name);
  }

  void record_variable_write(const Variable &variable) {
    used_variables.insert(variable.name);
  }

  Type get_variable_type(const Variable &variable) const {
    runtime_assert(types.count(variable.name) > 0, "Unknown variable " +
                                                       variable.name +
                                                       " in procedure " + name);
    return types.at(variable.name);
  }

  int get_offset(const Variable &variable) const {
    const int raw_offset = offsets.at(variable.name);
    const int num_params = arguments.size();
    return raw_offset + 4 * num_params;
  }

  bool is_variable_used(const std::string &variable) const {
    return used_variables.count(variable) > 0;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const ProcedureTable &table) {
    for (const auto &[variable, type] : table.types) {
      const bool is_used = table.is_variable_used(variable);
      os << "  " << variable << ": " << type_to_string(type) << " @ "
         << table.get_offset(Variable(variable, type)) << " "
         << (is_used ? "(used)" : "(unused)") << std::endl;
    }
    return os;
  }
};

struct SymbolTable {
  std::map<std::string, ProcedureTable> tables;
  mutable std::string current_procedure = "";

  bool use_print = false;
  bool use_memory = false;

  void enter_procedure(const std::string &name) { current_procedure = name; }
  void leave_procedure() { current_procedure.clear(); }

  void add_procedure(const std::string &name) {
    tables.insert(std::make_pair(name, ProcedureTable(name)));
  }

  ProcedureTable &get_table(const std::string &name) {
    runtime_assert(tables.count(name) > 0, "Unknown procedure '" + name + "'");
    return tables.at(name);
  }
  const ProcedureTable &get_table(const std::string &name) const {
    runtime_assert(tables.count(name) > 0, "Unknown procedure '" + name + "'");
    return tables.at(name);
  }

  void add_parameter(const std::string &procedure, const Variable &variable) {
    get_table(procedure).add_parameter(variable);
  }

  void set_return_type(const std::string &procedure, const Type type) {
    get_table(procedure).set_return_type(type);
  }

  void add_variable(const std::string &procedure, const Variable &variable) {
    get_table(procedure).add_variable(variable);
  }

  void remove_parameter(const std::string &procedure,
                        const Variable &variable) {
    get_table(procedure).remove_parameter(variable);
  }

  void remove_variable(const std::string &procedure, const Variable &variable) {
    get_table(procedure).remove_variable(variable);
  }

  void record_variable_read(const Variable &variable) {
    get_table(current_procedure).used_variables.insert(variable.name);
  }

  void record_variable_write(const Variable &variable) {
    get_table(current_procedure).used_variables.insert(variable.name);
  }

  Type get_variable_type(const Variable &variable) const {
    return get_table(current_procedure).get_variable_type(variable);
  }

  int get_offset(const Variable &variable) const {
    return get_table(current_procedure).get_offset(variable);
  }

  std::vector<Variable> get_arguments(const std::string &procedure) const {
    return get_table(procedure).arguments;
  }

  bool is_variable_used(const std::string &procedure,
                        const std::string &variable) const {
    return get_table(procedure).is_variable_used(variable);
  }

  Type get_return_type(const std::string &procedure_name) const {
    return tables.at(procedure_name).return_type;
  }

  friend std::ostream &operator<<(std::ostream &os, const SymbolTable &table) {
    os << "use_print: " << (table.use_print ? "true" : "false") << std::endl;
    os << "use_memory: " << (table.use_memory ? "true" : "false") << std::endl;
    for (const auto &[procedure, procedure_table] : table.tables) {
      os << "In procedure " << procedure << ": " << std::endl;
      os << procedure_table << std::endl;
    }
    return os;
  }
};
