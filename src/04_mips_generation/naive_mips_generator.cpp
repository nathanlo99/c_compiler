
#include "naive_mips_generator.hpp"
#include "ast_node.hpp"
#include "mips_instruction.hpp"
#include "util.hpp"
#include <memory>

void NaiveMIPSGenerator::visit(Program &program) {
  table = program.table;

  // Emit the C code
  comment("// Simplified C code:");
  std::stringstream ss;
  program.emit_c(ss, 0);
  std::string line;
  while (std::getline(ss, line)) {
    comment(line);
  }

  if (table.use_memory) {
    import("init");
    import("new");
    import("delete");
  }

  if (table.use_print) {
    import("print");
    load_const(10, "print");
  }

  init_constants();
  beq(0, 0, "wain");
  annotate("Done prologue, jumping to wain");

  for (auto &procedure : program.procedures) {
    procedure.accept_simple(*this);
  }

  const MIPSInstruction push_3_0 = MIPSInstruction::sw(3, -4, 30);
  const MIPSInstruction push_3_1 = MIPSInstruction::sub(30, 30, 4);
  const MIPSInstruction pop_5_0 = MIPSInstruction::add(30, 30, 4);
  const MIPSInstruction pop_5_1 = MIPSInstruction::lw(5, -4, 30);
  for (size_t i = 0; i + 1 < instructions.size(); ++i) {
    const bool is_push_3 =
        instructions[i] == push_3_0 && instructions[i + 1] == push_3_1;
    if (!is_push_3)
      continue;
    for (size_t j = i + 2; j + 1 < instructions.size(); ++j) {
      const bool is_pop_5 =
          instructions[j] == pop_5_0 && instructions[j + 1] == pop_5_1;
      const auto &instruction = instructions[j];

      if (is_pop_5) {
        instructions[i] = MIPSInstruction::add(5, 3, 0);
        instructions[i].comment_value = "Peephole optimization: push 3, pop 5";
        instructions.erase(instructions.begin() + j + 1);
        instructions.erase(instructions.begin() + j);
        instructions.erase(instructions.begin() + i + 1);
        std::cerr << "Optimized push/pop 3 into add 5, 3, 0 at indices " << i
                  << ", " << j << ", " << j + 1 << std::endl;
        break;
      } 

      // Otherwise, if the instruction is a jump, label, branch, or uses 5, we 
      // can't optimize
      if (instruction.opcode == Opcode::Jalr ||
          instruction.opcode == Opcode::Jr ||
          instruction.opcode == Opcode::Label ||
          instruction.opcode == Opcode::Beq ||
          instruction.opcode == Opcode::Bne || instruction.d == 5 ||
          instruction.s == 5 || instruction.t == 5) {
        break;
      }
    }
  }

  comment("Number of assembly instructions: " +
          std::to_string(num_assembly_instructions()));
}

void NaiveMIPSGenerator::visit(Procedure &procedure) {
  const std::string procedure_name = procedure.name;
  const bool is_wain = procedure_name == "wain";

  table.enter_procedure(procedure_name);

  comment("");
  comment("Generating code for " + procedure_name);
  label(procedure_name);
  if (is_wain) {
    push(1);
    push(2);

    if (table.use_memory) {
      comment("Calling init");
      const bool first_arg_is_array =
          table.get_arguments("wain")[0].type == Type::IntStar;
      if (!first_arg_is_array) {
        add(2, 0, 0);
      }
      push(31);
      load_and_jalr("init");
      pop(31);
      comment("Done calling init");
    }
  }
  sub(29, 30, 4);

  for (const auto &variable : procedure.decls) {
    push_const(3, variable.initial_value.value);
    annotate("Variable " + variable.name);
  }

  // save_registers();

  comment("Code for statements:");
  for (const auto &statement : procedure.statements) {
    statement->accept_simple(*this);
  }
  comment("Code for return value:");
  procedure.return_expr->accept_simple(*this);

  // We only have to do clean-up if we aren't wain
  if (procedure_name != "wain") {
    comment("Done evaluating result, popping decls and saved registers");
    // pop_registers();
    pop_and_discard(procedure.decls.size());
  }

  jr(31);
  annotate("Done generating code for " + procedure_name);

  table.leave_procedure();
}

void NaiveMIPSGenerator::visit(VariableLValueExpr &) {
  unreachable("Variable lvalue code is handled in addressof and assignment");
}

void NaiveMIPSGenerator::visit(DereferenceLValueExpr &) {
  unreachable("Dereference lvalue code is handled in addressof and assignment");
}

void NaiveMIPSGenerator::visit(TestExpr &expr) {
  expr.lhs->accept_simple(*this);
  push(3);
  expr.rhs->accept_simple(*this);
  pop(5);
  switch (expr.operation) {
  case ComparisonOperation::LessThan:
    slt(3, 5, 3);
    break;
  case ComparisonOperation::LessEqual:
    slt(3, 3, 5);
    sub(3, 11, 3);
    break;
  case ComparisonOperation::GreaterThan:
    slt(3, 3, 5);
    break;
  case ComparisonOperation::GreaterEqual:
    slt(3, 5, 3);
    sub(3, 11, 3);
    break;
  case ComparisonOperation::NotEqual:
    slt(6, 3, 5);
    slt(7, 5, 3);
    add(3, 6, 7);
    break;
  case ComparisonOperation::Equal:
    slt(6, 3, 5);
    slt(7, 5, 3);
    add(3, 6, 7);
    sub(3, 11, 3);
    break;
  default:
    runtime_assert(false, "Unknown comparison operation");
  }
}

void NaiveMIPSGenerator::visit(VariableExpr &expr) {
  const int offset = table.get_offset(expr.variable);
  lw(3, offset, 29);
  annotate("Loading " + expr.variable.name);
}

