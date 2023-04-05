
#pragma once

#include "bril.hpp"
#include "symbol_table.hpp"
#include "util.hpp"

#include <map>
#include <string>

namespace bril {

class BRILGenerator {
  std::string current_function;
  std::map<std::string, Function> functions;

public:
  Program program() const {
    Program program;
    for (const auto &[name, function] : functions) {
      program.cfgs.push_back(ControlFlowGraph(function));
    }
    return program;
  }

  inline std::string temp() const {
    static int next_idx = 0;
    return "tmp_" + std::to_string(next_idx++);
  }
  inline std::string generate_label(const std::string label_type) {
    static std::map<std::string, int> next_indices;
    const int next_idx = next_indices[label_type];
    return "." + label_type + "_" + std::to_string(next_idx);
  }

  void add_function(const std::string &name,
                    const std::vector<Variable> &arguments,
                    const Type return_type) {
    runtime_assert(functions.count(name) == 0, "Duplicate function " + name);
    functions.insert(
        std::make_pair(name, Function(name, arguments, return_type)));
  }

  void enter_function(const std::string &function) {
    current_function = function;
  }

  void leave_function() { current_function.clear(); }

  Function &function() {
    runtime_assert(functions.count(current_function) > 0,
                   "Unrecognized function " + current_function);
    return functions.at(current_function);
  }
  const Function &function() const {
    runtime_assert(functions.count(current_function) > 0,
                   "Unrecognized function " + current_function);
    return functions.at(current_function);
  }

  inline void emit(const Instruction &instruction) {
    function().instructions.push_back(instruction);
  }
  inline std::string last_result() const {
    runtime_assert(function().instructions.size() > 0,
                   "Cannot grab last result: no instructions in " +
                       current_function);
    return function().instructions.back().destination;
  }
  inline Type last_type() const {
    runtime_assert(function().instructions.size() > 0,
                   "Cannot grab last result: no instructions in " +
                       current_function);
    return function().instructions.back().type;
  }

  inline void add(const std::string &dest, const std::string &lhs,
                  const std::string &rhs) {
    emit(Instruction::add(dest, lhs, rhs));
  }
  inline void sub(const std::string &dest, const std::string &lhs,
                  const std::string &rhs) {
    emit(Instruction::sub(dest, lhs, rhs));
  }
  inline void mul(const std::string &dest, const std::string &lhs,
                  const std::string &rhs) {
    emit(Instruction::mul(dest, lhs, rhs));
  }
  inline void div(const std::string &dest, const std::string &lhs,
                  const std::string &rhs) {
    emit(Instruction::div(dest, lhs, rhs));
  }
  inline void mod(const std::string &dest, const std::string &lhs,
                  const std::string &rhs) {
    emit(Instruction::mod(dest, lhs, rhs));
  }
  inline void lt(const std::string &dest, const std::string &lhs,
                 const std::string &rhs) {
    emit(Instruction::lt(dest, lhs, rhs));
  }
  inline void le(const std::string &dest, const std::string &lhs,
                 const std::string &rhs) {
    emit(Instruction::le(dest, lhs, rhs));
  }
  inline void gt(const std::string &dest, const std::string &lhs,
                 const std::string &rhs) {
    emit(Instruction::gt(dest, lhs, rhs));
  }
  inline void ge(const std::string &dest, const std::string &lhs,
                 const std::string &rhs) {
    emit(Instruction::ge(dest, lhs, rhs));
  }
  inline void eq(const std::string &dest, const std::string &lhs,
                 const std::string &rhs) {
    emit(Instruction::eq(dest, lhs, rhs));
  }
  inline void ne(const std::string &dest, const std::string &lhs,
                 const std::string &rhs) {
    emit(Instruction::ne(dest, lhs, rhs));
  }
  inline void jmp(const std::string &dest) { emit(Instruction::jmp(dest)); }
  inline void br(const std::string &dest, const std::string &true_label,
                 const std::string &false_label) {
    emit(Instruction::br(dest, true_label, false_label));
  }
  inline void call(const std::string &destination, const std::string &function,
                   const std::vector<std::string> &arguments) {
    emit(Instruction::call(destination, function, arguments));
  }
  void ret(const std::string &arg) { emit(Instruction::ret(arg)); }
  void constant(const std::string &destination, const Literal &value) {
    emit(Instruction::constant(destination, value));
  }
  void id(const std::string &destination, const std::string &value,
          const Type type) {
    emit(Instruction::id(destination, value, type));
  }
  void print(const std::string &value) { emit(Instruction::print(value)); }
  void nop() { emit(Instruction::nop()); }
  void alloc(const std::string &destination, const std::string &argument) {
    emit(Instruction::alloc(destination, argument));
  }
  void free(const std::string &argument) { emit(Instruction::free(argument)); }
  void store(const std::string &destination, const std::string &argument) {
    emit(Instruction::store(destination, argument));
  }
  void load(const std::string &destination, const std::string &argument) {
    emit(Instruction::load(destination, argument));
  }
  void ptradd(const std::string &destination, const std::string &lhs,
              const std::string &rhs) {
    emit(Instruction::ptradd(destination, lhs, rhs));
  }
  void addressof(const std::string &destination, const std::string &argument) {
    emit(Instruction::addressof(destination, argument));
  }
  void label(const std::string &label_value) {
    emit(Instruction::label(label_value));
  }
  void phi(const std::string &destination,
           const std::vector<std::string> &values,
           const std::vector<std::string> &labels) {
    emit(Instruction::phi(destination, values, labels));
  }
};
} // namespace bril
