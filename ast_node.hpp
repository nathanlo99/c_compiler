
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "parse_node.hpp"

#include "types.hpp"

struct ASTNode {
  Type type;
  virtual ~ASTNode() {}
};

struct Expr : ASTNode {};
struct Statement : ASTNode {};

struct Literal : ASTNode {
  int32_t value = 0;
  Type type = Type::Int;
};

struct Variable {
  std::string name;
  Type type;
  Literal initial_value;

  Variable(const std::string &name, const Type type,
           const Literal initial_value)
      : name(name), type(type), initial_value(initial_value) {}
};

struct ParameterList : ASTNode {
  std::vector<Variable> parameters;

  ParameterList(const std::vector<Variable> &variables = {})
      : parameters(variables) {}
};

struct DeclarationList : ASTNode {
  std::vector<Variable> declarations;

  DeclarationList(const std::vector<Variable> &variables = {})
      : declarations(variables) {}
};

struct Statements : Statement {
  std::vector<std::shared_ptr<Statement>> statements;
};

struct Procedure : ASTNode {
  std::string name;
  ParameterList params;
  Type return_type;

  DeclarationList decls;
  std::shared_ptr<Statements> statements;
  std::shared_ptr<Expr> return_expr;

  Procedure(const std::string &name, std::shared_ptr<ParameterList> params,
            const Type type, std::shared_ptr<DeclarationList> decls,
            std::shared_ptr<Statements> statements,
            std::shared_ptr<Expr> return_expr)
      : name(name), params(*params), return_type(type), decls(*decls),
        statements(statements), return_expr(return_expr) {}
};

struct Program : ASTNode {
  std::vector<Procedure> procedures;
};

// Expressions
struct LValueExpr : Expr {};

struct VariableLValueExpr : LValueExpr {
  Variable variable;
};

struct DereferenceLValueExpr : LValueExpr {
  std::shared_ptr<Expr> argument;
};

struct TestExpr {
  std::shared_ptr<Expr> lhs;
  Token operation;
  std::shared_ptr<Expr> rhs;
};

struct BinaryExpr {
  std::shared_ptr<Expr> lhs;
  Token operation;
  std::shared_ptr<Expr> rhs;
};

struct AddressOfExpr {
  std::shared_ptr<LValueExpr> argument;
};

// Statements
struct AssignmentStatement : Statement {
  Variable variable;
  std::shared_ptr<Expr> assigned_value;
};

struct IfStatement : Statement {
  std::shared_ptr<TestExpr> test_expression;
  std::shared_ptr<Statement> true_statement;
  std::shared_ptr<Statement> false_statement;
};

struct WhileStatement : Statement {
  std::shared_ptr<TestExpr> test_expression;
  std::shared_ptr<Statement> body_statement;
};

struct PrintStatement : Statement {
  std::shared_ptr<Expr> expression;
};

struct DeleteStatement : Statement {
  std::shared_ptr<Expr> expression;
};

template <typename Target>
std::shared_ptr<Target> convert(std::shared_ptr<ASTNode> node) {
  return std::dynamic_pointer_cast<Target>(node);
}

std::shared_ptr<ASTNode> parse_tree_to_ast(std::shared_ptr<ParseNode> node);
