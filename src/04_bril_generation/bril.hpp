
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
  Unknown,
};

constexpr Type type_from_ast_type(const ::Type type) {
  switch (type) {
  case ::Type::Int:
    return Type::Int;
  case ::Type::IntStar:
    return Type::IntStar;
  case ::Type::Unknown:
    return Type::Void;
  }
}

inline std::ostream &operator<<(std::ostream &os, const Type type) {
  switch (type) {
  case Type::Void:
    os << "void";
    break;
  case Type::Bool:
    os << "bool";
    break;
  case Type::Int:
    os << "int";
    break;
  case Type::IntStar:
    os << "ptr<int>";
    break;
  case Type::Unknown:
    os << "?";
    break;
  }
  return os;
}

struct Variable {
  std::string name;
  Type type;

  Variable(const std::string &name, const Type type) : name(name), type(type) {}
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
  Type type = Type::Void;
  std::string destination;

  std::vector<std::string> arguments;
  std::vector<std::string> funcs;
  std::vector<std::string> labels;

  Instruction(const Opcode opcode, const std::string &destination,
              const Type type, const std::vector<std::string> &arguments,
              const std::vector<std::string> &funcs,
              const std::vector<std::string> &labels)
      : opcode(opcode), type(type), destination(destination),
        arguments(arguments), funcs(funcs), labels(labels) {}

  Instruction(const Opcode opcode, const Type type,
              const std::string &destination,
              const std::vector<std::string> &arguments)
      : opcode(opcode), type(type), destination(destination),
        arguments(arguments) {}

  inline bool is_jump() const {
    return opcode == Opcode::Jmp || opcode == Opcode::Br;
  }

