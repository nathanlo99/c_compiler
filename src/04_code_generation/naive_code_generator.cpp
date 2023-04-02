
#include "naive_code_generator.hpp"
#include "ast_node.hpp"
#include "mips_instruction.hpp"

void NaiveCodeGenerator::visit(Program &program) {
  table = program.table;

  // TODO: Add global imports and things
  instructions.push_back(MIPSInstruction::import("print"));
  load_const(4, 4);
  load_const(10, "print");
  load_const(11, 1);

  instructions.push_back(MIPSInstruction::beq(0, 0, "wain"));
  for (auto &procedure : program.procedures) {
    procedure.accept_simple(*this);
  }
}

void NaiveCodeGenerator::visit(Procedure &procedure) {
  // TODO
}

void NaiveCodeGenerator::visit(VariableLValueExpr &expr) {
  // TODO
}

void NaiveCodeGenerator::visit(DereferenceLValueExpr &expr) {
  expr.argument->accept_simple(*this);
  instructions.push_back(MIPSInstruction::lw(3, 0, 3));
}

void NaiveCodeGenerator::visit(TestExpr &expr) {
  expr.lhs->accept_simple(*this);
  push(3);
  expr.rhs->accept_simple(*this);
  pop(5);
  switch (expr.operation) {
  case ComparisonOperation::LessThan:
    instructions.push_back(MIPSInstruction::slt(3, 5, 3));
    break;
  case ComparisonOperation::LessEqual:
    instructions.push_back(MIPSInstruction::slt(3, 3, 5));
    instructions.push_back(MIPSInstruction::sub(3, 11, 3));
    break;
  case ComparisonOperation::GreaterThan:
    instructions.push_back(MIPSInstruction::slt(3, 3, 5));
    break;
  case ComparisonOperation::GreaterEqual:
    instructions.push_back(MIPSInstruction::slt(3, 5, 3));
    instructions.push_back(MIPSInstruction::sub(3, 11, 3));
    break;
  case ComparisonOperation::NotEqual:
    instructions.push_back(MIPSInstruction::slt(6, 3, 5));
    instructions.push_back(MIPSInstruction::slt(7, 5, 3));
    instructions.push_back(MIPSInstruction::add(3, 6, 7));
    break;
  case ComparisonOperation::Equal:
    instructions.push_back(MIPSInstruction::slt(6, 3, 5));
    instructions.push_back(MIPSInstruction::slt(7, 5, 3));
    instructions.push_back(MIPSInstruction::add(3, 6, 7));
    instructions.push_back(MIPSInstruction::sub(3, 11, 3));
    break;
  }
}

void NaiveCodeGenerator::visit(VariableExpr &expr) {
  // TODO
}

void NaiveCodeGenerator::visit(LiteralExpr &expr) {
  if (expr.literal.type == Type::IntStar && expr.literal.value == 0) {
    instructions.push_back(MIPSInstruction::add(3, 0, 11));
  } else {
    load_const(3, expr.literal.value);
  }
}

void NaiveCodeGenerator::visit(BinaryExpr &expr) {
  expr.lhs->accept_simple(*this);
  push(3);
  expr.rhs->accept_simple(*this);
  pop(5);
  switch (expr.operation) {
  case BinaryOperation::Add:
    instructions.push_back(MIPSInstruction::add(3, 5, 3));
    break;
  case BinaryOperation::Sub:
    instructions.push_back(MIPSInstruction::sub(3, 5, 3));
    break;
  case BinaryOperation::Mul:
    instructions.push_back(MIPSInstruction::mult(5, 3));
    instructions.push_back(MIPSInstruction::mflo(3));
    break;
  case BinaryOperation::Div:
    instructions.push_back(MIPSInstruction::div(5, 3));
    instructions.push_back(MIPSInstruction::mflo(3));
    break;
  case BinaryOperation::Mod:
    instructions.push_back(MIPSInstruction::div(5, 3));
    instructions.push_back(MIPSInstruction::mfhi(3));
    break;
  }
}

void NaiveCodeGenerator::visit(AddressOfExpr &expr) {
  // Since the code generated for an lvalue is its address,
  // we don't have to modify it in any way
  expr.argument->accept_simple(*this);
}

void NaiveCodeGenerator::visit(NewExpr &expr) {
  // TODO
}

void NaiveCodeGenerator::visit(FunctionCallExpr &expr) {
  // TODO
}

void NaiveCodeGenerator::visit(Statements &stmts) {
  // TODO
}

void NaiveCodeGenerator::visit(AssignmentStatement &statement) {
  statement.lhs->accept_simple(*this);
  push(3);
  statement.rhs->accept_simple(*this);
  pop(5);
  instructions.push_back(MIPSInstruction::sw(3, 0, 5));
}

void NaiveCodeGenerator::visit(IfStatement &statement) {
  const auto else_label = generate_label("if_else");
  const auto endif_label = generate_label("if_endif");
  statement.test_expression->accept_simple(*this);
  instructions.push_back(MIPSInstruction::beq(3, 0, else_label));
  statement.true_statement->accept_simple(*this);
  instructions.push_back(MIPSInstruction::beq(0, 0, endif_label));
  instructions.push_back(MIPSInstruction::label(else_label));
  statement.false_statement->accept_simple(*this);
  instructions.push_back(MIPSInstruction::label(endif_label));
}

void NaiveCodeGenerator::visit(WhileStatement &statement) {
  const auto loop_label = generate_label("while_loop");
  const auto end_label = generate_label("while_end");
  instructions.push_back(MIPSInstruction::label(loop_label));
  statement.test_expression->accept_simple(*this);
  instructions.push_back(MIPSInstruction::beq(3, 0, end_label));
  statement.body_statement->accept_simple(*this);
  instructions.push_back(MIPSInstruction::beq(0, 0, loop_label));
  instructions.push_back(MIPSInstruction::label(end_label));
}

void NaiveCodeGenerator::visit(PrintStatement &statement) {
  // TODO
}

void NaiveCodeGenerator::visit(DeleteStatement &statement) {
  // TODO
}
