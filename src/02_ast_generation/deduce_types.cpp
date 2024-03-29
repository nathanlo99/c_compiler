
#include "deduce_types.hpp"
#include "ast_node.hpp"

void DeduceTypesVisitor::pre_visit(Program &program) { table = program.table; }

void DeduceTypesVisitor::pre_visit(Procedure &procedure) {
  table->enter_procedure(procedure.name);
}

void DeduceTypesVisitor::post_visit(Procedure &) { table->leave_procedure(); }

void DeduceTypesVisitor::post_visit(VariableLValueExpr &expr) {
  expr.type = expr.variable.type = table->get_variable_type(expr.variable);
}

void DeduceTypesVisitor::post_visit(DereferenceLValueExpr &expr) {
  debug_assert(expr.argument->type == Type::IntStar,
               "Dereference expected 'int*', got {}",
               type_to_string(expr.argument->type));
  expr.type = Type::Int;
}

void DeduceTypesVisitor::post_visit(AssignmentExpr &expr) {
  debug_assert(expr.lhs->type == expr.rhs->type,
               "Cannot assign arguments of different types");
  expr.type = expr.lhs->type;
}

void DeduceTypesVisitor::post_visit(VariableExpr &expr) {
  expr.type = expr.variable.type = table->get_variable_type(expr.variable);
}

void DeduceTypesVisitor::post_visit(LiteralExpr &expr) {
  expr.type = expr.literal.type;
}

void DeduceTypesVisitor::post_visit(BinaryExpr &expr) {
  const std::map<std::pair<Type, Type>, Type> plus_types = {
      std::make_pair(std::make_pair(Type::Int, Type::Int), Type::Int),
      std::make_pair(std::make_pair(Type::IntStar, Type::Int), Type::IntStar),
      std::make_pair(std::make_pair(Type::Int, Type::IntStar), Type::IntStar),
  };
  const std::map<std::pair<Type, Type>, Type> minus_types = {
      std::make_pair(std::make_pair(Type::Int, Type::Int), Type::Int),
      std::make_pair(std::make_pair(Type::IntStar, Type::Int), Type::IntStar),
      std::make_pair(std::make_pair(Type::IntStar, Type::IntStar), Type::Int),
  };
  const std::map<std::pair<Type, Type>, Type> integer_types = {
      std::make_pair(std::make_pair(Type::Int, Type::Int), Type::Int),
  };

  const auto lhs_type = expr.lhs->type;
  const auto rhs_type = expr.rhs->type;
  const auto type_pair = std::make_pair(lhs_type, rhs_type);
  debug_assert(lhs_type != Type::Unknown, "Type deduction failed on lhs");
  debug_assert(rhs_type != Type::Unknown, "Type deduction failed on rhs");
  switch (expr.operation) {
  case BinaryOperation::Add: {
    debug_assert(plus_types.count(type_pair) > 0, "Invalid types to +");
    expr.type = plus_types.at(type_pair);
  } break;
  case BinaryOperation::Sub: {
    debug_assert(minus_types.count(type_pair) > 0, "Invalid types to -");
    expr.type = minus_types.at(type_pair);
  } break;
  case BinaryOperation::Mul:
  case BinaryOperation::Div:
  case BinaryOperation::Mod: {
    debug_assert(integer_types.count(type_pair) > 0, "Invalid types to {}",
                 binary_operation_to_string(expr.operation));
    expr.type = integer_types.at(type_pair);
  } break;
  case BinaryOperation::Equal:
  case BinaryOperation::NotEqual:
  case BinaryOperation::LessThan:
  case BinaryOperation::LessEqual:
  case BinaryOperation::GreaterThan:
  case BinaryOperation::GreaterEqual: {
    debug_assert(expr.lhs->type == expr.rhs->type,
                 "Cannot compare arguments of different types");
    expr.type = Type::Int;
  } break;
  default:
    unreachable("");
  }
}

