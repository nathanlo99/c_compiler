
#include "fold_constants.hpp"
#include "ast_node.hpp"
#include "util.hpp"
#include <memory>

bool is_literal(const std::shared_ptr<Expr> &expr) {
  if (std::dynamic_pointer_cast<LiteralExpr>(expr))
    return true;
  return false;
}

std::optional<Literal>
evaluate_binary_expression(std::shared_ptr<LiteralExpr> &lhs,
                           const BinaryOperation operation,
                           std::shared_ptr<LiteralExpr> &rhs) {
  const int lhs_value = lhs->literal.value;
  const int rhs_value = rhs->literal.value;
  const Type lhs_type = lhs->type;
  const Type rhs_type = rhs->type;
  switch (operation) {
  case BinaryOperation::Add: {
    if (lhs_type == Type::Int && rhs_type == Type::Int) {
      return Literal(lhs_value + rhs_value, Type::Int);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::Int) {
      return Literal(lhs_value + 4 * rhs_value, Type::IntStar);
    } else if (lhs_type == Type::Int && rhs_type == Type::IntStar) {
      return Literal(4 * lhs_value + rhs_value, Type::IntStar);
    }
  } break;
  case BinaryOperation::Sub: {
    if (lhs_type == Type::Int && rhs_type == Type::Int) {
      return Literal(lhs_value - rhs_value, Type::Int);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::Int) {
      return Literal(lhs_value - 4 * rhs_value, Type::IntStar);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::IntStar) {
      return Literal((lhs_value - rhs_value) / 4, Type::Int);
    }
  } break;
  case BinaryOperation::Mul:
    return Literal(lhs_value * rhs_value, Type::Int);
  case BinaryOperation::Div:
    if (rhs_value == 0)
      return std::nullopt;
    return Literal(lhs_value / rhs_value, Type::Int);
  case BinaryOperation::Mod:
    if (rhs_value == 0)
      return std::nullopt;
    return Literal(lhs_value % rhs_value, Type::Int);
  }
  return std::nullopt;
}

std::shared_ptr<Expr> cancel(std::shared_ptr<BinaryExpr> expr) {
  const BinaryOperation operation = expr->operation;

  // If lhs and rhs are variables with the same name, then they are cancellable
  if (auto lhs_node = std::dynamic_pointer_cast<VariableExpr>(expr->lhs)) {
    if (auto rhs_node = std::dynamic_pointer_cast<VariableExpr>(expr->rhs)) {
      if (operation == BinaryOperation::Sub &&
          lhs_node->variable == rhs_node->variable)
        return std::make_shared<LiteralExpr>(0, Type::Int);
      if (operation == BinaryOperation::Div &&
          lhs_node->variable == rhs_node->variable)
        return std::make_shared<LiteralExpr>(1, Type::Int);
      if (operation == BinaryOperation::Mod &&
          lhs_node->variable == rhs_node->variable)
        return std::make_shared<LiteralExpr>(0, Type::Int);
    }
  }
  return expr;
}

