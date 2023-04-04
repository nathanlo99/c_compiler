
#include "ast_node.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace bril {

struct Program;
struct Function;
struct Instruction;

enum class Type {
  Void,
  Bool,
  Int,
  IntStar,
};

inline std::ostream &operator<<(std::ostream &os, const Type type) {
  switch (type) {
  case Type::Void:
    os << "void";
  case Type::Bool:
    os << "bool";
  case Type::Int:
    os << "int";
  case Type::IntStar:
    os << "ptr<int>";
  }
  return os;
}

struct Variable {
  std::string name;
  Type type;
};

enum class Opcode {
  // Core BRIL
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Lt,
  Le,
  Gt,
  Ge,
  Eq,
  Ne,
  Jmp,
  Br,
  Call,
  Ret,
  Const,
  Id,
  Print,
  Nop,

  // Memory BRIL
  Alloc,
  Free,
  Store,
  Load,
  PointerAdd,
  AddressOf,

  // Label
  Label,

  // SSA
  Phi,
};

struct Instruction {
  Opcode opcode;
  std::string label_value; // Only if opcode == Label
  std::string destination;
  Type type;

  std::vector<std::string> arguments;
  std::vector<std::string> funcs;
  std::vector<std::string> labels;

  Instruction(const Opcode opcode, const std::string &label,
              const std::string &destination, const Type type,
              const std::vector<std::string> &arguments,
              const std::vector<std::string> &funcs,
              const std::vector<std::string> &labels)
      : opcode(opcode), label_value(label), destination(destination),
        type(type), arguments(arguments), funcs(funcs), labels(labels) {}

  Instruction(const Opcode opcode, const std::string &destination,
              const std::vector<std::string> &arguments)
      : opcode(opcode), destination(destination), arguments(arguments) {}

