
#include "naive_code_generator.hpp"
#include "ast_node.hpp"
#include "mips_instruction.hpp"

void NaiveCodeGenerator::visit(Program &program) {
  table = program.table;

  // Emit the C code
  comment("// Simplified C code:");
  std::stringstream ss;
  program.emit_c(ss, 0);
  std::string line;
  while (std::getline(ss, line)) {
    comment(line);
  }

  import("print");
  import("init");
  import("new");
  import("delete");
  load_const(4, 4);
  load_const(10, "print");
  load_const(11, 1);

  beq(0, 0, "wain");
  annotate("Done prologue, jumping to wain");

  for (auto &procedure : program.procedures) {
    procedure.accept_simple(*this);
  }

  comment("Number of assembly instructions: " +
          std::to_string(num_assembly_instructions()));
}

void NaiveCodeGenerator::visit(Procedure &procedure) {
  const std::string procedure_name = procedure.name;
  const bool is_wain = procedure_name == "wain";

  table.enter_procedure(procedure_name);

  comment("");
  comment("Generating code for " + procedure_name);
  label(procedure_name);
  if (is_wain) {
    push(1);
    push(2);

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
  sub(29, 30, 4);
  save_registers();

  for (const auto &variable : procedure.decls) {
    load_const(3, variable.initial_value.value);
    push(3);
    annotate("Variable " + variable.name);
  }

  comment("Code for statements:");
  for (const auto &statement : procedure.statements) {
    statement->accept_simple(*this);
  }
  comment("Code for return value:");
  procedure.return_expr->accept_simple(*this);

  // We only have to do clean-up if we aren't wain
  if (procedure_name != "wain") {
    comment("Done evaluating result, popping decls and saved registers");
    pop_and_discard(procedure.decls.size());
    pop_registers();
  }

  jr(31);
  annotate("Done generating code for " + procedure_name);

  table.leave_procedure();
}

void NaiveCodeGenerator::visit(VariableLValueExpr &expr) {
  const int offset = table.get_offset(expr.variable);
  load_const(3, offset);
  add(3, 29, 3);
}

void NaiveCodeGenerator::visit(DereferenceLValueExpr &expr) {
  expr.argument->accept_simple(*this);
  lw(3, 0, 3);
}

void NaiveCodeGenerator::visit(TestExpr &expr) {
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

void NaiveCodeGenerator::visit(VariableExpr &expr) {
  const int offset = table.get_offset(expr.variable);
  lw(3, offset, 29);
  annotate("Loading " + expr.variable.name);
}

void NaiveCodeGenerator::visit(LiteralExpr &expr) {
  if (expr.literal.type == Type::IntStar && expr.literal.value == 0) {
    add(3, 0, 11);
    annotate("Loading the literal NULL");
  } else {
    load_const(3, expr.literal.value);
    annotate("Loading the literal " + std::to_string(expr.literal.value));
  }
}

void NaiveCodeGenerator::visit(BinaryExpr &expr) {
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

void NaiveCodeGenerator::visit(AddressOfExpr &expr) {
  // Since the code generated for an lvalue is its address,
  // we don't have to modify it in any way
  expr.argument->accept_simple(*this);
}

void NaiveCodeGenerator::visit(NewExpr &expr) {
  // Let $1 be the result of the expression
  expr.rhs->accept_simple(*this);
  add(1, 3, 0);
  // Push $31, the linked register address
  push(31);
  // Jump to 'new'
  load_and_jalr("new");
  // Once we're back, restore $31
  pop(31);
  // The result is stored in $1, copy it to $3, and return NULL if it was 0
  bne(3, 0, 1);
  add(3, 11, 0);
}

void NaiveCodeGenerator::visit(FunctionCallExpr &expr) {
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

void NaiveCodeGenerator::visit(Statements &statements) {
  for (auto &statement : statements.statements) {
    statement->accept_simple(*this);
  }
}

void NaiveCodeGenerator::visit(AssignmentStatement &statement) {
  statement.lhs->accept_simple(*this);
  push(3);
  statement.rhs->accept_simple(*this);
  pop(5);
  sw(3, 0, 5);
}

void NaiveCodeGenerator::visit(IfStatement &statement) {
  const auto else_label = generate_label("if_else");
  const auto endif_label = generate_label("if_endif");
  statement.test_expression->accept_simple(*this);
  beq(3, 0, else_label);
  statement.true_statement->accept_simple(*this);
  beq(0, 0, endif_label);
  label(else_label);
  statement.false_statement->accept_simple(*this);
  label(endif_label);
}

void NaiveCodeGenerator::visit(WhileStatement &statement) {
  const auto loop_label = generate_label("while_loop");
  const auto end_label = generate_label("while_end");
  label(loop_label);
  statement.test_expression->accept_simple(*this);
  beq(3, 0, end_label);
  statement.body_statement->accept_simple(*this);
  beq(0, 0, loop_label);
  label(end_label);
}

void NaiveCodeGenerator::visit(PrintStatement &statement) {
  statement.expression->accept_simple(*this);
  add(1, 3, 0);
  push(31);
  load_and_jalr("print");
  pop(31);
}

void NaiveCodeGenerator::visit(DeleteStatement &statement) {
  const auto skip_label = generate_label("delete_skip");
  statement.expression->accept_simple(*this);
  beq(3, 11, skip_label);
  add(1, 3, 0);
  push(31);
  load_and_jalr("delete");
  pop(31);
  label(skip_label);
}
