
#pragma once

#include "bril.hpp"
#include "symbol_table.hpp"
#include "util.hpp"

#include <map>
#include <string>

class BRILGenerator {
  SymbolTable table;
  std::string current_procedure;
  std::map<std::string, bril::Function> functions;

  void enter_procedure(std::string &procedure) {
    current_procedure = procedure;
    table.enter_procedure(procedure);
  }

  void leave_procedure() {
    current_procedure.clear();
    table.leave_procedure();
  }

  bril::Function &function() {
    runtime_assert(functions.count(current_procedure) > 0,
                   "Unrecognized procedure " + current_procedure);
    return functions.at(current_procedure);
  }

  inline void emit(const bril::Instruction &instruction) {
    function().instructions.push_back(instruction);
  }

  inline void add(const std::string &dest, const std::string &lhs,
                  const std::string &rhs) {
    emit(bril::Instruction::add(dest, lhs, rhs));
  }
  inline void sub(const std::string &dest, const std::string &lhs,
                  const std::string &rhs) {
    emit(bril::Instruction::sub(dest, lhs, rhs));
  }
  inline void mul(const std::string &dest, const std::string &lhs,
                  const std::string &rhs) {
    emit(bril::Instruction::mul(dest, lhs, rhs));
  }
  inline void div(const std::string &dest, const std::string &lhs,
                  const std::string &rhs) {
    emit(bril::Instruction::div(dest, lhs, rhs));
  }
  inline void mod(const std::string &dest, const std::string &lhs,
                  const std::string &rhs) {
    emit(bril::Instruction::mod(dest, lhs, rhs));
  }
  inline void lt(const std::string &dest, const std::string &lhs,
                 const std::string &rhs) {
    emit(bril::Instruction::lt(dest, lhs, rhs));
  }
  inline void le(const std::string &dest, const std::string &lhs,
                 const std::string &rhs) {
    emit(bril::Instruction::le(dest, lhs, rhs));
  }
  inline void gt(const std::string &dest, const std::string &lhs,
                 const std::string &rhs) {
    emit(bril::Instruction::gt(dest, lhs, rhs));
  }
  inline void ge(const std::string &dest, const std::string &lhs,
                 const std::string &rhs) {
    emit(bril::Instruction::ge(dest, lhs, rhs));
  }
  inline void eq(const std::string &dest, const std::string &lhs,
                 const std::string &rhs) {
    emit(bril::Instruction::eq(dest, lhs, rhs));
  }
  inline void ne(const std::string &dest, const std::string &lhs,
                 const std::string &rhs) {
    emit(bril::Instruction::ne(dest, lhs, rhs));
  }
  inline void jmp(const std::string &dest) {
    emit(bril::Instruction::jmp(dest));
  }
  inline void br(const std::string &dest, const std::string &true_label,
                 const std::string &false_label) {
    emit(bril::Instruction::br(dest, true_label, false_label));
  }
  inline void call(const std::string &destination, const std::string &function,
                   const std::vector<std::string> &arguments) {
    emit(bril::Instruction::call(destination, function, arguments));
  }
  void ret(const std::string &arg) { emit(bril::Instruction::ret(arg)); }
  void constant(const std::string &destination, const std::string &value) {
    emit(bril::Instruction::constant(destination, value));
  }
  void id(const std::string &destination, const std::string &value) {
    emit(bril::Instruction::id(destination, value));
  }
  void print(const std::string &value) {
    emit(bril::Instruction::print(value));
  }
  void nop() { emit(bril::Instruction::nop()); }
  void alloc(const std::string &destination, const std::string &argument) {
    emit(bril::Instruction::alloc(destination, argument));
  }
  void free(const std::string &argument) {
    emit(bril::Instruction::free(argument));
  }
  void store(const std::string &destination, const std::string &argument) {
    emit(bril::Instruction::store(destination, argument));
  }
  void load(const std::string &destination, const std::string &argument) {
    emit(bril::Instruction::load(destination, argument));
  }
  void ptradd(const std::string &destination, const std::string &lhs,
              const std::string &rhs) {
    emit(bril::Instruction::ptradd(destination, lhs, rhs));
  }
  void addressof(const std::string &destination, const std::string &argument) {
    emit(bril::Instruction::addressof(destination, argument));
  }
  void label(const std::string &label_value) {
    emit(bril::Instruction::label(label_value));
  }
  void phi(const std::string &destination,
           const std::vector<std::string> &values,
           const std::vector<std::string> &labels) {
    emit(bril::Instruction::phi(destination, values, labels));
  }
};
