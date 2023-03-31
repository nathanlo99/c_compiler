
#include "deduce_types.hpp"
#include "ast_node.hpp"

void DeduceTypesVisitor::pre_visit(Program &program) { table = program.table; }

void DeduceTypesVisitor::pre_visit(Procedure &procedure) {
  table.enter_procedure(procedure.name);
}
void DeduceTypesVisitor::post_visit(Procedure &procedure) {
  runtime_assert(procedure.return_expr->type == procedure.return_type,
                 "Unexpected return type for prodecure " + procedure.name +
                     ": expected int, got " +
                     type_to_string(procedure.return_expr->type));
  table.leave_procedure();
}

void DeduceTypesVisitor::post_visit(VariableLValueExpr &expr) {
  expr.type = expr.variable.type = table.get_variable_type(expr.variable);
}

void DeduceTypesVisitor::post_visit(DereferenceLValueExpr &expr) {
  runtime_assert(expr.argument->type == Type::IntStar,
                 "Dereference expected 'int*', got " +
                     type_to_string(expr.argument->type));
  expr.type = Type::Int;
}

void DeduceTypesVisitor::post_visit(TestExpr &expr) {
  runtime_assert(expr.lhs->type == expr.rhs->type,
                 "Cannot compare arguments of different types");
  expr.type = Type::Int;
}

void DeduceTypesVisitor::post_visit(VariableExpr &expr) {
  expr.type = expr.variable.type = table.get_variable_type(expr.variable);
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
      std::make_pair(std::make_pair(Type::Int, Type::IntStar), Type::IntStar),
  };
  const std::map<std::pair<Type, Type>, Type> integer_types = {
      std::make_pair(std::make_pair(Type::Int, Type::Int), Type::Int),
  };

  const auto lhs_type = expr.lhs->type;
  const auto rhs_type = expr.rhs->type;
  const auto type_pair = std::make_pair(lhs_type, rhs_type);
  runtime_assert(lhs_type != Type::Unknown, "Type deduction failed on lhs");
  runtime_assert(rhs_type != Type::Unknown, "Type deduction failed on rhs");
  switch (expr.operation) {
  case BinaryOperation::Add: {
    runtime_assert(plus_types.count(type_pair) > 0, "Invalid types to +");
    expr.type = plus_types.at(type_pair);
  } break;
  case BinaryOperation::Sub: {
    runtime_assert(minus_types.count(type_pair) > 0, "Invalid types to -");
    expr.type = minus_types.at(type_pair);
  } break;
  case BinaryOperation::Mul:
  case BinaryOperation::Div:
  case BinaryOperation::Mod: {
    runtime_assert(integer_types.count(type_pair) > 0,
                   std::string("Invalid types to ") +
                       binary_operation_to_string(expr.operation));
    expr.type = integer_types.at(type_pair);
  } break;
  default:
    unreachable("");
  }
}

void DeduceTypesVisitor::post_visit(AddressOfExpr &expr) {
  runtime_assert(expr.argument->type == Type::Int,
                 "Can only take address of int");
  expr.type = Type::IntStar;
}

void DeduceTypesVisitor::post_visit(NewExpr &expr) {
  runtime_assert(expr.rhs->type == Type::Int, "Argument to new must be int");
  expr.type = Type::IntStar;
}

void DeduceTypesVisitor::post_visit(FunctionCallExpr &expr) {
  const auto procedure_name = expr.procedure_name;
  const std::vector<Variable> expected_arguments =
      table.arguments.at(procedure_name);

  std::vector<Type> argument_types;
  runtime_assert(expr.arguments.size() == expected_arguments.size(),
                 "Wrong number of arguments to function call to " +
                     procedure_name + ": expected " +
                     std::to_string(expected_arguments.size()) + " but got " +
                     std::to_string(expr.arguments.size()));

  for (size_t i = 0; i < expected_arguments.size(); ++i) {
    runtime_assert(expr.arguments[i]->type == expected_arguments[i].type,
                   "The " + std::to_string(i) + "th argument to " +
                       procedure_name + " had the wrong type: expected " +
                       type_to_string(expected_arguments[i].type) + ", got " +
                       type_to_string(expr.arguments[i]->type));
  }

  expr.type = table.get_return_type(procedure_name);
}

void DeduceTypesVisitor::post_visit(AssignmentStatement &statement) {
  runtime_assert(statement.lhs->type == statement.rhs->type,
                 "Assignment rhs had the wrong type");
}

void DeduceTypesVisitor::post_visit(PrintStatement &statement) {
  runtime_assert(statement.expression->type == Type::Int,
                 "println expected int, got " +
                     type_to_string(statement.expression->type));
}

void DeduceTypesVisitor::post_visit(DeleteStatement &statement) {
  runtime_assert(statement.expression->type == Type::IntStar,
                 "delete expected int*, got " +
                     type_to_string(statement.expression->type));
}
