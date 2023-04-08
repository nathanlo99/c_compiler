
#include "bril_interpreter.hpp"
#include "bril.hpp"
#include "util.hpp"

namespace bril {
namespace interpreter {

void BRILInterpreter::run(std::ostream &os) {
  context.clear();
  std::vector<BRILValue> arguments(2);
  const bool wain_is_array = program.wain().arguments[0].type == Type::IntStar;
  if (wain_is_array) {
    size_t num_elements;
    int value;
    std::cout << "Enter the number of elements in the array: " << std::flush;
    std::cin >> num_elements;
    const auto array = context.alloc(num_elements);
    for (size_t idx = 0; idx < num_elements; ++idx) {
      std::cout << "Enter the value of element " << idx << ": " << std::flush;
      std::cin >> value;
      context.store(BRILValue::heap_pointer(0, idx), BRILValue::integer(value));
    }
    arguments[0] = array;
    arguments[1] = BRILValue::integer(num_elements);
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

  // Free the input array's memory
  if (wain_is_array) {
    context.free(BRILValue::heap_pointer(0, 0));
  }

  for (size_t heap_idx = 0; heap_idx < context.heap_memory.size(); ++heap_idx) {
    if (context.heap_memory[heap_idx].active) {
      std::cerr << "Memory leak: Memory region heap[" << heap_idx
                << "] of size " << context.heap_memory[heap_idx].values.size()
                << " is still allocated at the end of execution" << std::endl;
    }
  }
}

BRILValue BRILInterpreter::interpret(const bril::ControlFlowGraph &graph,
                                     const std::vector<BRILValue> &arguments,
                                     std::ostream &os) {
  context.stack_frames.emplace_back();
  for (size_t idx = 0; idx < arguments.size(); ++idx) {
    context.write_value(graph.arguments[idx].name, arguments[idx]);
  }

  size_t block_idx = 0, instruction_idx = 0, last_block = -1;
  while (true) {
    // 1. Get the current instruction
    runtime_assert(block_idx < graph.blocks.size(), "Block idx out of range");
    runtime_assert(instruction_idx <
                       graph.blocks[block_idx].instructions.size(),
                   "Instruction idx out of range");
    const auto &instruction =
        graph.blocks[block_idx].instructions[instruction_idx];
    std::cerr << "Interpreting (" << block_idx << ", " << instruction_idx
              << "): " << instruction << std::endl;

    // 2. Advance the instruction pointer
    ++instruction_idx;
    if (instruction_idx >= graph.blocks[block_idx].instructions.size()) {
      instruction_idx = 0;
      last_block = block_idx;
      ++block_idx;
    }
    ++num_dynamic_instructions;

    // 3. Interpret the instruction
    const std::string destination = instruction.destination;
    switch (instruction.opcode) {
    case Opcode::Add: {
      const int lhs = context.get_int(instruction.arguments[0]);
      const int rhs = context.get_int(instruction.arguments[1]);
      context.write_int(destination, lhs + rhs);
    } break;

    case Opcode::Sub: {
      const int lhs = context.get_int(instruction.arguments[0]);
      const int rhs = context.get_int(instruction.arguments[1]);
      context.write_int(destination, lhs - rhs);
    } break;

    case Opcode::Mul: {
      const int lhs = context.get_int(instruction.arguments[0]);
      const int rhs = context.get_int(instruction.arguments[1]);
      context.write_int(destination, lhs * rhs);
    } break;

    case Opcode::Div: {
      const int lhs = context.get_int(instruction.arguments[0]);
      const int rhs = context.get_int(instruction.arguments[1]);
      if (rhs == 0)
        throw std::runtime_error("Division by zero");
      context.write_int(destination, lhs / rhs);
    } break;

    case Opcode::Mod: {
      const int lhs = context.get_int(instruction.arguments[0]);
      const int rhs = context.get_int(instruction.arguments[1]);
      if (rhs == 0)
        throw std::runtime_error("Division by zero");
      context.write_int(destination, lhs % rhs);
    } break;

    case Opcode::Lt: {
      const BRILValue lhs = context.get_value(instruction.arguments[0]);
      const BRILValue rhs = context.get_value(instruction.arguments[1]);
      context.write_bool(destination, lhs < rhs);
    } break;

    case Opcode::Le: {
      const BRILValue lhs = context.get_value(instruction.arguments[0]);
      const BRILValue rhs = context.get_value(instruction.arguments[1]);
      context.write_bool(destination, lhs <= rhs);
    } break;

    case Opcode::Gt: {
      const BRILValue lhs = context.get_value(instruction.arguments[0]);
      const BRILValue rhs = context.get_value(instruction.arguments[1]);
      context.write_bool(destination, lhs > rhs);
    } break;

    case Opcode::Ge: {
      const BRILValue lhs = context.get_value(instruction.arguments[0]);
      const BRILValue rhs = context.get_value(instruction.arguments[1]);
      context.write_bool(destination, lhs >= rhs);
    } break;

    case Opcode::Eq: {
      const BRILValue lhs = context.get_value(instruction.arguments[0]);
      const BRILValue rhs = context.get_value(instruction.arguments[1]);
      context.write_bool(destination, lhs == rhs);
    } break;

    case Opcode::Ne: {
      const BRILValue lhs = context.get_value(instruction.arguments[0]);
      const BRILValue rhs = context.get_value(instruction.arguments[1]);
      context.write_bool(destination, lhs != rhs);
    } break;

    case Opcode::Jmp: {
      const std::string label = instruction.labels[0];
      runtime_assert(label.substr(0, 4) == ".bb_",
                     "Label must start with .bb_");
      last_block = block_idx;
      block_idx = std::stoi(label.substr(4));
      instruction_idx = 0;
      continue;
    } break;

    case Opcode::Br: {
      const bool condition = context.get_bool(instruction.arguments[0]);
      const std::string label = instruction.labels[condition ? 0 : 1];
      runtime_assert(label.substr(0, 4) == ".bb_",
                     "Label must start with .bb_");
      last_block = block_idx;
      block_idx = std::stoi(label.substr(4));
      instruction_idx = 0;
      continue;
    } break;

    case Opcode::Call: {
      const std::string function_name = instruction.funcs[0];
      const auto &function = program.get_function(function_name);
      std::vector<BRILValue> arguments;
      for (size_t idx = 0; idx < instruction.arguments.size(); ++idx) {
        arguments.push_back(context.get_value(instruction.arguments[idx]));
      }
      const BRILValue result = interpret(function, arguments, os);
      context.write_value(destination, result);
    } break;

    case Opcode::Ret: {
      if (instruction.arguments.empty()) {
        context.stack_frames.pop_back();
        return BRILValue();
      } else {
        const auto result = context.get_value(instruction.arguments[0]);
        context.stack_frames.pop_back();
        return result;
      }
    } break;

    case Opcode::Const: {
      switch (instruction.type) {
      case Type::Int: {
        context.write_int(destination, instruction.value);
      } break;
      case Type::IntStar: {
        context.write_raw_pointer(destination, instruction.value);
      } break;
      default:
        runtime_assert(false, "Invalid type for const instruction");
      }
    } break;

    case Opcode::Id: {
      const BRILValue value = context.get_value(instruction.arguments[0]);
      context.write_value(destination, value);
    } break;

    case Opcode::Print: {
      const int value = context.get_int(instruction.arguments[0]);
      os << value << std::endl;
    } break;

    case Opcode::Nop: {
      // Do nothing
    } break;

    // Memory instructions
    case Opcode::Alloc: {
      const int size = context.get_int(instruction.arguments[0]);
      runtime_assert(size > 0, "Allocation size must be positive");
      const BRILValue result = context.alloc(size);
      context.write_value(instruction.destination, result);
    } break;

    case Opcode::Free: {
      const BRILValue pointer = context.get_value(instruction.arguments[0]);
      context.free(pointer);
    } break;

    case Opcode::Store: {
      const BRILValue pointer = context.get_value(instruction.arguments[0]);
      const BRILValue value = context.get_value(instruction.arguments[1]);
      context.store(pointer, value);
    } break;

    case Opcode::Load: {
      const BRILValue pointer = context.get_value(instruction.arguments[0]);
      const BRILValue value = context.load(pointer);
      context.write_value(instruction.destination, value);
    } break;

    case Opcode::PointerAdd: {
      const BRILValue pointer = context.get_value(instruction.arguments[0]);
      const int offset = context.get_int(instruction.arguments[1]);
      const BRILValue result = context.pointer_add(pointer, offset);
      context.write_value(instruction.destination, result);
    } break;

    case Opcode::PointerSub: {
      const BRILValue pointer = context.get_value(instruction.arguments[0]);
      const int offset = context.get_int(instruction.arguments[1]);
      const BRILValue result = context.pointer_sub(pointer, offset);
      context.write_value(instruction.destination, result);
    } break;

    case Opcode::PointerDiff: {
      const BRILValue lhs = context.get_value(instruction.arguments[0]);
      const BRILValue rhs = context.get_value(instruction.arguments[1]);
      const int result = context.pointer_diff(lhs, rhs);
      context.write_int(instruction.destination, result);
    } break;

    case Opcode::AddressOf: {
      const BRILValue result = BRILValue::address(
          context.stack_frames.size() - 1, instruction.arguments[0]);
      context.write_value(instruction.destination, result);
    } break;

    case Opcode::Label: {
      // No-op
    } break;

    case Opcode::Phi: {
      std::cerr << "Last block: " << last_block << std::endl;
      runtime_assert(last_block != static_cast<size_t>(-1),
                     "Reached phi instruction before any jumps or branches");
      const std::string label = ".bb_" + std::to_string(last_block);
      bool done = false;
      for (size_t i = 0; i < instruction.labels.size() && !done; ++i) {
        if (instruction.labels[i] == label) {
          const BRILValue value = context.get_value(instruction.arguments[i]);
          context.write_value(instruction.destination, value);
          done = true;
        }
      }
      runtime_assert(done, "No matching label for phi instruction");
    } break;

    default:
      unreachable("Invalid opcode");
    }
  }
}

} // namespace interpreter
} // namespace bril
