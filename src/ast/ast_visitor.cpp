
#include "ast_node.hpp"

void Program::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  for (Procedure &procedure : procedures)
    procedure.visit(visitor);
  visitor.post_visit(*this);
}

void Procedure::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  for (const auto &statement : statements)
    statement->visit(visitor);
  return_expr->visit(visitor);
  visitor.post_visit(*this);
}

void VariableLValueExpr::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  visitor.post_visit(*this);
}

void DereferenceLValueExpr::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  argument->visit(visitor);
  visitor.post_visit(*this);
}

void TestExpr::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  lhs->visit(visitor);
  rhs->visit(visitor);
  visitor.post_visit(*this);
}

void VariableExpr::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  visitor.post_visit(*this);
}

void LiteralExpr::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  visitor.post_visit(*this);
}

void BinaryExpr::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  lhs->visit(visitor);
  rhs->visit(visitor);
  visitor.post_visit(*this);
}

void AddressOfExpr::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  argument->visit(visitor);
  visitor.post_visit(*this);
}

void NewExpr::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  rhs->visit(visitor);
  visitor.post_visit(*this);
}

void FunctionCallExpr::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  for (auto &argument : arguments)
    argument->visit(visitor);
  visitor.post_visit(*this);
}

void Statements::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  for (const auto &statement : statements) {
    statement->visit(visitor);
  }
  visitor.post_visit(*this);
}

void AssignmentStatement::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  lhs->visit(visitor);
  rhs->visit(visitor);
  visitor.post_visit(*this);
}

void IfStatement::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  test_expression->visit(visitor);
  true_statement->visit(visitor);
  false_statement->visit(visitor);
  visitor.post_visit(*this);
}

void WhileStatement::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  test_expression->visit(visitor);
  body_statement->visit(visitor);
  visitor.post_visit(*this);
}

void PrintStatement::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  expression->visit(visitor);
  visitor.post_visit(*this);
}

void DeleteStatement::visit(ASTVisitor &visitor) {
  visitor.pre_visit(*this);
  expression->visit(visitor);
  visitor.post_visit(*this);
}
