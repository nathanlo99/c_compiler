
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
struct ReturnStatement;

struct ASTSimpleVisitor {
  virtual ~ASTSimpleVisitor() = default;

  virtual void visit(Program &) {}
  virtual void visit(Procedure &) {}
  virtual void visit(VariableLValueExpr &) {}
  virtual void visit(DereferenceLValueExpr &) {}
  virtual void visit(AssignmentExpr &) {}
  virtual void visit(VariableExpr &) {}
  virtual void visit(LiteralExpr &) {}
  virtual void visit(BinaryExpr &) {}
  virtual void visit(BooleanOrExpr &) {}
  virtual void visit(BooleanAndExpr &) {}
  virtual void visit(AddressOfExpr &) {}
  virtual void visit(DereferenceExpr &) {}
  virtual void visit(NewExpr &) {}
  virtual void visit(FunctionCallExpr &) {}
  virtual void visit(Statements &) {}
  virtual void visit(ExprStatement &) {}
  virtual void visit(AssignmentStatement &) {}
  virtual void visit(IfStatement &) {}
  virtual void visit(WhileStatement &) {}
  virtual void visit(ForStatement &) {}
  virtual void visit(PrintStatement &) {}
  virtual void visit(DeleteStatement &) {}
  virtual void visit(BreakStatement &) {}
  virtual void visit(ContinueStatement &) {}
  virtual void visit(ReturnStatement &) {}
};
