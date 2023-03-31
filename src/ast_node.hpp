
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "lexer.hpp"
#include "parse_node.hpp"

#include "ast_visitor.hpp"
#include "types.hpp"
#include "util.hpp"

struct ASTNode {
  virtual ~ASTNode() {}
  virtual void print(const size_t depth) const = 0;
  virtual void visit(ASTVisitor &visitor) = 0;
};

struct Expr : ASTNode {
  Type type;
};
struct Statement : ASTNode {};

struct Literal {
  int32_t value = 0;
  Type type = Type::Int;

  Literal() = default;
  Literal(const int32_t value, const Type type) : value(value), type(type) {}

  void print(const size_t depth) const;
};

struct Variable {
  std::string name;
  Type type;
  Literal initial_value;
  Variable(const std::string &name, const Type type,
           const Literal initial_value)
      : name(name), type(type), initial_value(initial_value) {}

  void print(const size_t depth) const;
};

struct ParameterList : ASTNode {
  std::vector<Variable> parameters;
  ParameterList(const std::vector<Variable> &variables = {})
      : parameters(variables) {}
  virtual ~ParameterList() = default;

  virtual void print(const size_t) const override { unreachable(""); }
  virtual void visit(ASTVisitor &) override { unreachable(""); }
};

struct DeclarationList : ASTNode {
  std::vector<Variable> declarations;
  DeclarationList(const std::vector<Variable> &variables = {})
      : declarations(variables) {}
  virtual ~DeclarationList() = default;

  virtual void print(const size_t) const override { unreachable(""); }
  virtual void visit(ASTVisitor &) override { unreachable(""); }
};

struct ArgumentList : ASTNode {
  std::vector<std::shared_ptr<Expr>> exprs;
  ArgumentList(const std::vector<std::shared_ptr<Expr>> &exprs = {})
      : exprs(exprs) {}
  virtual ~ArgumentList() = default;

  virtual void print(const size_t) const override { unreachable(""); }
  virtual void visit(ASTVisitor &) override { unreachable(""); }
};

struct Statements : Statement {
  std::vector<std::shared_ptr<Statement>> statements;
  Statements(const std::vector<std::shared_ptr<Statement>> &statements = {})
      : statements(statements) {}
  virtual ~Statements() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

struct Procedure : ASTNode {
  std::string name;
  std::vector<Variable> params;
  Type return_type;

  std::vector<Variable> decls;
  std::vector<std::shared_ptr<Statement>> statements;
  std::shared_ptr<Expr> return_expr;

  Procedure(const std::string &name, std::shared_ptr<ParameterList> params,
            const Type type, std::shared_ptr<DeclarationList> decls,
            std::shared_ptr<Statements> statements,
            std::shared_ptr<Expr> return_expr)
      : name(name), params(params->parameters), return_type(type),
        decls(decls->declarations), statements(statements->statements),
        return_expr(return_expr) {}
  virtual ~Procedure() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

struct Program : ASTNode {
  std::vector<Procedure> procedures;
  virtual ~Program() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

// Expressions
struct LValueExpr : Expr {};

struct VariableLValueExpr : LValueExpr {
  Variable variable;
  VariableLValueExpr(Variable variable) : variable(variable) {}
  virtual ~VariableLValueExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

struct DereferenceLValueExpr : LValueExpr {
  std::shared_ptr<Expr> argument;
  DereferenceLValueExpr(std::shared_ptr<Expr> argument) : argument(argument) {}
  virtual ~DereferenceLValueExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

struct VariableExpr : Expr {
  Variable variable;
  VariableExpr(const Variable &variable) : variable(variable) {}
  virtual ~VariableExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

struct LiteralExpr : Expr {
  Literal literal;
  LiteralExpr(const Literal &literal) : literal(literal) {
    this->type = literal.type;
  }
  LiteralExpr(const int32_t value, const Type type) : literal(value, type) {
    this->type = type;
  }
  virtual ~LiteralExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

enum class ComparisonOperation {
  LessThan,
  LessEqual,
  GreaterThan,
  GreaterEqual,
  Equal,
  NotEqual,
};

constexpr const char *
comparison_operation_to_string(const ComparisonOperation op) {
  switch (op) {
  case ComparisonOperation::LessThan:
    return "LessThan";
  case ComparisonOperation::LessEqual:
    return "LessEqual";
  case ComparisonOperation::GreaterThan:
    return "GreaterThan";
  case ComparisonOperation::GreaterEqual:
    return "GreaterEqual";
  case ComparisonOperation::Equal:
    return "Equal";
  case ComparisonOperation::NotEqual:
    return "NotEqual";
  }
}

constexpr ComparisonOperation
token_to_comparison_operation(const TokenKind operation) {
  switch (operation) {
  case TokenKind::Lt:
    return ComparisonOperation::LessThan;
  case TokenKind::Le:
    return ComparisonOperation::LessEqual;
  case TokenKind::Gt:
    return ComparisonOperation::GreaterThan;
  case TokenKind::Ge:
    return ComparisonOperation::GreaterEqual;
  case TokenKind::Eq:
    return ComparisonOperation::Equal;
  case TokenKind::Ne:
    return ComparisonOperation::NotEqual;
  default:
    runtime_assert(false, "Could not convert invalid type " +
                              token_kind_to_string(operation) +
                              " to comparison operation");
  }
  __builtin_unreachable();
}

enum class BinaryOperation {
  Add,
  Sub,
  Mul,
  Div,
  Mod,
};

constexpr inline const char *
binary_operation_to_string(const BinaryOperation op) {
  switch (op) {
  case BinaryOperation::Add:
    return "Add";
  case BinaryOperation::Sub:
    return "Sub";
  case BinaryOperation::Mul:
    return "Mul";
  case BinaryOperation::Div:
    return "Div";
  case BinaryOperation::Mod:
    return "Mod";
  }
}

constexpr inline BinaryOperation
token_to_binary_operation(const TokenKind operation) {
  switch (operation) {
  case TokenKind::Plus:
    return BinaryOperation::Add;
  case TokenKind::Minus:
    return BinaryOperation::Sub;
  case TokenKind::Star:
    return BinaryOperation::Mul;
  case TokenKind::Slash:
    return BinaryOperation::Div;
  case TokenKind::Pct:
    return BinaryOperation::Mod;
  default:
    runtime_assert(false, "Could not convert invalid type " +
                              token_kind_to_string(operation) +
                              " to binary operation");
  }
  __builtin_unreachable();
}

struct TestExpr : Expr {

