
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "util.hpp"

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
struct ForStatement;
struct PrintStatement;
struct DeleteStatement;
struct BreakStatement;
struct ContinueStatement;

struct ASTSimpleVisitor {
  virtual ~ASTSimpleVisitor() = default;

  virtual void visit(Program &) = 0;
  virtual void visit(Procedure &) = 0;
  virtual void visit(VariableLValueExpr &) = 0;
  virtual void visit(DereferenceLValueExpr &) = 0;
  virtual void visit(AssignmentExpr &) = 0;
  virtual void visit(VariableExpr &) = 0;
  virtual void visit(LiteralExpr &) = 0;
  virtual void visit(BinaryExpr &) = 0;
  virtual void visit(BooleanOrExpr &) = 0;
  virtual void visit(BooleanAndExpr &) = 0;
  virtual void visit(AddressOfExpr &) = 0;
  virtual void visit(DereferenceExpr &) = 0;
  virtual void visit(NewExpr &) = 0;
  virtual void visit(FunctionCallExpr &) = 0;
  virtual void visit(Statements &) = 0;
  virtual void visit(ExprStatement &) = 0;
  virtual void visit(AssignmentStatement &) = 0;
  virtual void visit(IfStatement &) = 0;
  virtual void visit(WhileStatement &) = 0;
  virtual void visit(ForStatement &) = 0;
  virtual void visit(PrintStatement &) = 0;
  virtual void visit(DeleteStatement &) = 0;
  virtual void visit(BreakStatement &) = 0;
  virtual void visit(ContinueStatement &) = 0;
};

/* Template:
  void visit(Program &) override {}
  void visit(Procedure &) override {}
  void visit(VariableLValueExpr &) override {}
  void visit(DereferenceLValueExpr &) override {}
  void visit(AssignmentExpr &) override {}
  void visit(VariableExpr &) override {}
  void visit(LiteralExpr &) override {}
  void visit(BinaryExpr &) override {}
  void visit(BooleanOrExpr &) override {}
  void visit(BooleanAndExpr &) override {}
  void visit(AddressOfExpr &) override {}
  void visit(DereferenceExpr &) override {}
  void visit(NewExpr &) override {}
  void visit(FunctionCallExpr &) override {}
  void visit(Statements &) override {}
  void visit(ExprStatement &) override {}
  void visit(AssignmentStatement &) override {}
  void visit(IfStatement &) override {}
  void visit(WhileStatement &) override {}
  void visit(ForStatement &) override {}
  void visit(PrintStatement &) override {}
  void visit(DeleteStatement &) override {}
  void visit(BreakStatement &) override {}
  void visit(ContinueStatement &) override {}
*/
