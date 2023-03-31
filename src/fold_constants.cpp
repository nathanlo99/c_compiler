
#include "fold_constants.hpp"
#include "ast_node.hpp"
#include <memory>

bool is_literal(std::shared_ptr<Expr> expr) {
  if (std::dynamic_pointer_cast<LiteralExpr>(expr))
    return true;
  return false;
}

Literal evaluate_binary_expression(std::shared_ptr<Expr> lhs_expr,
                                   const Token operation,
                                   std::shared_ptr<Expr> rhs_expr) {
  const auto lhs = std::dynamic_pointer_cast<LiteralExpr>(lhs_expr);
  const auto rhs = std::dynamic_pointer_cast<LiteralExpr>(rhs_expr);
  const int lhs_value = lhs->literal.value;
  const int rhs_value = rhs->literal.value;
  const Type lhs_type = lhs->type;
  const Type rhs_type = rhs->type;
  switch (operation.kind) {
  case TokenKind::Plus: {
    if (lhs_type == Type::Int && rhs_type == Type::Int) {
      return Literal(lhs_value + rhs_value, Type::Int);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::Int) {
      return Literal(lhs_value + 4 * rhs_value, Type::IntStar);
    } else if (lhs_type == Type::Int && rhs_type == Type::IntStar) {
      return Literal(4 * lhs_value + rhs_value, Type::IntStar);
    }
  } break;
  case TokenKind::Minus: {
    if (lhs_type == Type::Int && rhs_type == Type::Int) {
      return Literal(lhs_value - rhs_value, Type::Int);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::Int) {
      return Literal(lhs_value - 4 * rhs_value, Type::IntStar);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::IntStar) {
      return Literal((lhs_value - rhs_value) / 4, Type::Int);
    }
  } break;
  case TokenKind::Star:
    return Literal(lhs_value * rhs_value, Type::Int);
  case TokenKind::Slash:
    return Literal(lhs_value / rhs_value, Type::Int);
  case TokenKind::Pct:
    return Literal(lhs_value % rhs_value, Type::Int);
  default:
    break;
  }
  __builtin_unreachable();
}

std::shared_ptr<Expr> fold_constants(std::shared_ptr<Expr> expr) {
  if (auto node = std::dynamic_pointer_cast<TestExpr>(expr)) {
    const auto lhs = fold_constants(node->lhs);
    const auto rhs = fold_constants(node->rhs);
    if (is_literal(lhs) && is_literal(rhs)) {
      const auto literal =
          evaluate_binary_expression(lhs, node->operation, rhs);
      return std::make_shared<LiteralExpr>(literal);
    } else {
      return std::make_shared<TestExpr>(lhs, node->operation, rhs);
    }
  } else if (auto node = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
    const auto lhs = fold_constants(node->lhs);
    const auto rhs = fold_constants(node->rhs);
    if (is_literal(lhs) && is_literal(rhs)) {
      const auto literal =
          evaluate_binary_expression(lhs, node->operation, rhs);
      return std::make_shared<LiteralExpr>(literal);
    } else {
      return std::make_shared<TestExpr>(lhs, node->operation, rhs);
    }
  } else if (auto node = std::dynamic_pointer_cast<NewExpr>(expr)) {
    return std::make_shared<NewExpr>(fold_constants(node->rhs));
  } else if (auto node = std::dynamic_pointer_cast<FunctionCallExpr>(expr)) {
    auto result = std::make_shared<FunctionCallExpr>(node->procedure_name);
    for (const auto &arg : node->arguments)
      result->arguments.push_back(fold_constants(arg));
    return result;
  } else {
    return expr;
  }
}

void FoldConstantsVisitor::pre_visit(Procedure &procedure) {
  procedure.return_expr = fold_constants(procedure.return_expr);
}

void FoldConstantsVisitor::pre_visit(DereferenceLValueExpr &expr) {
  expr.argument = fold_constants(expr.argument);
}

void FoldConstantsVisitor::pre_visit(TestExpr &expr) {
  expr.lhs = fold_constants(expr.lhs);
  expr.rhs = fold_constants(expr.rhs);
}

void FoldConstantsVisitor::pre_visit(BinaryExpr &expr) {
  expr.lhs = fold_constants(expr.lhs);
  expr.rhs = fold_constants(expr.rhs);
}

void FoldConstantsVisitor::pre_visit(NewExpr &expr) {
  expr.rhs = fold_constants(expr.rhs);
}

void FoldConstantsVisitor::pre_visit(FunctionCallExpr &expr) {
  for (auto &argument : expr.arguments) {
    argument = fold_constants(argument);
  }
}

void FoldConstantsVisitor::pre_visit(AssignmentStatement &statement) {
  statement.rhs = fold_constants(statement.rhs);
}

void FoldConstantsVisitor::pre_visit(PrintStatement &statement) {
  statement.expression = fold_constants(statement.expression);
}

void FoldConstantsVisitor::pre_visit(DeleteStatement &statement) {
  statement.expression = fold_constants(statement.expression);
}
