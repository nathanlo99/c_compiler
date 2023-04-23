
#include "ast_node.hpp"

void Program::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  for (Procedure &procedure : procedures)
    procedure.accept_recursive(visitor);
  visitor.post_visit(*this);
}

void Procedure::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  for (const auto &statement : statements)
    statement->accept_recursive(visitor);
  return_expr->accept_recursive(visitor);
  visitor.post_visit(*this);
}

void VariableLValueExpr::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  visitor.post_visit(*this);
}

void DereferenceLValueExpr::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  argument->accept_recursive(visitor);
  visitor.post_visit(*this);
}

void AssignmentExpr::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  lhs->accept_recursive(visitor);
  rhs->accept_recursive(visitor);
  visitor.post_visit(*this);
}

void TestExpr::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  lhs->accept_recursive(visitor);
  rhs->accept_recursive(visitor);
  visitor.post_visit(*this);
}

void VariableExpr::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  visitor.post_visit(*this);
}

void LiteralExpr::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  visitor.post_visit(*this);
}

void BinaryExpr::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  lhs->accept_recursive(visitor);
  rhs->accept_recursive(visitor);
  visitor.post_visit(*this);
}

void AddressOfExpr::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  argument->accept_recursive(visitor);
  visitor.post_visit(*this);
}

void DereferenceExpr::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  argument->accept_recursive(visitor);
  visitor.post_visit(*this);
}

void NewExpr::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  rhs->accept_recursive(visitor);
  visitor.post_visit(*this);
}

void FunctionCallExpr::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  for (auto &argument : arguments)
    argument->accept_recursive(visitor);
  visitor.post_visit(*this);
}

void Statements::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  for (const auto &statement : statements) {
    statement->accept_recursive(visitor);
  }
  visitor.post_visit(*this);
}

void ExprStatement::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  expr->accept_recursive(visitor);
  visitor.post_visit(*this);
}

void AssignmentStatement::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  lhs->accept_recursive(visitor);
  rhs->accept_recursive(visitor);
  visitor.post_visit(*this);
}

void IfStatement::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  test_expression->accept_recursive(visitor);
  true_statements.accept_recursive(visitor);
  false_statements.accept_recursive(visitor);
  visitor.post_visit(*this);
}

void WhileStatement::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  test_expression->accept_recursive(visitor);
  body_statement->accept_recursive(visitor);
  visitor.post_visit(*this);
}

void PrintStatement::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  expression->accept_recursive(visitor);
  visitor.post_visit(*this);
}

void DeleteStatement::accept_recursive(ASTRecursiveVisitor &visitor) {
  visitor.pre_visit(*this);
  expression->accept_recursive(visitor);
  visitor.post_visit(*this);
}