  static inline Instruction add(const std::string &dest, const std::string &lhs,
                                const std::string &rhs) {
    return Instruction(Opcode::Add, Type::Int, dest, {lhs, rhs});
  }
  static inline Instruction sub(const std::string &dest, const std::string &lhs,
                                const std::string &rhs) {
    return Instruction(Opcode::Sub, Type::Int, dest, {lhs, rhs});
  }
  static inline Instruction mul(const std::string &dest, const std::string &lhs,
                                const std::string &rhs) {
    return Instruction(Opcode::Mul, Type::Int, dest, {lhs, rhs});
  }
  static inline Instruction div(const std::string &dest, const std::string &lhs,
                                const std::string &rhs) {
    return Instruction(Opcode::Div, Type::Int, dest, {lhs, rhs});
  }
  static inline Instruction mod(const std::string &dest, const std::string &lhs,
                                const std::string &rhs) {
    return Instruction(Opcode::Mod, Type::Int, dest, {lhs, rhs});
  }
  static inline Instruction lt(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Lt, Type::Bool, dest, {lhs, rhs});
  }
  static inline Instruction le(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Le, Type::Bool, dest, {lhs, rhs});
  }
  static inline Instruction gt(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Gt, Type::Bool, dest, {lhs, rhs});
  }
  static inline Instruction ge(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Ge, Type::Bool, dest, {lhs, rhs});
  }
  static inline Instruction eq(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Eq, Type::Bool, dest, {lhs, rhs});
  }
  static inline Instruction ne(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Ne, Type::Bool, dest, {lhs, rhs});
  }
  static inline Instruction jmp(const std::string &dest) {
    return Instruction(Opcode::Jmp, "", Type::Void, {}, {}, {dest});
  }
  static inline Instruction br(const std::string &dest,
                               const std::string &true_label,
                               const std::string &false_label) {
    return Instruction(Opcode::Br, "", Type::Void, {dest}, {},
                       {true_label, false_label});
  }
  static inline Instruction call(const std::string &destination,
                                 const std::string &function,
                                 const std::vector<std::string> &arguments) {
    return Instruction(Opcode::Call, destination, Type::Int, arguments,
                       {function}, {});
  }
  static inline Instruction ret(const std::string &arg) {
    return Instruction(Opcode::Ret, Type::Void, "", {arg});
  }
  static inline Instruction constant(const std::string &destination,
                                     const Literal &literal) {
    const Type type = type_from_ast_type(literal.type);
    return Instruction(Opcode::Const, type, destination,
                       {std::to_string(literal.value)});
  }
  static inline Instruction id(const std::string &destination,
                               const std::string &value, const Type type) {
    return Instruction(Opcode::Id, type, destination, {value});
  }
  static inline Instruction print(const std::string &value) {
    return Instruction(Opcode::Print, Type::Void, "", {value});
  }
  static inline Instruction nop() {
    return Instruction(Opcode::Nop, Type::Void, "", {});
  }
  static inline Instruction alloc(const std::string &destination,
                                  const std::string &argument) {
    return Instruction(Opcode::Alloc, Type::IntStar, destination, {argument});
  }
  static inline Instruction free(const std::string &argument) {
    return Instruction(Opcode::Free, Type::Void, "", {argument});
  }
  static inline Instruction store(const std::string &destination,
                                  const std::string &argument) {
    return Instruction(Opcode::Store, Type::Void, "", {destination, argument});
  }
  static inline Instruction load(const std::string &destination,
                                 const std::string &argument) {
    return Instruction(Opcode::Load, Type::Int, destination, {argument});
  }
  static inline Instruction ptradd(const std::string &destination,
                                   const std::string &lhs,
                                   const std::string &rhs) {
    return Instruction(Opcode::PointerAdd, Type::IntStar, destination,
                       {lhs, rhs});
  }
  static inline Instruction addressof(const std::string &destination,
                                      const std::string &argument) {
    return Instruction(Opcode::AddressOf, Type::IntStar, destination,
                       {argument});
  }
  static inline Instruction label(const std::string &label_value) {
    return Instruction(Opcode::Label, "", Type::Void, {}, {}, {label_value});
  }
  static inline Instruction phi(const std::string &destination,
                                const std::vector<std::string> &values,
                                const std::vector<std::string> &labels) {
    return Instruction(Opcode::Phi, destination, Type::Void, values, {},
                       labels);
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const Instruction &instruction) {
    switch (instruction.opcode) {
    case Opcode::Add:
      os << instruction.destination << ": " << instruction.type << " = add "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;
    case Opcode::Sub:
      os << instruction.destination << ": " << instruction.type << " = sub "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;
    case Opcode::Mul:
      os << instruction.destination << ": " << instruction.type << " = mul "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;
    case Opcode::Div:
      os << instruction.destination << ": " << instruction.type << " = div "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;
    case Opcode::Mod:
      os << instruction.destination << ": " << instruction.type << " = mod "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;

    case Opcode::Lt:
      os << instruction.destination << ": " << instruction.type << " = lt "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;
    case Opcode::Le:
      os << instruction.destination << ": " << instruction.type << " = le "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;
    case Opcode::Gt:
      os << instruction.destination << ": " << instruction.type << " = gt "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;
    case Opcode::Ge:
      os << instruction.destination << ": " << instruction.type << " = ge "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;
    case Opcode::Eq:
      os << instruction.destination << ": " << instruction.type << " = eq "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;
    case Opcode::Ne:
      os << instruction.destination << ": " << instruction.type << " = ne "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;

    case Opcode::Jmp:
      os << "jmp " << instruction.labels[0] << ";";
      break;
    case Opcode::Br:
      os << "br " << instruction.arguments[0] << " " << instruction.labels[0]
         << " " << instruction.labels[1] << ";";
      break;
    case Opcode::Call:
      os << "call " << instruction.funcs[0];
      for (const auto &argument : instruction.arguments)
        os << " " << argument;
      os << ";";
      break;
    case Opcode::Ret:
      if (instruction.arguments.empty())
        os << "ret;";
      else
        os << "ret " << instruction.arguments[0] << ";";
      break;

    case Opcode::Const:
      os << instruction.destination << ": " << instruction.type << " = const "
         << instruction.arguments[0] << ";";
      break;
    case Opcode::Id:
      os << instruction.destination << ": " << instruction.type << " = id "
         << instruction.arguments[0] << ";";
      break;
    case Opcode::Print:
      os << "print " << instruction.arguments[0] << ";";
      break;
    case Opcode::Nop:
      os << "nop;";
      break;

    case Opcode::Alloc:
      os << instruction.destination << ": " << instruction.type << " = alloc "
         << instruction.arguments[0] << ";";
      break;
    case Opcode::Free:
      os << "free " << instruction.arguments[0] << ";";
      break;
    case Opcode::Store:
      os << "store " << instruction.arguments[0] << " "
         << instruction.arguments[1] << ";";
      break;
    case Opcode::Load:
      os << instruction.destination << ": " << instruction.type << " = load "
         << instruction.arguments[0] << ";";
      break;
    case Opcode::PointerAdd:
      os << instruction.destination << ": " << instruction.type << " = ptradd "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;
    case Opcode::AddressOf:
      os << instruction.destination << ": " << instruction.type
         << " = addressof " << instruction.arguments[0] << ";";
      break;

    case Opcode::Label:
      os << instruction.labels[0] << ":";
      break;

    case Opcode::Phi:
      os << instruction.destination << ": " << instruction.type << " = phi ";
      const size_t n = instruction.arguments.size();
      for (size_t i = 0; i < n; ++i) {
        os << " " << instruction.labels[i] << " " << instruction.arguments[i];
      }
      os << ";";
      break;
    }
    return os;
  }
};

