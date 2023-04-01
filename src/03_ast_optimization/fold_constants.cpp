
#include "fold_constants.hpp"
#include "ast_node.hpp"
#include "util.hpp"
#include <memory>

bool is_literal(std::shared_ptr<Expr> expr) {
  if (std::dynamic_pointer_cast<LiteralExpr>(expr))
    return true;
  return false;
}

Literal evaluate_binary_expression(std::shared_ptr<LiteralExpr> lhs,
                                   const BinaryOperation operation,
                                   std::shared_ptr<LiteralExpr> rhs) {
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
    runtime_assert(rhs_value != 0, "Division by zero while constant folding");
    return Literal(lhs_value / rhs_value, Type::Int);
  case BinaryOperation::Mod:
    runtime_assert(rhs_value != 0, "Modulo by zero while constant folding");
    return Literal(lhs_value % rhs_value, Type::Int);
  default:
    std::cout << "Unknown token type " << binary_operation_to_string(operation)
              << std::endl;
    break;
  }
  __builtin_unreachable();
}

struct Sum {
  struct SumTerm {
    std::shared_ptr<Expr> value;
    bool is_negative = false;

    SumTerm(std::shared_ptr<Expr> value, bool negative)
        : value(value), is_negative(negative) {}
    static SumTerm positive(std::shared_ptr<Expr> value) {
      return SumTerm(value, false);
    }
    static SumTerm negative(std::shared_ptr<Expr> value) {
      return SumTerm(value, true);
    }
  };

  std::vector<SumTerm> terms;

  void gather_terms(std::shared_ptr<Expr> expr, bool is_negative = false) {
    if (auto node = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
      switch (node->operation) {
      case BinaryOperation::Add:
        gather_terms(node->lhs, is_negative);
        gather_terms(node->rhs, is_negative);
        break;
      case BinaryOperation::Sub:
        gather_terms(node->lhs, is_negative);
        gather_terms(node->rhs, !is_negative);
        break;
      default:
        terms.emplace_back(fold_constants(expr), is_negative);
        break;
      }
    } else {
      terms.emplace_back(fold_constants(expr), is_negative);
    }
  }

  std::shared_ptr<Expr> combine_terms() const {
    // Chosen because it's the additive identity for both int and int*
    std::shared_ptr<LiteralExpr> constant =
        std::make_shared<LiteralExpr>(0, Type::Int);
    std::shared_ptr<Expr> expr = nullptr;
    for (const auto &term : terms) {
      if (auto term_literal =
              std::dynamic_pointer_cast<LiteralExpr>(term.value)) {
        if (term_literal->literal.value == 0)
          continue;
      }
      const BinaryOperation operation =
          term.is_negative ? BinaryOperation::Sub : BinaryOperation::Add;
      if (auto node = std::dynamic_pointer_cast<LiteralExpr>(term.value)) {
        constant = std::make_shared<LiteralExpr>(
            evaluate_binary_expression(constant, operation, node));
      } else {
        if (expr == nullptr) {
          // TODO: Deal with negative pointers
          expr = term.value;
        } else {
          expr = std::make_shared<BinaryExpr>(expr, operation, term.value);
        }
      }
    }
    if (constant->literal.value == 0) {
      if (expr == nullptr)
        return std::make_shared<LiteralExpr>(0, constant->literal.type);
      return expr;
    } else {
      return std::make_shared<BinaryExpr>(expr, BinaryOperation::Add, constant);
    }
  }

  friend std::ostream &operator<<(std::ostream &os, const Sum &sum) {
    os << "Sum {" << std::endl;
    for (const auto &term : sum.terms) {
      os << "  " << (term.is_negative ? "-" : "+");
      term.value->print(0);
    }
    os << "}" << std::endl;
    return os;
  }
};

bool is_cancellable(std::shared_ptr<Expr> lhs, std::shared_ptr<Expr> rhs) {
  // If lhs and rhs are variables with the same name, then they are cancellable
  if (auto lhs_node = std::dynamic_pointer_cast<VariableExpr>(lhs)) {
    if (auto rhs_node = std::dynamic_pointer_cast<VariableExpr>(rhs)) {
      if (lhs_node->variable == rhs_node->variable)
        return true;
    }
  }
  return false;
}

std::shared_ptr<Expr>
simplify_binary_expression(std::shared_ptr<BinaryExpr> expr) {
  const auto lhs = expr->lhs = fold_constants(expr->lhs);
  const auto rhs = expr->rhs = fold_constants(expr->rhs);
  const auto operation = expr->operation;
  if (auto lhs_literal = std::dynamic_pointer_cast<LiteralExpr>(lhs)) {
    if (auto rhs_literal = std::dynamic_pointer_cast<LiteralExpr>(rhs)) {
      const auto literal =
          evaluate_binary_expression(lhs_literal, expr->operation, rhs_literal);
      return std::make_shared<LiteralExpr>(literal);
    }
  }

  // Sum sum;
  // sum.gather_terms(expr);
  // const auto combined_terms = sum.combine_terms();

  // std::cout << sum << std::endl;
  // combined_terms->emit_c(std::cout, 0);
  // std::cout << std::endl;

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
  if (operation == BinaryOperation::Sub && is_cancellable(lhs, rhs)) {
    return std::make_shared<LiteralExpr>(0, Type::Int);
  }

  return expr;
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
