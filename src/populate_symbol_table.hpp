
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
};
