
#pragma once

#include "ast_recursive_visitor.hpp"
#include "symbol_table.hpp"
#include "types.hpp"
#include <memory>

struct DeduceTypesVisitor : ASTRecursiveVisitor {
  SymbolTable table;
  bool has_table;

  DeduceTypesVisitor() : table(), has_table(false) {}
  DeduceTypesVisitor(const SymbolTable &table)
      : table(table), has_table(true) {}

  virtual ~DeduceTypesVisitor() = default;

  virtual void pre_visit(Program &program) override;
  virtual void pre_visit(Procedure &procedure) override;
  virtual void post_visit(Procedure &procedure) override;

  virtual void post_visit(VariableLValueExpr &) override;
  virtual void post_visit(DereferenceLValueExpr &) override;
  virtual void post_visit(TestExpr &) override;
  virtual void post_visit(VariableExpr &) override;
  virtual void post_visit(LiteralExpr &) override;
  virtual void post_visit(BinaryExpr &) override;
  virtual void post_visit(AddressOfExpr &) override;
  virtual void post_visit(NewExpr &) override;
  virtual void post_visit(FunctionCallExpr &) override;

  virtual void post_visit(AssignmentStatement &) override;
  virtual void post_visit(PrintStatement &) override;
  virtual void post_visit(DeleteStatement &) override;
};
