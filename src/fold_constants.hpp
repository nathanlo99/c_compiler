#pragma once

#include "ast_visitor.hpp"
#include "lexer.hpp"
#include "util.hpp"
#include <memory>

struct Expr;
std::shared_ptr<Expr> fold_constants(std::shared_ptr<Expr> expr);

struct FoldConstantsVisitor : ASTVisitor {
  virtual ~FoldConstantsVisitor() = default;

  void pre_visit(Procedure &procedure) override;

  void pre_visit(TestExpr &expr) override;
  void pre_visit(BinaryExpr &) override;
  void pre_visit(NewExpr &) override;
  void pre_visit(FunctionCallExpr &) override;
  void pre_visit(DereferenceLValueExpr &) override;

  void pre_visit(AssignmentStatement &) override;
  void pre_visit(PrintStatement &) override;
  void pre_visit(DeleteStatement &) override;
};