void DeduceTypesVisitor::post_visit(BooleanOrExpr &expr) {
  debug_assert(expr.lhs->type == Type::Int, "LHS of || must be int, got {}",
               type_to_string(expr.lhs->type));
  debug_assert(expr.rhs->type == Type::Int, "RHS of || must be int, got {}",
               type_to_string(expr.rhs->type));
  expr.type = Type::Int;
}

void DeduceTypesVisitor::post_visit(BooleanAndExpr &expr) {
  debug_assert(expr.lhs->type == Type::Int, "LHS of && must be int, got {}",
               type_to_string(expr.lhs->type));
  debug_assert(expr.rhs->type == Type::Int, "RHS of && must be int, got {}",
               type_to_string(expr.rhs->type));
  expr.type = Type::Int;
}

void DeduceTypesVisitor::post_visit(AddressOfExpr &expr) {
  debug_assert(expr.argument->type == Type::Int,
               "Can only take address of int");
  expr.type = Type::IntStar;
}

void DeduceTypesVisitor::post_visit(DereferenceExpr &expr) {
  debug_assert(expr.argument->type == Type::IntStar,
               "Dereference expected 'int*', got {}",
               type_to_string(expr.argument->type));
  expr.type = Type::Int;
}

void DeduceTypesVisitor::post_visit(NewExpr &expr) {
  debug_assert(expr.rhs->type == Type::Int, "Argument to new[] must be int");
  expr.type = Type::IntStar;
}

void DeduceTypesVisitor::post_visit(FunctionCallExpr &expr) {
  const auto procedure_name = expr.procedure_name;
  const std::vector<Variable> expected_arguments =
      table->get_arguments(procedure_name);

  std::vector<Type> argument_types;
  debug_assert(expr.arguments.size() == expected_arguments.size(),
               "Wrong number of arguments to call to {}: expected {}, got {}",
               procedure_name, expected_arguments.size(),
               expr.arguments.size());

  for (size_t i = 0; i < expected_arguments.size(); ++i) {
    debug_assert(
        expr.arguments[i]->type == expected_arguments[i].type,
        "The {}-th argument to {} had the wrong type: expected {}, got {}", i,
        procedure_name, type_to_string(expected_arguments[i].type),
        type_to_string(expr.arguments[i]->type));
  }

  expr.type = table->get_return_type(procedure_name);
}

void DeduceTypesVisitor::post_visit(IfStatement &statement) {
  debug_assert(statement.test_expression->type == Type::Int,
               "If condition expected int, got {}",
               type_to_string(statement.test_expression->type));
}

void DeduceTypesVisitor::post_visit(WhileStatement &statement) {
  debug_assert(statement.test_expression->type == Type::Int,
               "While condition expected int, got {}",
               type_to_string(statement.test_expression->type));
}

void DeduceTypesVisitor::post_visit(ForStatement &statement) {
  debug_assert(statement.test_expression->type == Type::Int,
               "For condition expected int, got {}",
               type_to_string(statement.test_expression->type));
}

void DeduceTypesVisitor::post_visit(AssignmentStatement &statement) {
  debug_assert(statement.lhs->type == statement.rhs->type,
               "Assignment rhs had the wrong type");
}

void DeduceTypesVisitor::post_visit(PrintStatement &statement) {
  debug_assert(statement.expression->type == Type::Int,
               "println expected int, got {}",
               type_to_string(statement.expression->type));
}

void DeduceTypesVisitor::post_visit(DeleteStatement &statement) {
  debug_assert(statement.expression->type == Type::IntStar,
               "delete expected int*, got {}",
               type_to_string(statement.expression->type));
}

void DeduceTypesVisitor::post_visit(ReturnStatement &statement) {
  const auto expected_return_type =
      table->get_return_type(table->current_procedure);
  debug_assert(statement.expr->type == expected_return_type,
               "Return type mismatch: expected {}, got {}",
               type_to_string(expected_return_type),
               type_to_string(statement.expr->type));
}
