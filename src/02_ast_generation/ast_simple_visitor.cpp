
#include "ast_simple_visitor.hpp"

#include "ast_node.hpp"

#define BODY visitor.visit(*this)
void Program::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void Procedure::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void VariableLValueExpr::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void DereferenceLValueExpr::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void AssignmentExpr::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void VariableExpr::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void LiteralExpr::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void BinaryExpr::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void BooleanOrExpr::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void BooleanAndExpr::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void AddressOfExpr::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void DereferenceExpr::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void NewExpr::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void FunctionCallExpr::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void Statements::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void ExprStatement::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void AssignmentStatement::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void IfStatement::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void WhileStatement::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void ForStatement::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void PrintStatement::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void DeleteStatement::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void BreakStatement::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void ContinueStatement::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
void ReturnStatement::accept_simple(ASTSimpleVisitor &visitor) { BODY; }
#undef BODY