  static inline Instruction add(const std::string &dest, const std::string &lhs,
                                const std::string &rhs) {
    return Instruction(Opcode::Add, dest, {lhs, rhs});
  }
  static inline Instruction sub(const std::string &dest, const std::string &lhs,
                                const std::string &rhs) {
    return Instruction(Opcode::Sub, dest, {lhs, rhs});
  }
  static inline Instruction mul(const std::string &dest, const std::string &lhs,
                                const std::string &rhs) {
    return Instruction(Opcode::Mul, dest, {lhs, rhs});
  }
  static inline Instruction div(const std::string &dest, const std::string &lhs,
                                const std::string &rhs) {
    return Instruction(Opcode::Div, dest, {lhs, rhs});
  }
  static inline Instruction mod(const std::string &dest, const std::string &lhs,
                                const std::string &rhs) {
    return Instruction(Opcode::Mod, dest, {lhs, rhs});
  }
  static inline Instruction lt(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Lt, dest, {lhs, rhs});
  }
  static inline Instruction le(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Le, dest, {lhs, rhs});
  }
  static inline Instruction gt(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Gt, dest, {lhs, rhs});
  }
  static inline Instruction ge(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Ge, dest, {lhs, rhs});
  }
  static inline Instruction eq(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Eq, dest, {lhs, rhs});
  }
  static inline Instruction ne(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Ne, dest, {lhs, rhs});
  }
  static inline Instruction jmp(const std::string &dest) {
    return Instruction(Opcode::Jmp, dest, "", Type::Void, {}, {}, {});
  }
  static inline Instruction br(const std::string &dest,
                               const std::string &true_label,
                               const std::string &false_label) {
    return Instruction(Opcode::Br, dest, {true_label, false_label});
  }
  static inline Instruction call(const std::string &destination,
                                 const std::string &function,
                                 const std::vector<std::string> &arguments) {
    return Instruction(Opcode::Call, "", destination, Type::Int, arguments,
                       {function}, {});
  }
  static inline Instruction ret(const std::string &arg) {
    return Instruction(Opcode::Ret, "", {arg});
  }
  static inline Instruction constant(const std::string &destination,
                                     const std::string &value) {
    return Instruction(Opcode::Const, destination, {value});
  }
  static inline Instruction id(const std::string &destination,
                               const std::string &value) {
    return Instruction(Opcode::Id, destination, {value});
  }
  static inline Instruction print(const std::string &value) {
    return Instruction(Opcode::Print, "", {value});
  }
  static inline Instruction nop() { return Instruction(Opcode::Nop, "", {}); }
  static inline Instruction alloc(const std::string &destination,
                                  const std::string &argument) {
    return Instruction(Opcode::Alloc, destination, {argument});
  }
  static inline Instruction free(const std::string &argument) {
    return Instruction(Opcode::Free, "", {argument});
  }
  static inline Instruction store(const std::string &destination,
                                  const std::string &argument) {
    return Instruction(Opcode::Store, "", {destination, argument});
  }
  static inline Instruction load(const std::string &destination,
                                 const std::string &argument) {
    return Instruction(Opcode::Load, destination, {argument});
  }
  static inline Instruction ptradd(const std::string &destination,
                                   const std::string &lhs,
                                   const std::string &rhs) {
    return Instruction(Opcode::PointerAdd, destination, {lhs, rhs});
  }
  static inline Instruction addressof(const std::string &destination,
                                      const std::string &argument) {
    return Instruction(Opcode::AddressOf, destination, {argument});
  }
  static inline Instruction label(const std::string &label_value) {
    return Instruction(Opcode::Label, label_value, "", Type::Void, {}, {}, {});
  }
  static inline Instruction phi(const std::string &destination,
                                const std::vector<std::string> &values,
                                const std::vector<std::string> &labels) {
    return Instruction(Opcode::Phi, "", destination, Type::Void, values, {},
                       labels);
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const Instruction &instruction) {
    switch (instruction.opcode) {
    case Opcode::Add:
      os << instruction.destination << " : " << instruction.type << " = add "
         << instruction.arguments[0] << " " << instruction.arguments[1];
      break;
    case Opcode::Sub:
      os << instruction.destination << " : " << instruction.type << " = sub "
         << instruction.arguments[0] << " " << instruction.arguments[1];
      break;
    case Opcode::Mul:
      os << instruction.destination << " : " << instruction.type << " = mul "
         << instruction.arguments[0] << " " << instruction.arguments[1];
      break;
    case Opcode::Div:
      os << instruction.destination << " : " << instruction.type << " = div "
         << instruction.arguments[0] << " " << instruction.arguments[1];
      break;
    case Opcode::Mod:
      os << instruction.destination << " : " << instruction.type << " = mod "
         << instruction.arguments[0] << " " << instruction.arguments[1];
      break;

    case Opcode::Lt:
      os << instruction.destination << " : " << instruction.type << " = lt "
         << instruction.arguments[0] << " " << instruction.arguments[1];
      break;
    case Opcode::Le:
      os << instruction.destination << " : " << instruction.type << " = le "
         << instruction.arguments[0] << " " << instruction.arguments[1];
      break;
    case Opcode::Gt:
      os << instruction.destination << " : " << instruction.type << " = gt "
         << instruction.arguments[0] << " " << instruction.arguments[1];
      break;
    case Opcode::Ge:
      os << instruction.destination << " : " << instruction.type << " = ge "
         << instruction.arguments[0] << " " << instruction.arguments[1];
      break;
    case Opcode::Eq:
      os << instruction.destination << " : " << instruction.type << " = eq "
         << instruction.arguments[0] << " " << instruction.arguments[1];
      break;
    case Opcode::Ne:
      os << instruction.destination << " : " << instruction.type << " = ne "
         << instruction.arguments[0] << " " << instruction.arguments[1];
      break;

    case Opcode::Jmp:
      os << "jmp " << instruction.labels[0];
      break;
    case Opcode::Br:
      os << "br " << instruction.arguments[0] << " " << instruction.labels[0]
         << " " << instruction.labels[1];
      break;
    case Opcode::Call:
      os << "call " << instruction.funcs[0];
      for (const auto &argument : instruction.arguments)
        os << " " << argument;
      break;
    case Opcode::Ret:
      if (instruction.arguments.empty())
        os << "ret";
      else
        os << "ret " << instruction.arguments[0];
      break;

    case Opcode::Const:
      os << instruction.destination << " : " << instruction.type << " = const "
         << instruction.arguments[0];
      break;
    case Opcode::Id:
      os << instruction.destination << " : " << instruction.type << " = id "
         << instruction.arguments[0];
      break;
    case Opcode::Print:
      os << "print " << instruction.arguments[0];
      break;
    case Opcode::Nop:
      os << "nop";
      break;

    case Opcode::Alloc:
      os << instruction.destination << " : " << instruction.type << " = alloc "
         << instruction.arguments[0];
      break;
    case Opcode::Free:
      os << "free " << instruction.arguments[0];
      break;
    case Opcode::Store:
      os << "store " << instruction.arguments[0] << " "
         << instruction.arguments[1];
      break;
    case Opcode::Load:
      os << instruction.destination << " : " << instruction.type << " = load "
         << instruction.arguments[0];
      break;
    case Opcode::PointerAdd:
      os << instruction.destination << " : " << instruction.type << " = ptradd "
         << instruction.arguments[0] << " " << instruction.arguments[1];
      break;
    case Opcode::AddressOf:
      os << instruction.destination << " : " << instruction.type
         << " = addressof " << instruction.arguments[0];
      break;

    case Opcode::Label:
      os << "." << instruction.labels[0];
      break;

    case Opcode::Phi:
      os << instruction.destination << " : " << instruction.type << " = phi ";
      const size_t n = instruction.arguments.size();
      for (size_t i = 0; i < n; ++i) {
        os << " " << instruction.labels[i] << " " << instruction.arguments[i];
      }
      break;
    }
    return os;
  }
};

struct Function {
  std::string procedure_name;
  std::vector<Variable> arguments;
  Type return_type;
  std::vector<Instruction> instructions;

  friend std::ostream &operator<<(std::ostream &os, const Function &function) {
    os << function.procedure_name;
    if (function.arguments.size() > 0) {
      os << "(";
      bool first = true;
      for (const auto &argument : function.arguments) {
        if (!first)
          first = true;
        else
          os << ", ";
        os << argument.name << ": " << argument.type;
      }
      os << ")";
    }
    os << " : " << function.return_type << " {" << std::endl;

    for (const auto &instruction : function.instructions) {
      if (instruction.opcode == Opcode::Label) {
        os << instruction << std::endl;
      } else {
        os << "  " << instruction << std::endl;
      }
    }

    os << "}" << std::endl;
    return os;
  }
};

struct Program {
  std::vector<Function> functions;

  friend std::ostream &operator<<(std::ostream &os, const Program &program) {
    for (const auto &function : program.functions) {
      os << function << std::endl;
    }
    return os;
  }
};

} // namespace bril
