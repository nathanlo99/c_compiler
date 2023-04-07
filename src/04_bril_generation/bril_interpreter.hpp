
#pragma once

#include "bril.hpp"
#include "util.hpp"
#include <stdexcept>

namespace bril {
namespace interpreter {

struct BRILValue {
  enum class Type { Int, Bool, RawPointer, Address, HeapPointer, Void };

  // If the type is Int or Bool, the value is stored in int_value
  // If the type is Address, the name of the variable is stored in string_value
  // If the type is HeapPointer, the index of the heap memory is stored in
  // int_value
  Type type;
  std::string string_value;
  int int_value;
  size_t heap_idx, heap_offset;

  BRILValue() : type(Type::Void) {}
  BRILValue(const Type type, const int int_value,
            const std::string &string_value)
      : type(type), string_value(string_value), int_value(int_value) {}
  BRILValue(const size_t heap_idx, const size_t heap_offset)
      : type(Type::HeapPointer), heap_idx(heap_idx), heap_offset(heap_offset) {}

  static BRILValue integer(const int value) {
    return BRILValue(Type::Int, value, "");
  }
  static BRILValue boolean(const bool value) {
    return BRILValue(Type::Bool, value, "");
  }
  static BRILValue raw_pointer(const int value) {
    return BRILValue(Type::RawPointer, value, "");
  }
  static BRILValue address(const std::string &name) {
    return BRILValue(Type::Address, 0, name);
  }
  static BRILValue heap_pointer(const size_t idx, const size_t offset) {
    return BRILValue(idx, offset);
  }

  friend std::ostream &operator<<(std::ostream &os, const BRILValue &value) {
    switch (value.type) {
    case Type::Int:
      os << value.int_value << ": int";
      break;
    case Type::Bool:
      os << value.int_value << ": bool";
      break;
    case Type::RawPointer:
      os << value.int_value << ": int*";
      break;
    case Type::Address:
      os << "&" << value.string_value << ": int*";
      break;
    case Type::HeapPointer:
      os << "heap_alloc #" << value.heap_idx << " + " << value.heap_offset
         << ": int*";
      break;
    case Type::Void:
      os << "(void)";
      break;
    }
    return os;
  }

  friend bool operator<(const BRILValue &lhs, const BRILValue &rhs) {
    if (lhs.type != rhs.type)
      throw std::runtime_error("Cannot compare values of different types");
    switch (lhs.type) {
    case Type::Int:
      return lhs.int_value < rhs.int_value;
    case Type::Bool:
      throw std::runtime_error("Cannot compare booleans");
    case Type::RawPointer:
      return lhs.int_value < rhs.int_value;
    case Type::Address:
      if (lhs.string_value != rhs.string_value)
        throw std::runtime_error("Cannot compare addresses of different "
                                 "variables");
      return false;
    case Type::HeapPointer:
      if (lhs.heap_idx != rhs.heap_idx)
        throw std::runtime_error("Cannot compare heap pointers to different "
                                 "heap memory");
      return lhs.heap_offset < rhs.heap_offset;
    default:
      throw std::runtime_error("Unknown type");
    }
  }

  friend bool operator>(const BRILValue &lhs, const BRILValue &rhs) {
    return rhs < lhs;
  }
  friend bool operator==(const BRILValue &lhs, const BRILValue &rhs) {
    if (lhs.type != rhs.type)
      throw std::runtime_error("Cannot compare values of different types");
    switch (lhs.type) {
    case Type::Int:
    case Type::Bool:
      return lhs.int_value == rhs.int_value;
    case Type::RawPointer:
      return lhs.int_value == rhs.int_value;
    case Type::Address:
      return lhs.string_value == rhs.string_value;
    case Type::HeapPointer:
      return lhs.heap_idx == rhs.heap_idx && lhs.heap_offset == rhs.heap_offset;
    default:
      throw std::runtime_error("Unknown type");
    }
  }
  friend bool operator!=(const BRILValue &lhs, const BRILValue &rhs) {
    return !(lhs == rhs);
  }
  friend bool operator<=(const BRILValue &lhs, const BRILValue &rhs) {
    return lhs < rhs || lhs == rhs;
  }
  friend bool operator>=(const BRILValue &lhs, const BRILValue &rhs) {
    return lhs > rhs || lhs == rhs;
  }
};

struct BRILAlloc {
  std::vector<BRILValue> values;
  bool active;
};

struct BRILContext {
  std::map<std::string, BRILValue> variables;
  std::vector<BRILAlloc> heap_memory;

  // Get the value of a variable
  bool get_bool(const std::string &name) {
    runtime_assert(variables.count(name) > 0,
                   "Variable " + name + " not found");
    runtime_assert(variables[name].type == BRILValue::Type::Bool,
                   "Variable " + name + " is not a bool");
    return variables[name].int_value;
  }
  int get_int(const std::string &name) {
    runtime_assert(variables.count(name) > 0,
                   "Variable " + name + " not found");
    runtime_assert(variables[name].type == BRILValue::Type::Int,
                   "Variable " + name + " is not an int");
    return variables[name].int_value;
  }
  BRILValue get_value(const std::string &name) {
    runtime_assert(variables.count(name) > 0,
                   "Variable " + name + " not found");
    return variables[name];
  }

