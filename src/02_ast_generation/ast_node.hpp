
#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ast_base.hpp"
#include "lexer.hpp"
#include "parse_node.hpp"
#include "symbol_table.hpp"

#include "ast_recursive_visitor.hpp"
#include "ast_simple_visitor.hpp"

#include "types.hpp"
#include "util.hpp"

struct ASTNode {
  virtual ~ASTNode() {}
  virtual void print(const size_t depth) const = 0;
  virtual void emit_c(std::ostream &os, const size_t indent_level) const = 0;
  virtual void accept_simple(ASTSimpleVisitor &visitor) = 0;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) = 0;
  virtual std::string node_type() const = 0;
};

struct Expr : ASTNode {
  Type type;
  Expr(Type type = Type::Unknown) : type(type) {}
};
struct Statement : ASTNode {};

struct ParameterList : ASTNode {
  std::vector<Variable> parameters;
  ParameterList(const std::vector<Variable> &variables = {})
      : parameters(variables) {}
  virtual ~ParameterList() = default;

  virtual void print(const size_t) const override { unreachable(""); }
  virtual void emit_c(std::ostream &, const size_t) const override {
    unreachable("");
  }
  virtual void accept_simple(ASTSimpleVisitor &) override { unreachable(""); }
  virtual void accept_recursive(ASTRecursiveVisitor &) override {
    unreachable("");
  }
  virtual std::string node_type() const override { return "ParameterList"; }
};

struct DeclarationList : ASTNode {
  std::vector<Variable> declarations;
  DeclarationList(const std::vector<Variable> &variables = {})
      : declarations(variables) {}
  virtual ~DeclarationList() = default;

  virtual void print(const size_t) const override { unreachable(""); }
  virtual void emit_c(std::ostream &, const size_t) const override {
    unreachable("");
  }
  virtual void accept_simple(ASTSimpleVisitor &) override { unreachable(""); }
  virtual void accept_recursive(ASTRecursiveVisitor &) override {
    unreachable("");
  }
  virtual std::string node_type() const override { return "DeclarationList"; }
};

struct ArgumentList : ASTNode {
  std::vector<std::shared_ptr<Expr>> exprs;
  ArgumentList(const std::vector<std::shared_ptr<Expr>> &exprs = {})
      : exprs(exprs) {}
  virtual ~ArgumentList() = default;

  virtual void print(const size_t) const override { unreachable(""); }
  virtual void emit_c(std::ostream &, const size_t) const override {
    unreachable("");
  }
  virtual void accept_simple(ASTSimpleVisitor &) override { unreachable(""); }
  virtual void accept_recursive(ASTRecursiveVisitor &) override {
    unreachable("");
  }
  virtual std::string node_type() const override { return "ArgumentList"; }
};

struct Statements : Statement {
  std::vector<std::shared_ptr<Statement>> statements;
  Statements(const std::vector<std::shared_ptr<Statement>> &statements = {})
      : statements(statements) {}
  virtual ~Statements() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "Statements"; }
};

struct Procedure : ASTNode {
  std::string name;
  std::vector<Variable> params;
  Type return_type;

  std::vector<Variable> decls;
  std::vector<std::shared_ptr<Statement>> statements;
  std::shared_ptr<Expr> return_expr;

  ProcedureTable table;

  Procedure(const std::string &name, std::shared_ptr<ParameterList> params,
            const Type type, std::shared_ptr<DeclarationList> decls,
            std::shared_ptr<Statements> statements,
            std::shared_ptr<Expr> return_expr)
      : name(name), params(params->parameters), return_type(type),
        decls(decls->declarations), statements(statements->statements),
        return_expr(return_expr), table(name) {}
  virtual ~Procedure() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "Procedure"; }
};

struct Program : ASTNode {
  std::vector<Procedure> procedures;
  SymbolTable table;
  virtual ~Program() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "Program"; }
};

// Expressions
struct LValueExpr : Expr {};

struct VariableLValueExpr : LValueExpr {
  Variable variable;
  VariableLValueExpr(Variable variable) : variable(variable) {}
  virtual ~VariableLValueExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override {
    return "VariableLValueExpr";
  }
};