std::shared_ptr<Expr>
simplify_binary_expression(std::shared_ptr<BinaryExpr> expr) {
  auto lhs = expr->lhs = fold_constants(expr->lhs);
  auto rhs = expr->rhs = fold_constants(expr->rhs);
  const auto operation = expr->operation;
  if (auto lhs_literal = std::dynamic_pointer_cast<LiteralExpr>(lhs)) {
    if (auto rhs_literal = std::dynamic_pointer_cast<LiteralExpr>(rhs)) {
      const auto literal =
          evaluate_binary_expression(lhs_literal, expr->operation, rhs_literal);
      if (literal.has_value())
        return std::make_shared<LiteralExpr>(literal.value());
      return expr;
    }
  }

  if (const auto lhs_literal = std::dynamic_pointer_cast<LiteralExpr>(lhs)) {
    const int value = lhs_literal->literal.value;
    // 0 + rhs == rhs
    if (value == 0 && operation == BinaryOperation::Add)
      return rhs;
    // 0 * rhs == 0
    if (value == 0 && operation == BinaryOperation::Mul)
      return std::make_shared<LiteralExpr>(0, Type::Int);
    // 1 * rhs == rhs
    if (value == 1 && operation == BinaryOperation::Mul)
      return rhs;
    // 0 / rhs == 0
    if (value == 0 && operation == BinaryOperation::Div)
      return std::make_shared<LiteralExpr>(0, Type::Int);
    // 0 % rhs == 0
    if (value == 0 && operation == BinaryOperation::Mod)
      return std::make_shared<LiteralExpr>(0, Type::Int);
  }

  if (const auto rhs_literal = std::dynamic_pointer_cast<LiteralExpr>(rhs)) {
    const int value = rhs_literal->literal.value;
    // lhs + 0 == lhs
    if (value == 0 && operation == BinaryOperation::Add)
      return lhs;
    // lhs - 0 == lhs
    if (value == 0 && operation == BinaryOperation::Sub)
      return lhs;
    // lhs * 0 == 0
    if (value == 0 && operation == BinaryOperation::Mul)
      return std::make_shared<LiteralExpr>(0, Type::Int);
    // lhs * 1 == lhs
    if (value == 1 && operation == BinaryOperation::Mul)
      return lhs;
    // lhs / 1 == lhs
    if (value == 1 && operation == BinaryOperation::Div)
      return lhs;
    // lhs / 0 == ERROR
    if (value == 0 && operation == BinaryOperation::Div)
      runtime_assert(false, "Division by zero");
    // lhs % 1 == 0
    if (value == 1 && operation == BinaryOperation::Mod)
      return std::make_shared<LiteralExpr>(0, Type::Int);
    // lhs % 0 == ERROR
    if (value == 0 && operation == BinaryOperation::Mod)
      runtime_assert(false, "Modulo by zero");
  }

  // If neither side is a literal, but they're still cancellable, then cancel
  // them
  return cancel(expr);
}

std::shared_ptr<Expr> fold_constants(std::shared_ptr<Expr> expr) {
  if (auto node = std::dynamic_pointer_cast<TestExpr>(expr)) {
    node->lhs = fold_constants(node->lhs);
    node->rhs = fold_constants(node->rhs);
    // if (is_literal(node->lhs) && is_literal(node->rhs)) {
    //   const auto literal = evaluate_comparison_expression(node->lhs,
    //   node->operation, node->rhs);
    // return std::make_shared<LiteralExpr>(literal);
    // }
    return node;
  } else if (auto node = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
    return simplify_binary_expression(node);
  } else if (auto node = std::dynamic_pointer_cast<NewExpr>(expr)) {
    node->rhs = fold_constants(node->rhs);
    return node;
  } else if (auto node = std::dynamic_pointer_cast<FunctionCallExpr>(expr)) {
    for (auto &arg : node->arguments)
      arg = fold_constants(arg);
    return node;
  } else {
    return expr;
  }
}

void ConstantFoldingVisitor::pre_visit(Procedure &procedure) {
  procedure.return_expr = fold_constants(procedure.return_expr);
}

void ConstantFoldingVisitor::pre_visit(DereferenceLValueExpr &expr) {
  expr.argument = fold_constants(expr.argument);
}

void ConstantFoldingVisitor::pre_visit(AssignmentExpr &expr) {
  expr.rhs = fold_constants(expr.rhs);
}

void ConstantFoldingVisitor::pre_visit(TestExpr &expr) {
  expr.lhs = fold_constants(expr.lhs);
  expr.rhs = fold_constants(expr.rhs);
}

void ConstantFoldingVisitor::pre_visit(BinaryExpr &expr) {
  expr.lhs = fold_constants(expr.lhs);
  expr.rhs = fold_constants(expr.rhs);
}

void ConstantFoldingVisitor::pre_visit(NewExpr &expr) {
  expr.rhs = fold_constants(expr.rhs);
}

void ConstantFoldingVisitor::pre_visit(DereferenceExpr &expr) {
  expr.argument = fold_constants(expr.argument);
}

void ConstantFoldingVisitor::pre_visit(FunctionCallExpr &expr) {
  for (auto &argument : expr.arguments) {
    argument = fold_constants(argument);
  }
}

void ConstantFoldingVisitor::pre_visit(ExprStatement &statement) {
  statement.expr = fold_constants(statement.expr);
}

void ConstantFoldingVisitor::pre_visit(AssignmentStatement &statement) {
  statement.rhs = fold_constants(statement.rhs);
}

void ConstantFoldingVisitor::pre_visit(PrintStatement &statement) {
  statement.expression = fold_constants(statement.expression);
}

void ConstantFoldingVisitor::pre_visit(DeleteStatement &statement) {
  statement.expression = fold_constants(statement.expression);
}
