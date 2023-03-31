
#pragma once

#include <memory>
#include <string>
#include <vector>

struct Variable;

struct Program;
struct Procedure;

struct VariableLValueExpr;
struct DereferenceLValueExpr;
struct TestExpr;
struct VariableExpr;
struct LiteralExpr;
struct BinaryExpr;
struct AddressOfExpr;
struct NewExpr;
struct FunctionCallExpr;

struct Statements;
struct AssignmentStatement;
struct IfStatement;
struct WhileStatement;
struct PrintStatement;
struct DeleteStatement;

struct ASTVisitor {
  virtual ~ASTVisitor() = default;

  virtual void pre_visit(Program &) {}
  virtual void pre_visit(Procedure &) {}
  virtual void pre_visit(VariableLValueExpr &) {}
  virtual void pre_visit(DereferenceLValueExpr &) {}
  virtual void pre_visit(TestExpr &) {}
  virtual void pre_visit(VariableExpr &) {}
  virtual void pre_visit(LiteralExpr &) {}
  virtual void pre_visit(BinaryExpr &) {}
  virtual void pre_visit(AddressOfExpr &) {}
  virtual void pre_visit(NewExpr &) {}
  virtual void pre_visit(FunctionCallExpr &) {}
  virtual void pre_visit(Statements &) {}
  virtual void pre_visit(AssignmentStatement &) {}
  virtual void pre_visit(IfStatement &) {}
  virtual void pre_visit(WhileStatement &) {}
  virtual void pre_visit(PrintStatement &) {}
  virtual void pre_visit(DeleteStatement &) {}

  virtual void post_visit(Program &) {}
  virtual void post_visit(Procedure &) {}
  virtual void post_visit(VariableLValueExpr &) {}
  virtual void post_visit(DereferenceLValueExpr &) {}
  virtual void post_visit(TestExpr &) {}
  virtual void post_visit(VariableExpr &) {}
  virtual void post_visit(LiteralExpr &) {}
  virtual void post_visit(BinaryExpr &) {}
  virtual void post_visit(AddressOfExpr &) {}
  virtual void post_visit(NewExpr &) {}
  virtual void post_visit(FunctionCallExpr &) {}
  virtual void post_visit(Statements &) {}
  virtual void post_visit(AssignmentStatement &) {}
  virtual void post_visit(IfStatement &) {}
  virtual void post_visit(WhileStatement &) {}
  virtual void post_visit(PrintStatement &) {}
  virtual void post_visit(DeleteStatement &) {}
};
