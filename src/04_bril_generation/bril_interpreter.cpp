
#include "bril_interpreter.hpp"
#include "bril.hpp"
#include "util.hpp"

namespace bril {
namespace interpreter {

BRILValue BRILInterpreter::interpret(const bril::ControlFlowGraph &graph,
                                     const std::vector<BRILValue> &arguments,
                                     std::ostream &os) {
  BRILContext context;
  for (size_t idx = 0; idx < arguments.size(); ++idx) {
    context.variables[graph.arguments[idx].name] = arguments[idx];
  }

  size_t block_idx = 0, instruction_idx = 0;
  while (true) {
    // 1. Get the current instruction
    runtime_assert(block_idx < graph.blocks.size(), "Block idx out of range");
    runtime_assert(instruction_idx <
                       graph.blocks[block_idx].instructions.size(),
                   "Instruction idx out of range");
    const auto &instruction =
        graph.blocks[block_idx].instructions[instruction_idx];

    // 2. Advance the instruction pointer
    ++instruction_idx;
    if (instruction_idx >= graph.blocks[block_idx].instructions.size()) {
      instruction_idx = 0;
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
      block_idx = graph.label_to_block.at(label);
      instruction_idx = 0;
      continue;
    } break;

    case Opcode::Br: {
      const bool condition = context.get_bool(instruction.arguments[0]);
      const std::string label = instruction.labels[condition ? 0 : 1];
      block_idx = graph.label_to_block.at(label);
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
        return BRILValue();
      } else {
        return context.get_value(instruction.arguments[0]);
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

    case Opcode::Label: {
      // TODO: Keep track of labels for phi nodes
    } break;

    default:
      runtime_assert(false,
                     "Opcode " +
                         std::to_string(static_cast<int>(instruction.opcode)) +
                         " not implemented");
    }
  }
}

} // namespace interpreter
} // namespace bril
