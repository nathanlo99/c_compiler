
#pragma once

#include "bril.hpp"
#include "util.hpp"
#include <stdexcept>

namespace bril {
namespace interpreter {

struct BRILValue {
  enum class Type { Int, RawPointer, Address, HeapPointer, Undefined };

  // If the type is Int, the value is stored in int_value
  // If the type is Address, the name of the variable is stored in string_value
  // If the type is HeapPointer, the index of the heap memory is stored in
  // int_value
  Type type;
  std::string string_value;
  int int_value;
  size_t heap_idx, heap_offset;

  BRILValue() : type(Type::Undefined) {}
  BRILValue(const Type type, const int int_value,
            const std::string &string_value)
      : type(type), string_value(string_value), int_value(int_value) {}
  BRILValue(const size_t heap_idx, const size_t heap_offset)
      : type(Type::HeapPointer), heap_idx(heap_idx), heap_offset(heap_offset) {}

  static BRILValue integer(const int value) {
    return BRILValue(Type::Int, value, "");
  }
  static BRILValue raw_pointer(const int value) {
    return BRILValue(Type::RawPointer, value, "");
  }
  static BRILValue address(const size_t stack_depth, const std::string &name) {
    return BRILValue(Type::Address, stack_depth, name);
  }
  static BRILValue heap_pointer(const size_t idx, const int offset) {
    return BRILValue(idx, offset);
  }

