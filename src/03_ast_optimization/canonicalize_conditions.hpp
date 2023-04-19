
#pragma once

#include "ast_recursive_visitor.hpp"

struct CanonicalizeConditions : ASTRecursiveVisitor {
  using ASTRecursiveVisitor::post_visit;
  using ASTRecursiveVisitor::pre_visit;

  virtual void post_visit(IfStatement &statement) override;
};
