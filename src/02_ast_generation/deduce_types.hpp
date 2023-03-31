
#pragma once

#include "ast_visitor.hpp"
#include "symbol_table.hpp"
#include "types.hpp"
#include <memory>

struct DeduceTypesVisitor : ASTVisitor {
  SymbolTable table;

  virtual ~DeduceTypesVisitor() = default;

  virtual void pre_visit(Program &program);
  virtual void pre_visit(Procedure &procedure);
  virtual void post_visit(Procedure &procedure);

  virtual void post_visit(VariableLValueExpr &);
  virtual void post_visit(DereferenceLValueExpr &);
  virtual void post_visit(TestExpr &);
  virtual void post_visit(VariableExpr &);
  virtual void post_visit(LiteralExpr &);
  virtual void post_visit(BinaryExpr &);
  virtual void post_visit(AddressOfExpr &);
  virtual void post_visit(NewExpr &);
  virtual void post_visit(FunctionCallExpr &);

  virtual void post_visit(AssignmentStatement &);
  virtual void post_visit(PrintStatement &);
  virtual void post_visit(DeleteStatement &);
};