struct Function {
  std::string name;
  std::vector<bril::Variable> arguments;
  Type return_type;
  std::vector<bril::Instruction> instructions;

  Function(const std::string &name, const std::vector<Variable> &arguments,
           const Type return_type)
      : name("@" + name), arguments(arguments), return_type(return_type) {}

  friend std::ostream &operator<<(std::ostream &os, const Function &function) {
    os << function.name;
    if (function.arguments.size() > 0) {
      os << "(";
      bool first = true;
      for (const auto &argument : function.arguments) {
        if (first)
          first = false;
        else
          os << ", ";
        os << argument.name << ": " << argument.type;
      }
      os << ")";
    }
    os << ": " << function.return_type << " {" << std::endl;

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

struct Block {
  size_t idx;
  std::vector<std::string> entry_labels;
  std::vector<Instruction> instructions;
  std::vector<std::string> exit_labels;

  std::set<size_t> incoming_blocks;
  std::set<size_t> outgoing_blocks;

  friend std::ostream &operator<<(std::ostream &os, const Block &block) {
    if (!block.incoming_blocks.empty()) {
      os << "incoming_blocks: [";
      for (const auto &block_idx : block.incoming_blocks) {
        os << block_idx << ", ";
      }
      os << "\b\b]" << std::endl;
    }
    if (!block.outgoing_blocks.empty()) {
      os << "outgoing_blocks: [";
      for (const auto &block_idx : block.outgoing_blocks) {
        os << block_idx << ", ";
      }
      os << "\b\b]" << std::endl;
    }

    os << "instructions: " << std::endl;
    for (const auto &instruction : block.instructions) {
      os << "  " << instruction << std::endl;
    }
    return os;
  }
};

// Stores the CFG for a single procedure
struct ControlFlowGraph {
  std::string name;
  std::vector<Block> blocks;
  std::set<size_t> exiting_blocks;

  ControlFlowGraph(const Function &function);

  void add_block(const Block &block) {
    if (block.instructions.empty())
      return;
    blocks.push_back(block);
    blocks.back().idx = blocks.size() - 1;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const ControlFlowGraph &graph) {
    const std::string separator = std::string(80, '-');
    os << "CFG for " << graph.name << std::endl;
    for (size_t i = 0; i < graph.blocks.size(); ++i) {
      os << separator << std::endl;
      os << "index: " << i << std::endl;
      os << graph.blocks[i];
    }
    os << separator << std::endl;
    os << "exiting blocks: [";
    for (size_t exiting_block : graph.exiting_blocks) {
      os << exiting_block << ", ";
    }
    os << "\b\b]" << std::endl;
    return os;
  }

private:
  void add_directed_edge(const size_t source, const size_t target);
  void compute_edges();
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
