
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
    } else {
      std::cout << "?" << std::endl;
    }
  } break;
  case TokenKind::Minus: {
    if (lhs_type == Type::Int && rhs_type == Type::Int) {
      return Literal(lhs_value - rhs_value, Type::Int);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::Int) {
      return Literal(lhs_value - 4 * rhs_value, Type::IntStar);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::IntStar) {
      return Literal((lhs_value - rhs_value) / 4, Type::Int);
    } else {
      std::cout << "??" << std::endl;
    }
  } break;
  case TokenKind::Star:
    return Literal(lhs_value * rhs_value, Type::Int);
  case TokenKind::Slash:
    runtime_assert(rhs_value != 0, "Division by zero while constant folding");
    return Literal(lhs_value / rhs_value, Type::Int);
  case TokenKind::Pct:
    runtime_assert(rhs_value != 0, "Modulo by zero while constant folding");
    return Literal(lhs_value % rhs_value, Type::Int);
  default:
    std::cout << "Unknown token type " << operation.lexeme << std::endl;
    break;
  }
  __builtin_unreachable();
}

std::shared_ptr<Expr>
simplify_binary_expression(std::shared_ptr<BinaryExpr> expr) {
  const auto lhs = expr->lhs;
  const auto rhs = expr->rhs;
  const auto operation = expr->operation.kind;
  if (is_literal(lhs) && is_literal(rhs)) {
    const auto literal = evaluate_binary_expression(lhs, expr->operation, rhs);
    return std::make_shared<LiteralExpr>(literal);
  }
  if (const auto lhs_literal = std::dynamic_pointer_cast<LiteralExpr>(lhs)) {
    const int value = lhs_literal->literal.value;
    // 0 + rhs == rhs
    if (value == 0 && operation == TokenKind::Plus)
      return rhs;
    // 0 * rhs == 0
    if (value == 0 && operation == TokenKind::Star)
      return std::make_shared<LiteralExpr>(0, Type::Int);
    // 1 * rhs == rhs
    if (value == 1 && operation == TokenKind::Star)
      return rhs;
    // 0 / rhs == 0
    if (value == 0 && operation == TokenKind::Slash)
      return std::make_shared<LiteralExpr>(0, Type::Int);
    // 0 % rhs == 0
    if (value == 0 && operation == TokenKind::Pct)
      return std::make_shared<LiteralExpr>(0, Type::Int);
  }

  if (const auto rhs_literal = std::dynamic_pointer_cast<LiteralExpr>(rhs)) {
    const int value = rhs_literal->literal.value;
    // lhs + 0 == lhs
    if (value == 0 && operation == TokenKind::Plus)
      return lhs;
    // lhs - 0 == lhs
    if (value == 0 && operation == TokenKind::Minus)
      return lhs;
    // lhs * 0 == 0
    if (value == 0 && operation == TokenKind::Star)
      return std::make_shared<LiteralExpr>(0, Type::Int);
    // lhs * 1 == lhs
    if (value == 1 && operation == TokenKind::Star)
      return lhs;
    // lhs / 1 == lhs
    if (value == 1 && operation == TokenKind::Slash)
      return lhs;
    // lhs / 0 == ERROR
    if (value == 0 && operation == TokenKind::Slash)
      runtime_assert(false, "Division by zero");
    // lhs % 1 == 0
    if (value == 1 && operation == TokenKind::Pct)
      return std::make_shared<LiteralExpr>(0, Type::Int);
    // lhs % 0 == ERROR
    if (value == 0 && operation == TokenKind::Pct)
      runtime_assert(false, "Modulo by zero");
  }

  return expr;
}

std::shared_ptr<Expr> fold_constants(std::shared_ptr<Expr> expr) {
  if (auto node = std::dynamic_pointer_cast<TestExpr>(expr)) {
    node->lhs = fold_constants(node->lhs);
    node->rhs = fold_constants(node->rhs);
    if (is_literal(node->lhs) && is_literal(node->rhs)) {
      const auto literal =
          evaluate_binary_expression(node->lhs, node->operation, node->rhs);
      return std::make_shared<LiteralExpr>(literal);
    }
    return node;
  } else if (auto node = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
    node->lhs = fold_constants(node->lhs);
    node->rhs = fold_constants(node->rhs);
    return simplify_binary_expression(node);
  } else if (auto node = std::dynamic_pointer_cast<NewExpr>(expr)) {
    return std::make_shared<NewExpr>(fold_constants(node->rhs));
  } else if (auto node = std::dynamic_pointer_cast<FunctionCallExpr>(expr)) {
    for (auto &arg : node->arguments)
      arg = fold_constants(arg);
    return node;
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
