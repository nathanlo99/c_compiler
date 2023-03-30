
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

  Literal() = default;
  Literal(const int32_t value, const Type type) : value(value), type(type) {}
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

struct ArgumentList : ASTNode {
  std::vector<std::shared_ptr<Expr>> exprs;
  ArgumentList(const std::vector<std::shared_ptr<Expr>> &exprs = {})
      : exprs(exprs) {}
};

struct Statements : Statement {
  std::vector<Statement> statements;
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
  VariableLValueExpr(Variable variable) : variable(variable) {}
};

struct DereferenceLValueExpr : LValueExpr {
  std::shared_ptr<Expr> argument;
  DereferenceLValueExpr(std::shared_ptr<Expr> argument) : argument(argument) {}
};

struct TestExpr : Expr {
  std::shared_ptr<Expr> lhs;
  Token operation;
  std::shared_ptr<Expr> rhs;

  TestExpr(std::shared_ptr<Expr> lhs, Token operation,
           std::shared_ptr<Expr> rhs)
      : lhs(lhs), operation(operation), rhs(rhs) {}
};

struct VariableExpr : Expr {
  Variable variable;
  VariableExpr(Variable variable) : variable(variable) {}
};

struct LiteralExpr : Expr {
  Literal literal;
  LiteralExpr(Literal literal) : literal(literal) {}
};

struct BinaryExpr : Expr {
  std::shared_ptr<Expr> lhs;
  Token operation;
  std::shared_ptr<Expr> rhs;

  BinaryExpr(std::shared_ptr<Expr> lhs, Token operation,
             std::shared_ptr<Expr> rhs)
      : lhs(lhs), operation(operation), rhs(rhs) {}
};

struct AddressOfExpr : Expr {
  std::shared_ptr<LValueExpr> argument;
  AddressOfExpr(std::shared_ptr<LValueExpr> argument) : argument(argument) {}
};

struct NewExpr : Expr {
  std::shared_ptr<Expr> rhs;
  NewExpr(std::shared_ptr<Expr> rhs) : rhs(rhs) {}
};

struct FunctionCallExpr : Expr {
  std::string procedure_name;
  std::vector<std::shared_ptr<Expr>> arguments;

  FunctionCallExpr(std::string procedure_name,
                   const std::vector<std::shared_ptr<Expr>> &arguments = {})
      : procedure_name(procedure_name), arguments(arguments) {}
};

// Statements
struct AssignmentStatement : Statement {
  std::shared_ptr<LValueExpr> lhs;
  std::shared_ptr<Expr> rhs;

  AssignmentStatement(std::shared_ptr<LValueExpr> lhs,
                      std::shared_ptr<Expr> rhs)
      : lhs(lhs), rhs(rhs) {}
};

struct IfStatement : Statement {
  std::shared_ptr<TestExpr> test_expression;
  std::shared_ptr<Statement> true_statement;
  std::shared_ptr<Statement> false_statement;

  IfStatement(std::shared_ptr<TestExpr> test_expression,
              std::shared_ptr<Statement> true_statement,
              std::shared_ptr<Statement> false_statement)
      : test_expression(test_expression), true_statement(true_statement),
        false_statement(false_statement) {}
};

struct WhileStatement : Statement {
  std::shared_ptr<TestExpr> test_expression;
  std::shared_ptr<Statement> body_statement;

  WhileStatement(std::shared_ptr<TestExpr> test_expression,
                 std::shared_ptr<Statement> body_statement)
      : test_expression(test_expression), body_statement(body_statement) {}
};

struct PrintStatement : Statement {
  std::shared_ptr<Expr> expression;
  PrintStatement(std::shared_ptr<Expr> expression) : expression(expression) {}
};

struct DeleteStatement : Statement {
  std::shared_ptr<Expr> expression;
  DeleteStatement(std::shared_ptr<Expr> expression) : expression(expression) {}
};

template <typename Target>
std::shared_ptr<Target> convert(std::shared_ptr<ASTNode> node) {
  return std::dynamic_pointer_cast<Target>(node);
}

std::shared_ptr<ASTNode> parse_tree_to_ast(std::shared_ptr<ParseNode> node);
