
#pragma once

#include "ast_recursive_visitor.hpp"
#include "types.hpp"

#include <map>
#include <memory>
#include <string>

#include "symbol_table.hpp"

struct PopulateSymbolTableVisitor : ASTRecursiveVisitor {
  SymbolTable table;

  virtual ~PopulateSymbolTableVisitor() = default;

  virtual void pre_visit(Procedure &procedure) override;
  virtual void pre_visit(VariableExpr &expr) override;
  virtual void pre_visit(VariableLValueExpr &expr) override;

  // Update the program's table once we're done
  virtual void post_visit(Procedure &procedure) override;
  virtual void post_visit(Program &program) override;
};
