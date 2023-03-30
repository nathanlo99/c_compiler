
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "parse_node.hpp"

#include "types.hpp"
#include "util.hpp"

static std::string get_padding(const size_t depth) {
  return std::string(2 * depth, ' ');
}

static std::string type_to_string(const Type type) {
  switch (type) {
  case Type::Int:
    return "int";
  case Type::IntStar:
    return "int*";
  default:
    return "??";
  }
}

struct ASTNode {
  Type type;
  virtual ~ASTNode() {}
  virtual void print(const size_t depth = 0) const = 0;
};

struct Expr : ASTNode {};
struct Statement : ASTNode {};

struct Literal : ASTNode {
  int32_t value = 0;
  Type type = Type::Int;

  Literal() = default;
  Literal(const int32_t value, const Type type) : value(value), type(type) {}
  virtual ~Literal() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << value << ": " << type_to_string(type)
              << std::endl;
  }
};

struct Variable {
  std::string name;
  Type type;
  Literal initial_value;
  Variable(const std::string &name, const Type type,
           const Literal initial_value)
      : name(name), type(type), initial_value(initial_value) {}
  virtual ~Variable() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << name << ": " << type_to_string(type)
              << " = " << initial_value.value << std::endl;
  }
};

struct ParameterList : ASTNode {
  std::vector<Variable> parameters;
  ParameterList(const std::vector<Variable> &variables = {})
      : parameters(variables) {}
  virtual ~ParameterList() = default;

  virtual void print(const size_t) const { unreachable(""); }
};

struct DeclarationList : ASTNode {
  std::vector<Variable> declarations;
  DeclarationList(const std::vector<Variable> &variables = {})
      : declarations(variables) {}
  virtual ~DeclarationList() = default;

  virtual void print(const size_t) const { unreachable(""); }
};

struct ArgumentList : ASTNode {
  std::vector<std::shared_ptr<Expr>> exprs;
  ArgumentList(const std::vector<std::shared_ptr<Expr>> &exprs = {})
      : exprs(exprs) {}
  virtual ~ArgumentList() = default;

  virtual void print(const size_t) const { unreachable(""); }
};

struct Statements : Statement {
  std::vector<std::shared_ptr<Statement>> statements;

  virtual ~Statements() = default;

  virtual void print(const size_t depth = 0) const {
    for (const auto &statement : statements) {
      statement->print(depth);
    }
  }
};

struct Procedure : ASTNode {
  std::string name;
  std::vector<Variable> params;
  Type return_type;

  std::vector<Variable> decls;
  std::shared_ptr<Statements> statements;
  std::shared_ptr<Expr> return_expr;

  Procedure(const std::string &name, std::shared_ptr<ParameterList> params,
            const Type type, std::shared_ptr<DeclarationList> decls,
            std::shared_ptr<Statements> statements,
            std::shared_ptr<Expr> return_expr)
      : name(name), params(params->parameters), return_type(type),
        decls(decls->declarations), statements(statements),
        return_expr(return_expr) {}
  virtual ~Procedure() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "Procedure {" << std::endl;
    std::cout << get_padding(depth + 1) << "name: " << name << std::endl;
    std::cout << get_padding(depth + 1)
              << "return_type: " << type_to_string(return_type) << std::endl;
    std::cout << get_padding(depth + 1) << "parameters: " << std::endl;
    for (const Variable &variable : params) {
      std::cout << get_padding(depth + 2) << variable.name << ": "
                << type_to_string(variable.type) << std::endl;
    }
    std::cout << get_padding(depth + 1) << "declarations: " << std::endl;
    for (const Variable &variable : decls) {
      std::cout << get_padding(depth + 2) << variable.name << ": "
                << type_to_string(variable.type) << " = "
                << variable.initial_value.value << std::endl;
    }
    std::cout << get_padding(depth + 1) << "statements: " << std::endl;
    statements->print(depth + 2);
    std::cout << get_padding(depth + 1) << "return_expr: " << std::endl;
    return_expr->print(depth + 2);
    std::cout << get_padding(depth) << "}" << std::endl;
  }
};