struct DereferenceLValueExpr : LValueExpr {
  std::shared_ptr<Expr> argument;
  DereferenceLValueExpr(std::shared_ptr<Expr> argument) : argument(argument) {}
  virtual ~DereferenceLValueExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override {
    return "DereferenceLValueExpr";
  }
};

struct VariableExpr : Expr {
  Variable variable;
  VariableExpr(const Variable &variable)
      : Expr(variable.type), variable(variable) {}
  virtual ~VariableExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "VariableExpr"; }
};

struct LiteralExpr : Expr {
  Literal literal;
  LiteralExpr(const Literal &literal) : Expr(literal.type), literal(literal) {}
  LiteralExpr(const int64_t value, const Type type)
      : Expr(type), literal(value, type) {}
  virtual ~LiteralExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "LiteralExpr"; }
};

struct AssignmentExpr : Expr {
  std::shared_ptr<LValueExpr> lhs;
  std::shared_ptr<Expr> rhs;
  AssignmentExpr(std::shared_ptr<LValueExpr> lhs, std::shared_ptr<Expr> rhs)
      : Expr(lhs->type), lhs(lhs), rhs(rhs) {}
  virtual ~AssignmentExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "AssignmentExpr"; }
};

enum class BooleanOperation {
  LessThan,
  LessEqual,
  GreaterThan,
  GreaterEqual,
  Equal,
  NotEqual,
};

constexpr const char *
comparison_operation_to_string(const BooleanOperation op) {
  switch (op) {
  case BooleanOperation::LessThan:
    return "LessThan";
  case BooleanOperation::LessEqual:
    return "LessEqual";
  case BooleanOperation::GreaterThan:
    return "GreaterThan";
  case BooleanOperation::GreaterEqual:
    return "GreaterEqual";
  case BooleanOperation::Equal:
    return "Equal";
  case BooleanOperation::NotEqual:
    return "NotEqual";
  default:
    unreachable("");
  }
  __builtin_unreachable();
}

constexpr BooleanOperation
token_to_comparison_operation(const TokenKind operation) {
  switch (operation) {
  case TokenKind::Lt:
    return BooleanOperation::LessThan;
  case TokenKind::Le:
    return BooleanOperation::LessEqual;
  case TokenKind::Gt:
    return BooleanOperation::GreaterThan;
  case TokenKind::Ge:
    return BooleanOperation::GreaterEqual;
  case TokenKind::Eq:
    return BooleanOperation::Equal;
  case TokenKind::Ne:
    return BooleanOperation::NotEqual;
  default:
    debug_assert(false,
                 "Could not convert invalid type {} to comparison operation",
                 token_kind_to_string(operation));
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
  default:
    unreachable("");
  }
  __builtin_unreachable();
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
    debug_assert(false, "Could not convert invalid type {} to binary operation",
                 token_kind_to_string(operation));
  }
  __builtin_unreachable();
}

struct TestExpr : Expr {
  std::shared_ptr<Expr> lhs;
  BooleanOperation operation;
  std::shared_ptr<Expr> rhs;

  TestExpr(std::shared_ptr<Expr> lhs, const BooleanOperation operation,
           std::shared_ptr<Expr> rhs)
      : lhs(lhs), operation(operation), rhs(rhs) {}
  TestExpr(std::shared_ptr<Expr> value) {
    if (const auto &test_expr = std::dynamic_pointer_cast<TestExpr>(value);
        test_expr) {
      lhs = test_expr->lhs;
      operation = test_expr->operation;
      rhs = test_expr->rhs;
    } else {
      lhs = value;
      rhs = std::make_shared<LiteralExpr>(0, Type::Int);
      operation = BooleanOperation::NotEqual;
    }
  }
  virtual ~TestExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "TestExpr"; }
};

struct BinaryExpr : Expr {
  std::shared_ptr<Expr> lhs;
  BinaryOperation operation;
  std::shared_ptr<Expr> rhs;

  BinaryExpr(std::shared_ptr<Expr> lhs, const BinaryOperation operation,
             std::shared_ptr<Expr> rhs)
      : lhs(lhs), operation(operation), rhs(rhs) {}
  virtual ~BinaryExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "BinaryExpr"; }
};