  std::shared_ptr<Expr> lhs;
  ComparisonOperation operation;
  std::shared_ptr<Expr> rhs;

  TestExpr(std::shared_ptr<Expr> lhs, const Token operation,
           std::shared_ptr<Expr> rhs)
      : lhs(lhs), operation(token_to_comparison_operation(operation.kind)),
        rhs(rhs) {}
  virtual ~TestExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

struct BinaryExpr : Expr {
  std::shared_ptr<Expr> lhs;
  BinaryOperation operation;
  std::shared_ptr<Expr> rhs;

  BinaryExpr(std::shared_ptr<Expr> lhs, const Token operation,
             std::shared_ptr<Expr> rhs)
      : lhs(lhs), operation(token_to_binary_operation(operation.kind)),
        rhs(rhs) {}
  virtual ~BinaryExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

struct AddressOfExpr : Expr {
  std::shared_ptr<LValueExpr> argument;
  AddressOfExpr(std::shared_ptr<LValueExpr> argument) : argument(argument) {}
  virtual ~AddressOfExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

struct NewExpr : Expr {
  std::shared_ptr<Expr> rhs;
  NewExpr(std::shared_ptr<Expr> rhs) : rhs(rhs) {}
  virtual ~NewExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

struct FunctionCallExpr : Expr {
  std::string procedure_name;
  std::vector<std::shared_ptr<Expr>> arguments;

  FunctionCallExpr(const std::string &procedure_name,
                   const std::vector<std::shared_ptr<Expr>> &arguments = {})
      : procedure_name(procedure_name), arguments(arguments) {}
  virtual ~FunctionCallExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

// Statements
struct AssignmentStatement : Statement {
  std::shared_ptr<LValueExpr> lhs;
  std::shared_ptr<Expr> rhs;

  AssignmentStatement(std::shared_ptr<LValueExpr> lhs,
                      std::shared_ptr<Expr> rhs)
      : lhs(lhs), rhs(rhs) {}
  virtual ~AssignmentStatement() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
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
  virtual ~IfStatement() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

struct WhileStatement : Statement {
  std::shared_ptr<TestExpr> test_expression;
  std::shared_ptr<Statement> body_statement;

  WhileStatement(std::shared_ptr<TestExpr> test_expression,
                 std::shared_ptr<Statement> body_statement)
      : test_expression(test_expression), body_statement(body_statement) {}
  virtual ~WhileStatement() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

struct PrintStatement : Statement {
  std::shared_ptr<Expr> expression;
  PrintStatement(std::shared_ptr<Expr> expression) : expression(expression) {}
  virtual ~PrintStatement() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

struct DeleteStatement : Statement {
  std::shared_ptr<Expr> expression;
  DeleteStatement(std::shared_ptr<Expr> expression) : expression(expression) {}
  virtual ~DeleteStatement() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void visit(ASTVisitor &visitor) override;
};

std::shared_ptr<ASTNode> construct_ast(std::shared_ptr<ParseNode> node);

template <typename Target>
std::shared_ptr<Target> construct_ast(std::shared_ptr<ParseNode> node) {
  const auto result = std::dynamic_pointer_cast<Target>(construct_ast(node));
  runtime_assert(result != nullptr, "Unexpected AST node type");
  return result;
}