struct Program : ASTNode {
  std::vector<Procedure> procedures;
  virtual ~Program() = default;

  virtual void print(const size_t depth = 0) const {
    for (const auto &procedure : procedures) {
      procedure.print(depth);
      std::cout << std::endl;
    }
  }
};

// Expressions
struct LValueExpr : Expr {};

struct VariableLValueExpr : LValueExpr {
  Variable variable;
  VariableLValueExpr(Variable variable) : variable(variable) {}
  virtual ~VariableLValueExpr() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "VariableLValueExpr(" << variable.name
              << ")" << std::endl;
  }
};

struct DereferenceLValueExpr : LValueExpr {
  std::shared_ptr<Expr> argument;
  DereferenceLValueExpr(std::shared_ptr<Expr> argument) : argument(argument) {}
  virtual ~DereferenceLValueExpr() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "DereferenceLValueExpr {" << std::endl;
    argument->print(depth + 1);
    std::cout << get_padding(depth) << "}" << std::endl;
  }
};

struct TestExpr : Expr {
  std::shared_ptr<Expr> lhs;
  Token operation;
  std::shared_ptr<Expr> rhs;

  TestExpr(std::shared_ptr<Expr> lhs, Token operation,
           std::shared_ptr<Expr> rhs)
      : lhs(lhs), operation(operation), rhs(rhs) {}
  virtual ~TestExpr() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "TestExpr {" << std::endl;
    std::cout << get_padding(depth + 1) << "lhs: " << std::endl;
    lhs->print(depth + 2);
    std::cout << get_padding(depth + 1) << "operation: " << operation.lexeme
              << std::endl;
    std::cout << get_padding(depth + 1) << "rhs: " << std::endl;
    rhs->print(depth + 2);
    std::cout << get_padding(depth) << "}" << std::endl;
  }
};

struct VariableExpr : Expr {
  Variable variable;
  VariableExpr(Variable variable) : variable(variable) {}
  virtual ~VariableExpr() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "VariableExpr(" << variable.name << ")"
              << std::endl;
  }
};

struct LiteralExpr : Expr {
  Literal literal;
  LiteralExpr(Literal literal) : literal(literal) {}
  virtual ~LiteralExpr() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << literal.value << ": "
              << type_to_string(literal.type) << std::endl;
  }
};

struct BinaryExpr : Expr {
  std::shared_ptr<Expr> lhs;
  Token operation;
  std::shared_ptr<Expr> rhs;

  BinaryExpr(std::shared_ptr<Expr> lhs, Token operation,
             std::shared_ptr<Expr> rhs)
      : lhs(lhs), operation(operation), rhs(rhs) {}
  virtual ~BinaryExpr() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "BinaryExpr {" << std::endl;
    std::cout << get_padding(depth + 1) << "lhs: " << std::endl;
    lhs->print(depth + 2);
    std::cout << get_padding(depth + 1) << "operation: " << operation.lexeme
              << std::endl;
    std::cout << get_padding(depth + 1) << "rhs: " << std::endl;
    rhs->print(depth + 2);
    std::cout << get_padding(depth) << "}" << std::endl;
  }
};

struct AddressOfExpr : Expr {
  std::shared_ptr<LValueExpr> argument;
  AddressOfExpr(std::shared_ptr<LValueExpr> argument) : argument(argument) {}
  virtual ~AddressOfExpr() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "AddressOfExpr {" << std::endl;
    argument->print(depth + 1);
    std::cout << get_padding(depth) << "}" << std::endl;
  }
};

struct NewExpr : Expr {
  std::shared_ptr<Expr> rhs;
  NewExpr(std::shared_ptr<Expr> rhs) : rhs(rhs) {}
  virtual ~NewExpr() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "NewExpr {" << std::endl;
    rhs->print(depth + 1);
    std::cout << get_padding(depth) << "}" << std::endl;
  }
};