void NaiveMIPSGenerator::visit(LiteralExpr &expr) {
  load_const(3, expr.literal.value);
  annotate("Loading the literal " + expr.literal.value_to_string());
}

void NaiveMIPSGenerator::visit(BinaryExpr &expr) {
  const auto lhs_type = expr.lhs->type;
  const auto rhs_type = expr.rhs->type;
  expr.lhs->accept_simple(*this);
  push(3);
  expr.rhs->accept_simple(*this);
  pop(5);
  switch (expr.operation) {
  case BinaryOperation::Add:
    if (lhs_type == Type::IntStar) {
      // Multiply rhs ($3) by 4
      mult(3, 4);
      mflo(3);
    } else if (rhs_type == Type::IntStar) {
      // Multiply lhs ($5) by 4
      mult(5, 4);
      mflo(5);
    }
    add(3, 5, 3);
    break;
  case BinaryOperation::Sub:
    if (lhs_type == Type::Int && rhs_type == Type::Int) {
      sub(3, 5, 3);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::Int) {
      // lhs - 4 * rhs  -->  $5 - 4 * $3
      mult(3, 4);
      mflo(3);
      sub(3, 5, 3);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::IntStar) {
      // (lhs - rhs) / 4  --> ($5 - $3) / $4
      sub(3, 5, 3);
      div(3, 4);
      mflo(3);
    }
    break;
  case BinaryOperation::Mul:
    mult(5, 3);
    mflo(3);
    break;
  case BinaryOperation::Div:
    div(5, 3);
    mflo(3);
    break;
  case BinaryOperation::Mod:
    div(5, 3);
    mfhi(3);
    break;
  default:
    runtime_assert(false, "Unknown binary operation");
  }
}

void NaiveMIPSGenerator::visit(AddressOfExpr &expr) {
  if (const auto lhs =
          std::dynamic_pointer_cast<VariableLValueExpr>(expr.argument)) {
    const int offset = table.get_offset(lhs->variable);
    load_const(3, offset);
    add(3, 3, 29);
  } else if (const auto lhs = std::dynamic_pointer_cast<DereferenceLValueExpr>(
                 expr.argument)) {
    lhs->argument->accept_simple(*this);
  } else {
    unreachable("Unknown lvalue type");
  }
}

void NaiveMIPSGenerator::visit(DereferenceExpr &expr) {
  expr.argument->accept_simple(*this);
  lw(3, 0, 3);
}

void NaiveMIPSGenerator::visit(NewExpr &expr) {
  expr.rhs->accept_simple(*this);
  add(1, 3, 0);
  push(31);
  load_and_jalr("new");
  pop(31);
  // The result is stored in $1: copy it to $3, and return NULL (1) if it was 0
  bne(3, 0, 1);
  add(3, 11, 0);
}

void NaiveMIPSGenerator::visit(FunctionCallExpr &expr) {
  const std::string procedure_name = expr.procedure_name;
  const auto params = table.get_arguments(procedure_name);
  const size_t num_arguments = params.size();
  push(29);
  push(31);
  for (size_t i = 0; i < num_arguments; ++i) {
    comment("Pushing argument " + params[i].name);
    auto &argument = expr.arguments[i];
    argument->accept_simple(*this);
    push(3);
    comment("Done pushing argument " + params[i].name);
  }
  load_and_jalr(procedure_name);
  pop_and_discard(num_arguments);
  pop(31);
  pop(29);
}

void NaiveMIPSGenerator::visit(Statements &statements) {
  for (auto &statement : statements.statements) {
    statement->accept_simple(*this);
  }
}

void NaiveMIPSGenerator::visit(AssignmentStatement &statement) {
  std::stringstream ss;
  statement.emit_c(ss, 0);
  comment(ss.str());
  if (const auto lvalue =
          std::dynamic_pointer_cast<VariableLValueExpr>(statement.lhs)) {
    const int offset = table.get_offset(lvalue->variable);
    statement.rhs->accept_simple(*this);
    sw(3, offset, 29);
  } else if (const auto lvalue =
                 std::dynamic_pointer_cast<DereferenceLValueExpr>(
                     statement.lhs)) {
    lvalue->argument->accept_simple(*this);
    push(3);
    statement.rhs->accept_simple(*this);
    pop(5);
    sw(3, 0, 5);
  } else {
    unreachable("Unknown lvalue type");
  }
}

void NaiveMIPSGenerator::visit(IfStatement &statement) {
  const auto else_label = generate_label("ifelse");
  const auto endif_label = generate_label("ifendif");
  statement.test_expression->accept_simple(*this);
  beq(3, 0, else_label);
  statement.true_statement->accept_simple(*this);
  beq(0, 0, endif_label);
  label(else_label);
  statement.false_statement->accept_simple(*this);
  label(endif_label);
}

void NaiveMIPSGenerator::visit(WhileStatement &statement) {
  const auto loop_label = generate_label("whileloop");
  const auto end_label = generate_label("whileend");
  label(loop_label);
  statement.test_expression->accept_simple(*this);
  beq(3, 0, end_label);
  statement.body_statement->accept_simple(*this);
  beq(0, 0, loop_label);
  label(end_label);
}

void NaiveMIPSGenerator::visit(PrintStatement &statement) {
  statement.expression->accept_simple(*this);
  add(1, 3, 0);
  push(31);
  load_and_jalr("print");
  pop(31);
}

void NaiveMIPSGenerator::visit(DeleteStatement &statement) {
  const auto skip_label = generate_label("deleteskip");
  statement.expression->accept_simple(*this);
  beq(3, 11, skip_label);
  add(1, 3, 0);
  push(31);
  load_and_jalr("delete");
  pop(31);
  label(skip_label);
}
