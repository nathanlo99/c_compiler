
#include "canonicalize_conditions.hpp"
#include "ast_node.hpp"

void CanonicalizeConditions::post_visit(IfStatement &statement) {
  auto &cond = statement.test_expression;
  switch (cond->operation) {
  case BinaryOperation::LessThan: {
    // Do nothing, this is already canonical
  } break;

  case BinaryOperation::LessEqual: {
    std::swap(cond->lhs, cond->rhs);
    std::swap(statement.true_statements, statement.false_statements);
    cond->operation = BinaryOperation::LessThan;
  } break;

  case BinaryOperation::GreaterThan: {
    std::swap(cond->lhs, cond->rhs);
    cond->operation = BinaryOperation::LessThan;
  } break;

  case BinaryOperation::GreaterEqual: {
    std::swap(statement.true_statements, statement.false_statements);
    cond->operation = BinaryOperation::LessThan;
  } break;

  case BinaryOperation::Equal: {
    // Do nothing, this is already canonical
  } break;

  case BinaryOperation::NotEqual: {
    std::swap(statement.true_statements, statement.false_statements);
    cond->operation = BinaryOperation::Equal;
  } break;

  default: {
    debug_assert(false,
                 "If statement contained unexpected binary operation: expected "
                 "boolean operation but got {}",
                 binary_operation_to_string(cond->operation));
  } break;
  }
}