struct AddressOfExpr : Expr {
  std::shared_ptr<VariableLValueExpr> argument;
  AddressOfExpr(std::shared_ptr<VariableLValueExpr> argument)
      : argument(argument) {}
  virtual ~AddressOfExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "AddressOfExpr"; }
};

struct DereferenceExpr : Expr {
  std::shared_ptr<Expr> argument;
  DereferenceExpr(std::shared_ptr<Expr> argument) : argument(argument) {}
  virtual ~DereferenceExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "DereferenceExpr"; }
};

struct NewExpr : Expr {
  std::shared_ptr<Expr> rhs;
  NewExpr(std::shared_ptr<Expr> rhs) : rhs(rhs) {}
  virtual ~NewExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "NewExpr"; }
};

struct FunctionCallExpr : Expr {
  std::string procedure_name;
  std::vector<std::shared_ptr<Expr>> arguments;

  FunctionCallExpr(const std::string &procedure_name,
                   const std::vector<std::shared_ptr<Expr>> &arguments = {})
      : procedure_name(procedure_name), arguments(arguments) {}
  virtual ~FunctionCallExpr() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "FunctionCallExpr"; }
};

// Statements
struct ExprStatement : Statement {
  std::shared_ptr<Expr> expr;

  ExprStatement(std::shared_ptr<Expr> expr) : expr(expr) {}
  virtual ~ExprStatement() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "ExprStatement"; }
};

struct AssignmentStatement : Statement {
  std::shared_ptr<LValueExpr> lhs;
  std::shared_ptr<Expr> rhs;

  AssignmentStatement(std::shared_ptr<LValueExpr> lhs,
                      std::shared_ptr<Expr> rhs)
      : lhs(lhs), rhs(rhs) {}
  virtual ~AssignmentStatement() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override {
    return "AssignmentStatement";
  }
};

struct IfStatement : Statement {
  std::shared_ptr<TestExpr> test_expression;
  Statements true_statements;
  Statements false_statements;

  IfStatement(std::shared_ptr<Expr> cond, const Statements &true_statements,
              const Statements &false_statements)
      : test_expression(std::make_shared<TestExpr>(cond)),
        true_statements(true_statements), false_statements(false_statements) {}
  virtual ~IfStatement() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "IfStatement"; }
};

struct WhileStatement : Statement {
  std::shared_ptr<TestExpr> test_expression;
  std::shared_ptr<Statement> body_statement;

  WhileStatement(std::shared_ptr<Expr> test_expression,
                 std::shared_ptr<Statement> body_statement)
      : test_expression(std::make_shared<TestExpr>(test_expression)),
        body_statement(body_statement) {}
  virtual ~WhileStatement() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "WhileStatement"; }
};

struct PrintStatement : Statement {
  std::shared_ptr<Expr> expression;
  PrintStatement(std::shared_ptr<Expr> expression) : expression(expression) {}
  virtual ~PrintStatement() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "PrintStatement"; }
};

struct DeleteStatement : Statement {
  std::shared_ptr<Expr> expression;
  DeleteStatement(std::shared_ptr<Expr> expression) : expression(expression) {}
  virtual ~DeleteStatement() = default;

  virtual void print(const size_t depth = 0) const override;
  virtual void emit_c(std::ostream &os,
                      const size_t indent_level) const override;
  virtual void accept_simple(ASTSimpleVisitor &visitor) override;
  virtual void accept_recursive(ASTRecursiveVisitor &visitor) override;
  virtual std::string node_type() const override { return "DeleteStatement"; }
};

std::shared_ptr<ASTNode> construct_ast(const std::shared_ptr<ParseNode> &node);

template <typename Target>
std::shared_ptr<Target> construct_ast(const std::shared_ptr<ParseNode> &node) {
  const auto ast = construct_ast(node);
  const auto result = std::dynamic_pointer_cast<Target>(ast);
  if (result == nullptr) {
    std::cerr << "BAD:" << std::endl;
    ast->print(0);
  }
  debug_assert(result != nullptr, "Unexpected AST node type: {}",
               ast->node_type());
  return result;
}
