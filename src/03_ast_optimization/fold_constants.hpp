#pragma once

#include "ast_recursive_visitor.hpp"
#include <memory>

struct Expr;
std::shared_ptr<Expr> fold_constants(std::shared_ptr<Expr> expr);

struct ConstantFoldingVisitor : ASTRecursiveVisitor {
  using ASTRecursiveVisitor::post_visit;
  using ASTRecursiveVisitor::pre_visit;

  ~ConstantFoldingVisitor() = default;

  void pre_visit(Procedure &) override;

  void pre_visit(TestExpr &) override;
  void pre_visit(BinaryExpr &) override;
  void pre_visit(NewExpr &) override;
  void pre_visit(DereferenceExpr &) override;
  void pre_visit(FunctionCallExpr &) override;
  void pre_visit(DereferenceLValueExpr &) override;

  void pre_visit(AssignmentStatement &) override;
  void pre_visit(PrintStatement &) override;
  void pre_visit(DeleteStatement &) override;
};
