
#pragma once

#include "ast_visitor.hpp"
#include "types.hpp"

#include <map>
#include <memory>
#include <string>

#include "symbol_table.hpp"

struct PopulateSymbolTableVisitor : ASTVisitor {
  SymbolTable table;

  virtual ~PopulateSymbolTableVisitor() = default;

  virtual void pre_visit(Procedure &procedure);
  virtual void pre_visit(VariableExpr &expr);
  virtual void pre_visit(VariableLValueExpr &expr);

  // Update the program's table once we're done
  virtual void post_visit(Program &program);
};
