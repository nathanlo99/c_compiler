
#include "canonicalize_conditions.hpp"
#include "ast_node.hpp"

void CanonicalizeConditions::post_visit(IfStatement &statement) {
  auto &cond = statement.test_expression;
  switch (cond->operation) {
  case ComparisonOperation::LessThan: {
    // Do nothing, this is already canonical
  } break;

  case ComparisonOperation::LessEqual: {
    std::swap(cond->lhs, cond->rhs);
    std::swap(statement.true_statements, statement.false_statements);
    cond->operation = ComparisonOperation::LessThan;
  } break;

  case ComparisonOperation::GreaterThan: {
    std::swap(cond->lhs, cond->rhs);
    cond->operation = ComparisonOperation::LessThan;
  } break;

  case ComparisonOperation::GreaterEqual: {
    std::swap(statement.true_statements, statement.false_statements);
    cond->operation = ComparisonOperation::LessThan;
  } break;

  case ComparisonOperation::Equal: {
    // Do nothing, this is already canonical
  } break;

  case ComparisonOperation::NotEqual: {
    std::swap(statement.true_statements, statement.false_statements);
    cond->operation = ComparisonOperation::Equal;
  } break;
  }
}
