
#pragma once

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
  void post_visit(TestExpr &) override;
  void post_visit(VariableExpr &) override;
  void post_visit(LiteralExpr &) override;
  void post_visit(BinaryExpr &) override;
  void post_visit(AddressOfExpr &) override;
  void post_visit(DereferenceExpr &) override;
  void post_visit(NewExpr &) override;
  void post_visit(FunctionCallExpr &) override;

  void post_visit(AssignmentStatement &) override;
  void post_visit(PrintStatement &) override;
  void post_visit(DeleteStatement &) override;
};
