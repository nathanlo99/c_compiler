
#pragma once

#include "ast_recursive_visitor.hpp"
#include "ast_simple_visitor.hpp"
#include "types.hpp"

#include <map>
#include <memory>
#include <string>

#include "symbol_table.hpp"

struct PopulateSymbolTableVisitor : ASTRecursiveVisitor {
  using ASTRecursiveVisitor::post_visit;
  using ASTRecursiveVisitor::pre_visit;

  SymbolTable table;

  ~PopulateSymbolTableVisitor() = default;

  void pre_visit(Procedure &procedure) override;
  void pre_visit(VariableExpr &expr) override;
  void pre_visit(VariableLValueExpr &expr) override;

  // Update the program's table once we're done
  void post_visit(Procedure &procedure) override;
  void post_visit(Program &program) override;

  // Update import flags
  void pre_visit(PrintStatement &) override;
  void pre_visit(DeleteStatement &) override;
  void pre_visit(NewExpr &) override;

  // The rest don't need any work done: simply recurse on the children
  void pre_visit(Program &) override {}
  void pre_visit(DereferenceLValueExpr &) override {}
  void pre_visit(AssignmentExpr &) override {}
  void pre_visit(LiteralExpr &) override {}
  void pre_visit(BinaryExpr &) override {}
  void pre_visit(BooleanOrExpr &) override {}
  void pre_visit(BooleanAndExpr &) override {}
  void pre_visit(AddressOfExpr &) override {}
  void pre_visit(DereferenceExpr &) override {}
  void pre_visit(FunctionCallExpr &) override {}
  void pre_visit(Statements &) override {}
  void pre_visit(ExprStatement &) override {}
  void pre_visit(AssignmentStatement &) override {}
  void pre_visit(IfStatement &) override {}
  void pre_visit(WhileStatement &) override {}
  void pre_visit(ForStatement &) override {}
  void pre_visit(BreakStatement &) override {}
  void pre_visit(ContinueStatement &) override {}

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
  void post_visit(ForStatement &) override {}
  void post_visit(PrintStatement &) override {}
  void post_visit(DeleteStatement &) override {}
  void post_visit(BreakStatement &) override {}
  void post_visit(ContinueStatement &) override {}
};
