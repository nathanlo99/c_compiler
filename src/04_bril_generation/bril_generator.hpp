
#pragma once

#include "bril.hpp"
#include "symbol_table.hpp"
#include "util.hpp"

#include <map>
#include <string>

namespace bril {

class BRILGenerator {
  std::string current_function;
  std::unordered_map<std::string, Function> functions;

public:
  Program program() const {
    Program program;
    for (const auto &[name, function] : functions) {
      program.functions.emplace(name, ControlFlowGraph(function));
    }
    return program;
  }

  inline std::string temp() const {
    // NOTE: The underscore is important here to make sure we don't collide with
    // any user-defined variables
    static int next_idx = 0;
    const size_t idx = next_idx++;
    return "_t" + std::to_string(idx);
  }
  inline std::string generate_label(const std::string &label_type) {
    static std::unordered_map<std::string, int> next_indices;
    const int next_idx = next_indices[label_type]++;
    return label_type + std::to_string(next_idx);
  }

  void add_function(const std::string &name,
                    const std::vector<Variable> &arguments,
                    const Type return_type) {
    debug_assert(functions.count(name) == 0, "Duplicate function {}", name);
    functions.emplace(name, Function(name, arguments, return_type));
  }

  void enter_function(const std::string &function) {
    debug_assert(functions.count(function) > 0, "Unrecognized function {}",
                 function);
    current_function = function;
  }

  void leave_function() { current_function.clear(); }

  Function &function() { return functions.at(current_function); }
  const Function &function() const { return functions.at(current_function); }

  inline void emit(const Instruction &instruction) {
    function().instructions.push_back(instruction);
  }
  inline std::string last_result() const {
    const auto &instructions = function().instructions;
    for (int idx = instructions.size() - 1; idx >= 0; --idx) {
      const auto &instruction = instructions[idx];
      if (instruction.destination != "") {
        return instruction.destination;
      }
    }
    unreachable("Cannot grab last result: no instructions in {}",
                current_function);
    return "??";
  }
  inline Type last_type() const {
    const auto &instructions = function().instructions;
    for (int idx = instructions.size() - 1; idx >= 0; --idx) {
      const auto &instruction = instructions[idx];
      if (instruction.destination != "") {
        return instruction.type;
      }
    }
    unreachable("Cannot grab last type: no instructions in {}",
                current_function);
    return Type::Unknown;
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
                   const std::vector<std::string> &arguments, const Type type) {
    emit(Instruction::call(destination, function, arguments, type));
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
  void ptrsub(const std::string &destination, const std::string &lhs,
              const std::string &rhs) {
    emit(Instruction::ptrsub(destination, lhs, rhs));
  }
  void ptrdiff(const std::string &destination, const std::string &lhs,
               const std::string &rhs) {
    emit(Instruction::ptrdiff(destination, lhs, rhs));
  }
  void addressof(const std::string &destination, const std::string &argument) {
    emit(Instruction::addressof(destination, argument));
  }
  void label(const std::string &label_value) {
    emit(Instruction::label(label_value));
  }
  void phi(const std::string &destination, const Type type,
           const std::vector<std::string> &values,
           const std::vector<std::string> &labels) {
    emit(Instruction::phi(destination, type, values, labels));
  }
};
} // namespace bril
