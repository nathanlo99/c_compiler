
#pragma once

#include "ast_node.hpp"
#include "util.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace bril {

struct Program;
struct Function;
struct Instruction;

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

  int value;
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

  Instruction(const std::string &destination, const int value, const Type type)
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
                                     const int value, const Type type) {
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

struct Function {
  std::string name;
  std::vector<bril::Variable> arguments;
  Type return_type;
  std::vector<bril::Instruction> instructions;

  Function(const std::string &name, const std::vector<Variable> &arguments,
           const Type return_type)
      : name(name), arguments(arguments), return_type(return_type) {}
};

struct ControlFlowGraph;
struct Block {
  std::string entry_label;
  std::vector<Instruction> instructions;
  std::vector<std::string> exit_labels;

  std::set<std::string> incoming_blocks;
  std::set<std::string> outgoing_blocks;

  // Insert an instruction at the beginning of the block, after any labels.
  void prepend(const Instruction &instruction) {
    auto it = instructions.begin();
    while (it != instructions.end() && it->opcode == Opcode::Label)
      ++it;
    instructions.insert(it, instruction);
  }

  template <typename Pred> bool all_of(const Pred &pred) const {
    return std::all_of(instructions.begin(), instructions.end(), pred);
  }
  template <typename Pred> bool any_of(const Pred &pred) const {
    return std::any_of(instructions.begin(), instructions.end(), pred);
  }

  // Returns true if the block has any instructions other than labels.
  bool has_instructions() const {
    return any_of([](const auto &instruction) {
      return instruction.opcode != Opcode::Label;
    });
  }

  bool uses_pointers() const {
    return any_of(
        [](const auto &instruction) { return instruction.uses_memory(); });
  }

  bool has_loads_or_stores() const {
    return any_of(
        [](const auto &instruction) { return instruction.is_load_or_store(); });
  }

  friend std::ostream &operator<<(std::ostream &os, const Block &block) {
    using util::operator<<;
    if (!block.incoming_blocks.empty()) {
      os << "incoming_blocks: " << block.incoming_blocks << std::endl;
    }
    if (!block.outgoing_blocks.empty()) {
      os << "outgoing_blocks: " << block.outgoing_blocks << std::endl;
    }

    os << "instructions: " << std::endl;
    for (const auto &instruction : block.instructions) {
      if (instruction.opcode == Opcode::Label)
        os << instruction.labels[0] << ":" << std::endl;
      else
        os << "  " << instruction << std::endl;
    }
    return os;
  }
};

// Stores the CFG for a single procedure
struct ControlFlowGraph {
  std::string name;
  std::vector<bril::Variable> arguments;
  Type return_type;

  std::vector<std::string> block_labels;
  std::string entry_label;
  std::map<std::string, Block> blocks;
  std::set<std::string> exiting_blocks;

  // Dominator data structures
  std::map<std::string, std::map<std::string, bool>> raw_dominators;
  std::map<std::string, std::set<std::string>> dominators;
  std::map<std::string, std::string> immediate_dominators;
  std::map<std::string, std::set<std::string>> dominance_frontiers;

  // True if the graph has been modified since the last time dominator data was
  // computed
  bool is_graph_dirty = true;

  // Construct a CFG from a function
  explicit ControlFlowGraph(const Function &function);

  std::string get_fresh_label(const std::string &prefix) const {
    if (blocks.count(prefix) == 0)
      return prefix;

    size_t idx = 0;
    while (true) {
      const std::string label = prefix + std::to_string(idx);
      if (blocks.count(label) == 0)
        return label;
      ++idx;
    }
  }

  Block &get_block(const std::string &block_label) {
    debug_assert(blocks.count(block_label) > 0, "Block not found: {}",
                 block_label);
    return blocks.at(block_label);
  }
  const Block &get_block(const std::string &block_label) const {
    debug_assert(blocks.count(block_label) > 0, "Block not found: {}",
                 block_label);
    return blocks.at(block_label);
  }
  void add_block(const Block &block);
  void remove_block(const std::string &block_label);
  void combine_blocks(const std::string &source, const std::string &target);
  std::string split_block(const std::string &block_label,
                          const size_t instruction_idx,
                          const std::string &new_label_hint = "splitLabel");

  void rename_label(const std::string &old_label, const std::string &new_label);

  template <typename Pred> bool all_of_blocks(const Pred &pred) const {
    return std::all_of(blocks.begin(), blocks.end(),
                       [&pred](const auto &pair) { return pred(pair.second); });
  }
  template <typename Pred> bool any_of_blocks(const Pred &pred) const {
    return std::any_of(blocks.begin(), blocks.end(),
                       [&pred](const auto &pair) { return pred(pair.second); });
  }

  bool uses_pointers() const {
    return any_of_blocks(
        [](const auto &block) { return block.uses_pointers(); });
  }

  bool uses_print() const {
    return any_of_blocks([](const auto &block) {
      return block.any_of([](const Instruction &instruction) {
        return instruction.opcode == Opcode::Print;
      });
    });
  }

  bool uses_heap() const {
    return any_of_blocks([](const Block &block) {
      return block.any_of([](const Instruction &instruction) {
        return instruction.opcode == Opcode::Alloc ||
               instruction.opcode == Opcode::Free;
      });
    });
  }

  bool has_phi_instructions() const {
    return any_of_blocks([](const Block &block) {
      return block.any_of([](const Instruction &instruction) {
        return instruction.opcode == Opcode::Phi;
      });
    });
  }

  // Convert the CFG to SSA form, if it has no memory accesses
  void convert_to_ssa();
  void convert_from_ssa();
  bool is_in_ssa_form() const;
  void
  rename_variables(const std::string &block_label,
                   std::map<std::string, std::vector<std::string>> definitions,
                   std::map<std::string, size_t> &next_idx);

  // Applies a local pass to each block in the CFG and returns the number of
  // removed lines
  template <typename Func> size_t apply_local_pass(const Func &func) {
    size_t num_removed_lines = 0;
    for (const auto &label : block_labels) {
      auto &block = get_block(label);
      num_removed_lines += func(*this, block);
    }
    recompute_graph();
    return num_removed_lines;
  }

  template <typename Func> void for_each_block(const Func &func) const {
    for (const auto &label : block_labels) {
      const auto &block = get_block(label);
      func(block);
    }
  }

  template <typename Func> void for_each_instruction(const Func &func) const {
    for (const auto &label : block_labels) {
      const auto &block = get_block(label);
      for (const auto &instruction : block.instructions) {
        func(instruction);
      }
    }
  }

  template <typename Func> void for_each_block(const Func &func) {
    for (const auto &label : block_labels) {
      auto &block = get_block(label);
      func(block);
    }
  }

  template <typename Func> void for_each_instruction(const Func &func) {
    for (const auto &label : block_labels) {
      auto &block = get_block(label);
      for (auto &instruction : block.instructions) {
        func(instruction);
      }
    }
  }

  std::vector<Instruction> flatten() const {
    std::vector<Instruction> instructions;
    for (const auto &label : block_labels) {
      const auto &block = get_block(label);
      instructions.insert(instructions.end(), block.instructions.begin(),
                          block.instructions.end());
    }
    return instructions;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const ControlFlowGraph &graph) {
    using util::operator<<;
    const std::string separator = std::string(80, '-');
    os << "CFG for " << graph.name << "(";

    bool first = true;
    for (const auto &argument : graph.arguments) {
      if (first)
        first = false;
      else
        os << ", ";
      os << argument.name << ": " << argument.type;
    }
    os << ") : " << graph.return_type << std::endl;

    graph.for_each_block([&](const Block &block) {
      os << separator << std::endl;
      os << "label: " << block.entry_label << std::endl;
      os << "immediate dominator: "
         << graph.immediate_dominator(block.entry_label) << std::endl;
      os << block;
    });
    os << separator << std::endl;
    os << "exiting blocks: " << graph.exiting_blocks << std::endl;
    os << separator << std::endl;
    return os;
  }

  void add_edge(const std::string &source, const std::string &target);
  void remove_edge(const std::string &source, const std::string &target);

  void compute_edges();
  void compute_dominators();
  void recompute_graph(const bool force = false) {
    if (!force && !is_graph_dirty)
      return;
    compute_edges();
    compute_dominators();
    is_graph_dirty = false;
  }

  std::string immediate_dominator(const std::string &label) const;
  std::set<std::string> dominance_frontier(const std::string &label) const;
};

struct Program {
  std::map<std::string, ControlFlowGraph> functions;

  const ControlFlowGraph &wain() const {
    debug_assert(functions.count("wain") > 0, "wain not found");
    return functions.at("wain");
  }

  ControlFlowGraph &get_function(const std::string &name) {
    debug_assert(functions.count(name) > 0, "Function {} not found", name);
    return functions.at(name);
  }
  const ControlFlowGraph &get_function(const std::string &name) const {
    debug_assert(functions.count(name) > 0, "Function {} not found", name);
    return functions.at(name);
  }

  void convert_to_ssa() {
    for_each_function(
        [](ControlFlowGraph &function) { function.convert_to_ssa(); });
  }
  void convert_from_ssa() {
    for_each_function(
        [](ControlFlowGraph &function) { function.convert_from_ssa(); });
  }

  bool has_phi_instructions() const {
    return std::any_of(
        functions.begin(), functions.end(),
        [](const auto &pair) { return pair.second.has_phi_instructions(); });
  }
  bool uses_heap() const {
    return std::any_of(
        functions.begin(), functions.end(),
        [](const auto &pair) { return pair.second.uses_heap(); });
  }
  bool uses_print() const {
    return std::any_of(
        functions.begin(), functions.end(),
        [](const auto &pair) { return pair.second.uses_print(); });
  }

  void inline_function_call(const std::string &function_name,
                            const std::string &block_label,
                            const size_t instruction_idx);

  friend std::ostream &operator<<(std::ostream &os, const Program &program) {
    program.for_each_function(
        [&os](const auto &function) { os << function << std::endl; });
    return os;
  }

  void print_flattened(std::ostream &os) const {
    using util::operator<<;
    for (const auto &[name, function] : functions) {
      os << "@" << name << "(";
      bool first = true;
      for (const auto &argument : function.arguments) {
        if (first)
          first = false;
        else
          os << ", ";
        os << argument.name << ": " << argument.type;
      }
      os << ") : " << function.return_type << " {" << std::endl;
      for (const auto &instruction : function.flatten()) {
        if (instruction.opcode == Opcode::Label) {
          const auto label = instruction.labels[0];
          const auto padding = 50 - label.size();
          os << instruction.labels[0] << ":" << std::string(padding, ' ')
             << "preds = " << function.get_block(label).incoming_blocks
             << std::endl;
        } else {
          os << "  " << instruction << std::endl;
        }
      }
      os << "}\n" << std::endl;
    }
  }

  template <typename Func> size_t apply_pass(const Func &func) {
    return func(*this);
  }

  template <typename Func> size_t apply_global_pass(const Func &func) {
    size_t num_removed_lines = 0;
    for (auto &[name, function] : functions)
      num_removed_lines += func(function);
    return num_removed_lines;
  }

  template <typename Func> size_t apply_local_pass(const Func &func) {
    size_t num_removed_lines = 0;
    for (auto &[name, function] : functions)
      num_removed_lines += function.apply_local_pass(func);
    return num_removed_lines;
  }

  template <typename Func> void for_each_function(const Func &func) const {
    for (auto &[name, function] : functions)
      func(function);
  }

  template <typename Func> void for_each_block(const Func &func) const {
    for_each_function([&](const ControlFlowGraph &function) {
      function.for_each_block(func);
    });
  }

  template <typename Func> void for_each_instruction(const Func &func) const {
    for_each_block([&](const Block &block) {
      for (const auto &instruction : block.instructions)
        func(instruction);
    });
  }

  template <typename Func> void for_each_function(const Func &func) {
    for (auto &[name, function] : functions)
      func(function);
  }

  template <typename Func> void for_each_block(const Func &func) {
    for_each_function(
        [&](ControlFlowGraph &function) { function.for_each_block(func); });
  }

  template <typename Func> void for_each_instruction(const Func &func) {
    for_each_block([&](Block &block) {
      for (auto &instruction : block.instructions)
        func(instruction);
    });
  }
};

} // namespace bril
