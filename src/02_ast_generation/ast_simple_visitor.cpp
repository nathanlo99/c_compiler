
#include "ast_simple_visitor.hpp"
#include "ast_node.hpp"

#include "ast_node.hpp"

void Program::accept_simple(ASTSimpleVisitor &visitor) { visitor.visit(*this); }

void Procedure::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void VariableLValueExpr::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void DereferenceLValueExpr::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void TestExpr::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void VariableExpr::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void LiteralExpr::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void BinaryExpr::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void AddressOfExpr::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void DereferenceExpr::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void NewExpr::accept_simple(ASTSimpleVisitor &visitor) { visitor.visit(*this); }

void FunctionCallExpr::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void Statements::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void AssignmentStatement::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void IfStatement::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void WhileStatement::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void PrintStatement::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}

void DeleteStatement::accept_simple(ASTSimpleVisitor &visitor) {
  visitor.visit(*this);
}
