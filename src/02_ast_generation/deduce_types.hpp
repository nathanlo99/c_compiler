
#pragma once

#include "ast_node.hpp"
#include "ast_recursive_visitor.hpp"
#include "symbol_table.hpp"
#include "types.hpp"
#include <memory>

struct DeduceTypesVisitor : ASTRecursiveVisitor {
  using ASTRecursiveVisitor::post_visit;
  using ASTRecursiveVisitor::pre_visit;

  std::optional<SymbolTable> table;

  DeduceTypesVisitor() = default;
  DeduceTypesVisitor(const SymbolTable &table) : table(table) {}

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
  void post_visit(ForStatement &) override;
  void post_visit(AssignmentStatement &) override;
  void post_visit(PrintStatement &) override;
  void post_visit(DeleteStatement &) override;
  void post_visit(ReturnStatement &) override;
};