struct FunctionCallExpr : Expr {
  std::string procedure_name;
  std::vector<std::shared_ptr<Expr>> arguments;

  FunctionCallExpr(std::string procedure_name,
                   const std::vector<std::shared_ptr<Expr>> &arguments = {})
      : procedure_name(procedure_name), arguments(arguments) {}
  virtual ~FunctionCallExpr() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "FunctionCall {" << std::endl;
    std::cout << get_padding(depth + 1) << "procedure_name: " << procedure_name
              << "," << std::endl;
    std::cout << get_padding(depth + 1) << "arguments: [" << std::endl;
    for (const auto &expr : arguments) {
      expr->print(depth + 2);
    }
    std::cout << get_padding(depth + 1) << "]" << std::endl;
    std::cout << get_padding(depth) << "}" << std::endl;
  }
};

// Statements
struct AssignmentStatement : Statement {
  std::shared_ptr<LValueExpr> lhs;
  std::shared_ptr<Expr> rhs;

  AssignmentStatement(std::shared_ptr<LValueExpr> lhs,
                      std::shared_ptr<Expr> rhs)
      : lhs(lhs), rhs(rhs) {}
  virtual ~AssignmentStatement() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "AssignmentStatement {" << std::endl;
    std::cout << get_padding(depth + 1) << "lhs: " << std::endl;
    lhs->print(depth + 2);
    std::cout << get_padding(depth + 1) << "rhs: " << std::endl;
    rhs->print(depth + 2);
    std::cout << get_padding(depth) << ")" << std::endl;
  }
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

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "IfStatement {" << std::endl;
    std::cout << get_padding(depth + 1) << "condition: " << std::endl;
    test_expression->print(depth + 2);
    std::cout << get_padding(depth + 1) << "true_statement: " << std::endl;
    true_statement->print(depth + 2);
    std::cout << get_padding(depth + 1) << "false_statement: " << std::endl;
    false_statement->print(depth + 2);
    std::cout << get_padding(depth) << "}" << std::endl;
  }
};

struct WhileStatement : Statement {
  std::shared_ptr<TestExpr> test_expression;
  std::shared_ptr<Statement> body_statement;

  WhileStatement(std::shared_ptr<TestExpr> test_expression,
                 std::shared_ptr<Statement> body_statement)
      : test_expression(test_expression), body_statement(body_statement) {}
  virtual ~WhileStatement() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "WhileStatement {" << std::endl;
    std::cout << get_padding(depth + 1) << "condition: " << std::endl;
    test_expression->print(depth + 2);
    std::cout << get_padding(depth + 1) << "body: " << std::endl;
    body_statement->print(depth + 2);
    std::cout << get_padding(depth) << "}" << std::endl;
  }
};

struct PrintStatement : Statement {
  std::shared_ptr<Expr> expression;
  PrintStatement(std::shared_ptr<Expr> expression) : expression(expression) {}
  virtual ~PrintStatement() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "PrintStatement {" << std::endl;
    expression->print(depth + 1);
    std::cout << get_padding(depth) << "}" << std::endl;
  }
};

struct DeleteStatement : Statement {
  std::shared_ptr<Expr> expression;
  DeleteStatement(std::shared_ptr<Expr> expression) : expression(expression) {}
  virtual ~DeleteStatement() = default;

  virtual void print(const size_t depth = 0) const {
    std::cout << get_padding(depth) << "DeleteStatement {" << std::endl;
    expression->print(depth + 1);
    std::cout << get_padding(depth) << "}" << std::endl;
  }
};

template <typename Target>
std::shared_ptr<Target> convert(std::shared_ptr<ASTNode> node) {
  return std::dynamic_pointer_cast<Target>(node);
}

std::shared_ptr<ASTNode> parse_tree_to_ast(std::shared_ptr<ParseNode> node);
