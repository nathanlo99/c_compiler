
#pragma once

#include "data_flow/data_flow.hpp"
#include "util.hpp"

namespace bril {

struct MemoryLocation {
  struct MemoryLocationHasher {
    size_t operator()(const MemoryLocation &loc) const {
      return std::hash<Type>()(loc.type) ^ std::hash<std::string>()(loc.name) ^
             std::hash<size_t>()(loc.instruction_idx);
    }
  };
  using Set = std::unordered_set<MemoryLocation, MemoryLocationHasher>;
  enum class Type { AddressOf, Allocation, Parameter, RawPointer };

  Type type;
  std::string name;
  size_t instruction_idx;
  int64_t raw_value;

  MemoryLocation(const Type type, const std::string &name,
                 const size_t instruction_idx, const int64_t raw_value)
      : type(type), name(name), instruction_idx(instruction_idx),
        raw_value(raw_value) {}

  static MemoryLocation address_of(const std::string &name) {
    return MemoryLocation(Type::AddressOf, name, 0, 0);
  }
  static MemoryLocation allocation(const std::string &label,
                                   const size_t instruction_idx) {
    return MemoryLocation(Type::Allocation, label, instruction_idx, 0);
  }
  static MemoryLocation parameter(const size_t parameter_idx) {
    return MemoryLocation(Type::Parameter, "__param", parameter_idx, 0);
  }
  static MemoryLocation raw_pointer(int64_t value) {
    return MemoryLocation(Type::RawPointer, "", 0, value);
  }

  bool operator==(const MemoryLocation &other) const {
    return type == other.type && name == other.name &&
           instruction_idx == other.instruction_idx;
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const bril::MemoryLocation &location) {
    switch (location.type) {
    case bril::MemoryLocation::Type::AddressOf:
      return os << "&" << location.name;
    case bril::MemoryLocation::Type::Allocation:
      return os << "Alloc @ (" << location.name << ", "
                << location.instruction_idx << ")";
    case bril::MemoryLocation::Type::Parameter:
      return os << "Param @ " << location.instruction_idx;
    case bril::MemoryLocation::Type::RawPointer:
      return os << "0x" << std::hex << location.raw_value << std::dec;
    }
    return os;
  }
};

struct MayAliasAnalysis
    : ForwardDataFlowPass<
          std::unordered_map<std::string, MemoryLocation::Set>> {
  using Result = std::unordered_map<std::string, MemoryLocation::Set>;

  Result _init;
  MayAliasAnalysis(const ControlFlowGraph &function)
      : ForwardDataFlowPass<Result>(function) {
    for (size_t i = 0; i < function.arguments.size(); ++i) {
      const auto &argument = function.arguments[i];
      if (argument.type == Type::IntStar)
        _init[argument.name] = {MemoryLocation::parameter(i)};
    }
  }

  Result init() override { return _init; }
  Result merge(const std::vector<Result> &args) override {
    Result result;
    for (const auto &arg : args) {
      for (const auto &[variable, locations] : arg) {
        result[variable].insert(locations.begin(), locations.end());
      }
    }
    return result;
  }

  Result transfer(const Result &in, const InstructionLocation &location,
                  const Instruction &instruction) override {
    const std::string &label = location.label;
    const size_t instruction_idx = location.instruction_idx;

    // If the instruction doesn't assign to a variable, just pass the input
    if (instruction.destination == "")
      return in;

    // If the instruction produces an integer, the variable cannot refer to a
    // memory location
    if (instruction.type == Type::Int) {
      Result result = in;
      result[instruction.destination] = {};
      return result;
    }

    Result result = in;
    switch (instruction.opcode) {
    // If we get here, the constant expression must assign to a constant (a raw
    // pointer)
    case Opcode::Const:
      result[instruction.destination] = {
          MemoryLocation::raw_pointer(instruction.value)};
      break;

    // If we get here, the function call must have produced a pointer
    case Opcode::Call:
      result[instruction.destination] = {
          MemoryLocation::allocation(label, instruction_idx)};
      break;

    case Opcode::Id:
      result[instruction.destination] = result[instruction.arguments[0]];
      break;

    case Opcode::Alloc:
      result[instruction.destination] = {
          MemoryLocation::allocation(label, instruction_idx)};
      break;

    case Opcode::PointerAdd:
    case Opcode::PointerSub:
      result[instruction.destination] = result[instruction.arguments[0]];
      break;

    case Opcode::AddressOf:
      result[instruction.destination] = {
          MemoryLocation::address_of(instruction.arguments[0])};
      break;

    // The origin of a phi node is the union of all its arguments
    case Opcode::Phi:
      result[instruction.destination] = {};
      for (const auto &argument : instruction.arguments) {
        const auto &locations = result[argument];
        result[instruction.destination].insert(locations.begin(),
                                               locations.end());
      }
      break;

    default:
      std::cerr << "Instruction: " << instruction << std::endl;
      unreachable(fmt::format("Opcode {} not handled in alias analysis",
                              static_cast<int>(instruction.opcode)));
    }
    return result;
  }
};

} // namespace bril
