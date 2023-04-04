
#include "ast_node.hpp"
#include "types.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace bril {

struct Program;
struct Function;
struct Instruction;

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
  std::string label; // Only if opcode == Label
  std::string destination;
  Type type;

  std::vector<std::string> arguments;
  std::vector<std::string> funcs;
  std::vector<std::string> labels;

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
  std::vector<std::shared_ptr<Instruction>> instructions;

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
      if (instruction->opcode == Opcode::Label) {
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
