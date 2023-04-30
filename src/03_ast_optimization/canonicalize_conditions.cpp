
#include "canonicalize_conditions.hpp"
#include "ast_node.hpp"

void CanonicalizeConditions::post_visit(IfStatement &statement) {
  auto &cond = statement.test_expression;
  switch (cond->operation) {
  case BooleanOperation::LessThan: {
    // Do nothing, this is already canonical
  } break;

  case BooleanOperation::LessEqual: {
    std::swap(cond->lhs, cond->rhs);
    std::swap(statement.true_statements, statement.false_statements);
    cond->operation = BooleanOperation::LessThan;
  } break;

  case BooleanOperation::GreaterThan: {
    std::swap(cond->lhs, cond->rhs);
    cond->operation = BooleanOperation::LessThan;
  } break;

  case BooleanOperation::GreaterEqual: {
    std::swap(statement.true_statements, statement.false_statements);
    cond->operation = BooleanOperation::LessThan;
  } break;

  case BooleanOperation::Equal: {
    // Do nothing, this is already canonical
  } break;

  case BooleanOperation::NotEqual: {
    std::swap(statement.true_statements, statement.false_statements);
    cond->operation = BooleanOperation::Equal;
  } break;
  }
}
