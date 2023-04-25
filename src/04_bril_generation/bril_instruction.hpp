
#include "ast_node.hpp"
#include "util.hpp"

namespace bril {
enum class Type {
  Void,
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
  default:
    unreachable("");
    return Type::Unknown;
  }
}

inline std::ostream &operator<<(std::ostream &os, const Type type) {
  switch (type) {
  case Type::Void:
    os << "void";
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
  default:
    debug_assert(false, "Unknown type in operator<<");
  }
  return os;
}

struct Variable {
  std::string name;
  Type type;

  Variable(const std::string &name, const Type type) : name(name), type(type) {}

  friend std::ostream &operator<<(std::ostream &os, const Variable &variable) {
    return os << variable.name << ": " << variable.type;
  }
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
  PointerSub,
  PointerDiff,
  AddressOf,

  // Label
  Label,

  // SSA
  Phi,
};

inline std::ostream &operator<<(std::ostream &os, const Opcode opcode) {
  switch (opcode) {
  case Opcode::Add:
    return os << "add";
  case Opcode::Sub:
    return os << "sub";
  case Opcode::Mul:
    return os << "mul";
  case Opcode::Div:
    return os << "div";
  case Opcode::Mod:
    return os << "mod";
  case Opcode::Lt:
    return os << "lt";
  case Opcode::Le:
    return os << "le";
  case Opcode::Gt:
    return os << "gt";
  case Opcode::Ge:
    return os << "ge";
  case Opcode::Eq:
    return os << "eq";
  case Opcode::Ne:
    return os << "ne";
  case Opcode::Jmp:
    return os << "jmp";
  case Opcode::Br:
    return os << "br";
  case Opcode::Call:
    return os << "call";
  case Opcode::Ret:
    return os << "ret";
  case Opcode::Const:
    return os << "const";
  case Opcode::Id:
    return os << "id";
  case Opcode::Print:
    return os << "print";
  case Opcode::Nop:
    return os << "nop";
  case Opcode::Alloc:
    return os << "alloc";
  case Opcode::Free:
    return os << "free";
  case Opcode::Store:
    return os << "store";
  case Opcode::Load:
    return os << "load";
  case Opcode::PointerAdd:
    return os << "ptradd";
  case Opcode::PointerSub:
    return os << "ptrsub";
  case Opcode::PointerDiff:
    return os << "ptrdiff";
  case Opcode::AddressOf:
    return os << "addressof";
  case Opcode::Label:
    return os << "label";
  case Opcode::Phi:
    return os << "phi";
  default:
    debug_assert(false, "Unknown opcode type in operator<<");
  }
  return os;
}

struct Instruction {
  Opcode opcode;
  Type type = Type::Void;
  std::string destination;

  int64_t value;
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

  Instruction(const std::string &destination, const int64_t value,
              const Type type)
      : opcode(Opcode::Const), type(type), destination(destination),
        value(value) {}

  inline bool is_pure() const {
    return opcode != Opcode::Call && opcode != Opcode::Print &&
           opcode != Opcode::Alloc && opcode != Opcode::Free &&
           opcode != Opcode::Load && opcode != Opcode::Store;
  }

  inline bool is_jump() const {
    return opcode == Opcode::Jmp || opcode == Opcode::Br ||
           opcode == Opcode::Ret;
  }
  inline bool uses_memory() const {
    return opcode == Opcode::Alloc || opcode == Opcode::Free ||
           opcode == Opcode::Store || opcode == Opcode::Load ||
           opcode == Opcode::PointerAdd || opcode == Opcode::PointerSub ||
           opcode == Opcode::PointerDiff || opcode == Opcode::AddressOf;
  }
  inline bool is_load_or_store() const {
    return opcode == Opcode::Load || opcode == Opcode::Store;
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
    return Instruction(Opcode::Lt, Type::Int, dest, {lhs, rhs});
  }
  static inline Instruction le(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Le, Type::Int, dest, {lhs, rhs});
  }
  static inline Instruction gt(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Gt, Type::Int, dest, {lhs, rhs});
  }
  static inline Instruction ge(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Ge, Type::Int, dest, {lhs, rhs});
  }
  static inline Instruction eq(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Eq, Type::Int, dest, {lhs, rhs});
  }
  static inline Instruction ne(const std::string &dest, const std::string &lhs,
                               const std::string &rhs) {
    return Instruction(Opcode::Ne, Type::Int, dest, {lhs, rhs});
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
                                 const std::vector<std::string> &arguments,
                                 const Type type) {
    return Instruction(Opcode::Call, destination, type, arguments, {function},
                       {});
  }
  static inline Instruction ret(const std::string &arg) {
    return Instruction(Opcode::Ret, Type::Void, "", {arg});
  }
  static inline Instruction constant(const std::string &destination,
                                     const int64_t value, const Type type) {
    return Instruction(destination, value, type);
  }
  static inline Instruction constant(const std::string &destination,
                                     const Literal &literal) {
    const Type type = type_from_ast_type(literal.type);
    return Instruction(destination, literal.value, type);
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
  static inline Instruction ptrsub(const std::string &destination,
                                   const std::string &lhs,
                                   const std::string &rhs) {
    return Instruction(Opcode::PointerSub, Type::IntStar, destination,
                       {lhs, rhs});
  }
  static inline Instruction ptrdiff(const std::string &destination,
                                    const std::string &lhs,
                                    const std::string &rhs) {
    return Instruction(Opcode::PointerDiff, Type::Int, destination, {lhs, rhs});
  }
  static inline Instruction addressof(const std::string &destination,
                                      const std::string &argument) {
    return Instruction(Opcode::AddressOf, Type::IntStar, destination,
                       {argument});
  }
  static inline Instruction label(const std::string &label_value) {
    return Instruction(Opcode::Label, "", Type::Void, {}, {}, {label_value});
  }
  static inline Instruction phi(const std::string &destination, const Type type,
                                const std::vector<std::string> &values,
                                const std::vector<std::string> &labels) {
    return Instruction(Opcode::Phi, destination, type, values, {}, labels);
  }

  inline std::string to_string() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
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
      os << instruction.destination << ": " << instruction.type << " = call @"
         << instruction.funcs[0];
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

    case Opcode::Const: {
      const std::string value_string = std::to_string(instruction.value);
      os << instruction.destination << ": " << instruction.type << " = const "
         << value_string << ";";
    } break;
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
    case Opcode::PointerSub:
      os << instruction.destination << ": " << instruction.type << " = ptrsub "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;
    case Opcode::PointerDiff:
      os << instruction.destination << ": " << instruction.type << " = ptrdiff "
         << instruction.arguments[0] << " " << instruction.arguments[1] << ";";
      break;
    case Opcode::AddressOf:
      os << instruction.destination << ": " << instruction.type
         << " = addressof " << instruction.arguments[0] << ";";
      break;

    case Opcode::Label:
      os << instruction.labels[0] << ":";
      break;

    case Opcode::Phi: {
      os << instruction.destination << ": " << instruction.type << " = phi";
      const size_t n = instruction.arguments.size();
      for (size_t i = 0; i < n; ++i) {
        os << " " << instruction.labels[i] << " " << instruction.arguments[i];
      }
      os << ";";
    } break;

    default:
      unreachable("");
    }
    return os;
  }
};

} // namespace bril
