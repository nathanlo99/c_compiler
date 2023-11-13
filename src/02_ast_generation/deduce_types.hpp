
#pragma once

#include "ast_node.hpp"
#include "ast_recursive_visitor.hpp"
#include "symbol_table.hpp"
#include "types.hpp"
#include <memory>

struct DeduceTypesVisitor : ASTRecursiveVisitor {
  using ASTRecursiveVisitor::post_visit;
  using ASTRecursiveVisitor::pre_visit;

  SymbolTable table;
  bool has_table;

  DeduceTypesVisitor() : table(), has_table(false) {}
  DeduceTypesVisitor(const SymbolTable &table)
      : table(table), has_table(true) {}

  ~DeduceTypesVisitor() = default;

  void pre_visit(Program &program) override;
  void pre_visit(Procedure &procedure) override;

  void post_visit(Procedure &procedure) override;

  void post_visit(VariableLValueExpr &) override;
  void post_visit(DereferenceLValueExpr &) override;
  void post_visit(AssignmentExpr &) override;
  void post_visit(VariableExpr &) override;
  void post_visit(LiteralExpr &) override;
  void post_visit(BinaryExpr &) override;
  void post_visit(BooleanOrExpr &) override;
  void post_visit(BooleanAndExpr &) override;
  void post_visit(AddressOfExpr &) override;
  void post_visit(DereferenceExpr &) override;
  void post_visit(NewExpr &) override;
  void post_visit(FunctionCallExpr &) override;

  void post_visit(IfStatement &) override;
  void post_visit(WhileStatement &) override;
  void post_visit(AssignmentStatement &) override;
  void post_visit(PrintStatement &) override;
  void post_visit(DeleteStatement &) override;

  // The rest don't need any work done: simply recurse on the children
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
  void post_visit(Statements &) override {}
  void post_visit(ExprStatement &) override {}
};
