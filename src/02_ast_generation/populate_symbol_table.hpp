
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
};
