
#pragma once

#include "ast_recursive_visitor.hpp"

struct CanonicalizeConditions : ASTRecursiveVisitor {
  using ASTRecursiveVisitor::ASTRecursiveVisitor;

  virtual void post_visit(IfStatement &statement) override;
};
