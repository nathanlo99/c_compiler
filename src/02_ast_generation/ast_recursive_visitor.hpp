
#pragma once

#include "ast_simple_visitor.hpp"
#include <memory>
#include <string>
#include <vector>

struct Variable;

struct Program;
struct Procedure;

struct VariableLValueExpr;
struct DereferenceLValueExpr;
struct AssignmentExpr;
struct VariableExpr;
struct LiteralExpr;
struct BinaryExpr;
struct BooleanAndExpr;
struct BooleanOrExpr;
struct AddressOfExpr;
struct DereferenceExpr;
struct NewExpr;
struct FunctionCallExpr;

struct Statements;
struct ExprStatement;
struct AssignmentStatement;
struct IfStatement;
struct WhileStatement;
struct PrintStatement;
struct DeleteStatement;
struct BreakStatement;
struct ContinueStatement;

struct ASTRecursiveVisitor {
  virtual ~ASTRecursiveVisitor() = default;

  virtual void pre_visit(Program &) = 0;
  virtual void pre_visit(Procedure &) = 0;
  virtual void pre_visit(VariableLValueExpr &) = 0;
  virtual void pre_visit(DereferenceLValueExpr &) = 0;
  virtual void pre_visit(AssignmentExpr &) = 0;
  virtual void pre_visit(VariableExpr &) = 0;
  virtual void pre_visit(LiteralExpr &) = 0;
  virtual void pre_visit(BinaryExpr &) = 0;
  virtual void pre_visit(BooleanOrExpr &) = 0;
  virtual void pre_visit(BooleanAndExpr &) = 0;
  virtual void pre_visit(AddressOfExpr &) = 0;
  virtual void pre_visit(DereferenceExpr &) = 0;
  virtual void pre_visit(NewExpr &) = 0;
  virtual void pre_visit(FunctionCallExpr &) = 0;
  virtual void pre_visit(Statements &) = 0;
  virtual void pre_visit(ExprStatement &) = 0;
  virtual void pre_visit(AssignmentStatement &) = 0;
  virtual void pre_visit(IfStatement &) = 0;
  virtual void pre_visit(WhileStatement &) = 0;
  virtual void pre_visit(ForStatement &) = 0;
  virtual void pre_visit(PrintStatement &) = 0;
  virtual void pre_visit(DeleteStatement &) = 0;
  virtual void pre_visit(BreakStatement &) = 0;
  virtual void pre_visit(ContinueStatement &) = 0;

  virtual void post_visit(Program &) = 0;
  virtual void post_visit(Procedure &) = 0;
  virtual void post_visit(VariableLValueExpr &) = 0;
  virtual void post_visit(DereferenceLValueExpr &) = 0;
  virtual void post_visit(AssignmentExpr &) = 0;
  virtual void post_visit(VariableExpr &) = 0;
  virtual void post_visit(LiteralExpr &) = 0;
  virtual void post_visit(BinaryExpr &) = 0;
  virtual void post_visit(BooleanOrExpr &) = 0;
  virtual void post_visit(BooleanAndExpr &) = 0;
  virtual void post_visit(AddressOfExpr &) = 0;
  virtual void post_visit(DereferenceExpr &) = 0;
  virtual void post_visit(NewExpr &) = 0;
  virtual void post_visit(FunctionCallExpr &) = 0;
  virtual void post_visit(Statements &) = 0;
  virtual void post_visit(ExprStatement &) = 0;
  virtual void post_visit(AssignmentStatement &) = 0;
  virtual void post_visit(IfStatement &) = 0;
  virtual void post_visit(WhileStatement &) = 0;
  virtual void post_visit(ForStatement &) = 0;
  virtual void post_visit(PrintStatement &) = 0;
  virtual void post_visit(DeleteStatement &) = 0;
  virtual void post_visit(BreakStatement &) = 0;
  virtual void post_visit(ContinueStatement &) = 0;
};

/*
Template for new recursive visitors:

  void pre_visit(Program &) override {}
  void pre_visit(Procedure &) override {}
  void pre_visit(VariableLValueExpr &) override {}
  void pre_visit(DereferenceLValueExpr &) override {}
  void pre_visit(AssignmentExpr &) override {}
  void pre_visit(VariableExpr &) override {}
  void pre_visit(LiteralExpr &) override {}
  void pre_visit(BinaryExpr &) override {}
  void pre_visit(BooleanOrExpr &) override {}
  void pre_visit(BooleanAndExpr &) override {}
  void pre_visit(AddressOfExpr &) override {}
  void pre_visit(DereferenceExpr &) override {}
  void pre_visit(NewExpr &) override {}
  void pre_visit(FunctionCallExpr &) override {}
  void pre_visit(Statements &) override {}
  void pre_visit(ExprStatement &) override {}
  void pre_visit(AssignmentStatement &) override {}
  void pre_visit(IfStatement &) override {}
  void pre_visit(WhileStatement &) override {}
  void pre_visit(PrintStatement &) override {}
  void pre_visit(DeleteStatement &) override {}

  void post_visit(Program &) override {}
  void post_visit(Procedure &) override {}
  void post_visit(VariableLValueExpr &) override {}
  void post_visit(DereferenceLValueExpr &) override {}
  void post_visit(AssignmentExpr &) override {}
  void post_visit(VariableExpr &) override {}
  void post_visit(LiteralExpr &) override {}
  void post_visit(BinaryExpr &) override {}
  void post_visit(BooleanOrExpr &) override {}
  void post_visit(BooleanAndExpr &) override {}
  void post_visit(AddressOfExpr &) override {}
  void post_visit(DereferenceExpr &) override {}
  void post_visit(NewExpr &) override {}
  void post_visit(FunctionCallExpr &) override {}
  void post_visit(Statements &) override {}
  void post_visit(ExprStatement &) override {}
  void post_visit(AssignmentStatement &) override {}
  void post_visit(IfStatement &) override {}
  void post_visit(WhileStatement &) override {}
  void post_visit(PrintStatement &) override {}
  void post_visit(DeleteStatement &) override {}
*/