  // Set the value of a variable
  void write_int(const std::string &name, const int value) {
    variables[name] = BRILValue::integer(value);
  }
  void write_raw_pointer(const std::string &name, const int value) {
    variables[name] = BRILValue::raw_pointer(value);
  }
  void write_bool(const std::string &name, const bool value) {
    variables[name] = BRILValue::boolean(value);
  }
  void write_value(const std::string &name, const BRILValue &value) {
    variables[name] = value;
  }

  // Allocate a new heap memory block
  BRILValue alloc(size_t size) {
    const size_t index = heap_memory.size();
    heap_memory.push_back({std::vector<BRILValue>(size), true});
    return BRILValue::heap_pointer(index, 0);
  }

  void free(const BRILValue value) {
    runtime_assert(value.type == BRILValue::Type::HeapPointer,
                   "Freed object was not heap pointer");
    runtime_assert(value.heap_offset == 0, "Freed object was not base pointer");
    const size_t heap_idx = value.heap_idx;
    runtime_assert(heap_idx < heap_memory.size(), "Invalid heap index");
    runtime_assert(heap_memory[heap_idx].active, "Double free");
    heap_memory[heap_idx].active = false;
  }

  BRILValue load(const BRILValue pointer) {
    if (pointer.type == BRILValue::Type::Address) {
      return get_value(pointer.string_value);
    }
    runtime_assert(pointer.type == BRILValue::Type::HeapPointer,
                   "Writing to non-heap pointer");
    const size_t idx = pointer.heap_idx;
    const size_t offset = pointer.heap_offset;
    runtime_assert(idx < heap_memory.size(), "Invalid heap index");
    runtime_assert(heap_memory[idx].active, "Reading from freed memory");
    return heap_memory[idx].values[offset];
  }

  void store(const BRILValue pointer, const BRILValue value) {
    if (pointer.type == BRILValue::Type::Address) {
      write_value(pointer.string_value, value);
      return;
    }
    runtime_assert(pointer.type == BRILValue::Type::HeapPointer,
                   "Writing to non-heap pointer");
    const size_t idx = pointer.heap_idx;
    const size_t offset = pointer.heap_offset;
    runtime_assert(idx < heap_memory.size(), "Invalid heap index");
    runtime_assert(heap_memory[idx].active, "Writing to freed memory");
    heap_memory[idx].values[offset] = value;
  }

  // Pointer arithmetic
  BRILValue pointer_add(const BRILValue pointer, const int offset) {
    runtime_assert(pointer.type == BRILValue::Type::HeapPointer,
                   "Adding to non-heap pointer");
    const size_t idx = pointer.heap_idx;
    const size_t old_offset = pointer.heap_offset;
    runtime_assert(idx < heap_memory.size(), "Invalid heap index");
    runtime_assert(heap_memory[idx].active, "Adding to freed memory");
    return BRILValue::heap_pointer(idx, old_offset + offset);
  }

  BRILValue pointer_sub(const BRILValue pointer, const int offset) {
    runtime_assert(pointer.type == BRILValue::Type::HeapPointer,
                   "Subtracting from non-heap pointer");
    const size_t idx = pointer.heap_idx;
    const size_t old_offset = pointer.heap_offset;
    runtime_assert(idx < heap_memory.size(), "Invalid heap index");
    runtime_assert(heap_memory[idx].active, "Subtracting from freed memory");
    return BRILValue::heap_pointer(idx, old_offset - offset);
  }

  int pointer_diff(const BRILValue p1, const BRILValue p2) {
    runtime_assert(p1.type == BRILValue::Type::HeapPointer,
                   "Subtracting non-heap pointer");
    runtime_assert(p2.type == BRILValue::Type::HeapPointer,
                   "Subtracting non-heap pointer");
    const size_t idx1 = p1.heap_idx;
    const size_t idx2 = p2.heap_idx;
    const size_t offset1 = p1.heap_offset;
    const size_t offset2 = p2.heap_offset;
    runtime_assert(idx1 < heap_memory.size(), "Invalid heap index");
    runtime_assert(idx2 < heap_memory.size(), "Invalid heap index");
    runtime_assert(heap_memory[idx1].active, "Subtracting from freed memory");
    runtime_assert(heap_memory[idx2].active, "Subtracting from freed memory");
    runtime_assert(idx1 == idx2, "Subtracting pointers to different objects");
    return offset1 - offset2;
  }
};

struct BRILInterpreter {
  bril::Program program;
  size_t num_dynamic_instructions = 0;

  BRILInterpreter(const bril::Program &program) : program(program) {}

  void run(std::ostream &os) {
    std::vector<BRILValue> arguments(2);
    const bool wain_is_array =
        program.wain().arguments[0].type == Type::IntStar;
    if (wain_is_array) {
      runtime_assert(false, "Array arguments not supported");
    } else {
      int first_arg, second_arg;
      std::cout << "Enter the value of the first argument: " << std::flush;
      std::cin >> first_arg;
      std::cout << "Enter the value of the second argument: " << std::flush;
      std::cin >> second_arg;
      arguments[0] = BRILValue::integer(first_arg);
      arguments[1] = BRILValue::integer(second_arg);
    }

    const BRILValue result = interpret(program.wain(), arguments, os);
    std::cerr << "wain returned " << result << std::endl;
    std::cerr << "Number of dynamic instructions: " << num_dynamic_instructions
              << std::endl;
  }

  // Interpret a bril function, piping the output to the given stream
  BRILValue interpret(const bril::ControlFlowGraph &graph,
                      const std::vector<BRILValue> &arguments,
                      std::ostream &os);
};

} // namespace interpreter
} // namespace bril