  friend std::ostream &operator<<(std::ostream &os, const BRILValue &value) {
    switch (value.type) {
    case Type::Int:
      os << value.int_value << ": int";
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
    case Type::Undefined:
      os << "__undefined";
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

struct BRILStackFrame {
  std::unordered_map<std::string, BRILValue> variables;

  // Get the value of a variable
  int get_int(const std::string &name) {
    debug_assert(variables.count(name) > 0, "Variable {} not found", name);
    debug_assert(variables[name].type == BRILValue::Type::Int,
                 "Variable {} is not an int", name);
    return variables[name].int_value;
  }
  BRILValue get_value(const std::string &name) {
    if (name == "__undefined")
      return BRILValue();
    debug_assert(variables.count(name) > 0, "Variable {} not found", name);
    return variables[name];
  }

  // Set the value of a variable
  void write_int(const std::string &name, const int value) {
    variables[name] = BRILValue::integer(value);
  }
  void write_raw_pointer(const std::string &name, const int value) {
    variables[name] = BRILValue::raw_pointer(value);
  }
  void write_value(const std::string &name, const BRILValue &value) {
    variables[name] = value;
  }
};

struct BRILContext {
  std::vector<BRILStackFrame> stack_frames;
  std::vector<BRILAlloc> heap_memory;

  void clear() {
    stack_frames.clear();
    heap_memory.clear();
  }

  // Get the value of a variable
  inline int get_int(const std::string &name) {
    debug_assert(name != "__undefined", "Reading from uninitialized variable");
    return stack_frames.back().get_int(name);
  }
  inline BRILValue get_value(const std::string &name) {
    if (name == "__undefined")
      return BRILValue();
    return stack_frames.back().get_value(name);
  }

  // Set the value of a variable
  void write_int(const std::string &name, const int value) {
    stack_frames.back().write_int(name, value);
  }
  void write_raw_pointer(const std::string &name, const int value) {
    stack_frames.back().write_raw_pointer(name, value);
  }
  void write_value(const std::string &name, const BRILValue &value) {
    stack_frames.back().write_value(name, value);
  }

  // Allocate a new heap memory block
  BRILValue alloc(size_t size) {
    const size_t index = heap_memory.size();
    heap_memory.push_back({std::vector<BRILValue>(size), true});
    return BRILValue::heap_pointer(index, 0);
  }

  void free(const BRILValue value) {
    debug_assert(value.type == BRILValue::Type::HeapPointer,
                 "Freed object was not heap pointer");
    debug_assert(value.heap_offset == 0, "Freed object was not base pointer");
    const size_t heap_idx = value.heap_idx;
    debug_assert(heap_idx < heap_memory.size(), "Invalid heap index");
    debug_assert(heap_memory[heap_idx].active, "Double free");
    heap_memory[heap_idx].active = false;
  }

  BRILValue load(const BRILValue pointer) {
    if (pointer.type == BRILValue::Type::Address) {
      const size_t stack_depth = pointer.int_value;
      debug_assert(stack_depth < stack_frames.size(), "Invalid stack depth");
      return stack_frames[stack_depth].get_value(pointer.string_value);
    }
    debug_assert(pointer.type == BRILValue::Type::HeapPointer,
                 "Writing to non-heap pointer");
    const size_t idx = pointer.heap_idx;
    const size_t offset = pointer.heap_offset;
    debug_assert(idx < heap_memory.size(), "Invalid heap index");
    debug_assert(heap_memory[idx].active, "Reading from freed memory");
    return heap_memory[idx].values[offset];
  }

  void store(const BRILValue pointer, const BRILValue value) {
    if (pointer.type == BRILValue::Type::Address) {
      const size_t stack_depth = pointer.int_value;
      debug_assert(stack_depth < stack_frames.size(), "Invalid stack depth");
      stack_frames[stack_depth].write_value(pointer.string_value, value);
      return;
    }
    debug_assert(pointer.type == BRILValue::Type::HeapPointer,
                 "Writing to non-heap pointer");
    const size_t idx = pointer.heap_idx;
    const size_t offset = pointer.heap_offset;
    debug_assert(idx < heap_memory.size(), "Invalid heap index");
    debug_assert(heap_memory[idx].active, "Writing to freed memory");
    debug_assert(offset < heap_memory[idx].values.size(),
                 "Writing out of bounds: {} >= {}", offset,
                 heap_memory[idx].values.size());
    heap_memory[idx].values[offset] = value;
  }

  // Pointer arithmetic
  BRILValue pointer_add(const BRILValue pointer, const int offset) {
    debug_assert(pointer.type == BRILValue::Type::HeapPointer,
                 "Adding to non-heap pointer");
    const size_t idx = pointer.heap_idx;
    const size_t old_offset = pointer.heap_offset;
    debug_assert(idx < heap_memory.size(), "Invalid heap index");
    return BRILValue::heap_pointer(idx, old_offset + offset);
  }

  BRILValue pointer_sub(const BRILValue pointer, const int amount) {
    debug_assert(pointer.type == BRILValue::Type::HeapPointer,
                 "Subtracting from non-heap pointer");
    const size_t idx = pointer.heap_idx;
    const size_t old_offset = pointer.heap_offset;
    debug_assert(idx < heap_memory.size(), "Invalid heap index: {} >= {}", idx,
                 heap_memory.size());
    return BRILValue::heap_pointer(idx, old_offset - amount);
  }

  int pointer_diff(const BRILValue p1, const BRILValue p2) {
    debug_assert(p1.type == BRILValue::Type::HeapPointer,
                 "Subtracting non-heap pointer");
    debug_assert(p2.type == BRILValue::Type::HeapPointer,
                 "Subtracting non-heap pointer");
    const size_t idx1 = p1.heap_idx;
    const size_t idx2 = p2.heap_idx;
    const size_t offset1 = p1.heap_offset;
    const size_t offset2 = p2.heap_offset;
    debug_assert(idx1 < heap_memory.size(), "Invalid heap index");
    debug_assert(idx2 < heap_memory.size(), "Invalid heap index");
    debug_assert(idx1 == idx2, "Subtracting pointers to different objects");
    return offset1 - offset2;
  }
};

struct BRILInterpreter {
  bril::Program program;
  BRILContext context;
  size_t num_dynamic_instructions = 0;

  BRILInterpreter(const bril::Program &program) : program(program) {}

  void run(std::ostream &os);

  // Interpret a bril function, piping the output to the given stream
  BRILValue interpret(const bril::ControlFlowGraph &graph,
                      const std::vector<BRILValue> &arguments,
                      std::ostream &os);
};

} // namespace interpreter
} // namespace bril
