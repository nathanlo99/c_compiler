
#include "simple_bril_generator.hpp"
#include "ast_node.hpp"
#include "util.hpp"
#include <memory>

namespace bril {

void SimpleBRILGenerator::visit(::Program &program) {
  for (auto &function : program.procedures) {
    function.accept_simple(*this);
  }
}

void SimpleBRILGenerator::visit(Procedure &procedure) {
  const std::string name = procedure.name;
  std::vector<bril::Variable> params;
  params.reserve(procedure.params.size());
  for (const auto &param : procedure.params) {
    params.emplace_back(param.name, type_from_ast_type(param.type));
  }
  add_function(name, params, type_from_ast_type(procedure.return_type));

  enter_function(name);
  for (const auto &decl : procedure.decls) {
    constant(decl.name, decl.initial_value);
  }
  for (const auto &statement : procedure.statements) {
    statement->accept_simple(*this);
  }
  leave_function();
}

void SimpleBRILGenerator::visit(VariableLValueExpr &expr) {
  unreachable("BRIL generation for variable lvalue ({}) should be handled in "
              "assignment",
              expr.variable.name);
}

void SimpleBRILGenerator::visit(DereferenceLValueExpr &) {
  unreachable(
      "BRIL generation for dereference should be handled in assignment");
}

void SimpleBRILGenerator::visit(AssignmentExpr &expr) {
  expr.rhs->accept_simple(*this);
  const std::string rhs_variable = last_result();
  const Type type = last_type();
  const std::string result_variable = temp();

  if (const auto &lhs =
          std::dynamic_pointer_cast<VariableLValueExpr>(expr.lhs)) {
    id(lhs->variable.name, rhs_variable, type);
    id(result_variable, lhs->variable.name, type);
  } else if (const auto &lhs =
                 std::dynamic_pointer_cast<DereferenceLValueExpr>(expr.lhs)) {
    lhs->argument->accept_simple(*this);
    const std::string lhs_variable = last_result();
    store(lhs_variable, rhs_variable);
    id(result_variable, rhs_variable, type);
  } else {
    unreachable("Assigning to unknown kind of lvalue: was neither "
                "variable nor dereference");
  }
}

void SimpleBRILGenerator::visit(VariableExpr &expr) {
  const std::string destination = temp();
  id(destination, expr.variable.name, type_from_ast_type(expr.variable.type));
}

void SimpleBRILGenerator::visit(LiteralExpr &expr) {
  const std::string destination = temp();
  constant(destination, expr.literal);
}

void SimpleBRILGenerator::visit(BinaryExpr &expr) {
  expr.lhs->accept_simple(*this);
  const std::string lhs_variable = last_result();
  expr.rhs->accept_simple(*this);
  const std::string rhs_variable = last_result();
  const std::string destination = temp();

  const bool left_is_pointer = expr.lhs->type == ::Type::IntStar;
  const bool right_is_pointer = expr.rhs->type == ::Type::IntStar;
  switch (expr.operation) {
  case BinaryOperation::Add:
    if (left_is_pointer) {
      ptradd(destination, lhs_variable, rhs_variable);
    } else if (right_is_pointer) {
      ptradd(destination, rhs_variable, lhs_variable);
    } else {
      add(destination, lhs_variable, rhs_variable);
    }
    break;
  case BinaryOperation::Sub:
    if (left_is_pointer && !right_is_pointer) {
      ptrsub(destination, lhs_variable, rhs_variable);
    } else if (left_is_pointer && right_is_pointer) {
      ptrdiff(destination, lhs_variable, rhs_variable);
    } else {
      sub(destination, lhs_variable, rhs_variable);
    }
    break;
  case BinaryOperation::Mul:
    mul(destination, lhs_variable, rhs_variable);
    break;
  case BinaryOperation::Div:
    div(destination, lhs_variable, rhs_variable);
    break;
  case BinaryOperation::Mod:
    mod(destination, lhs_variable, rhs_variable);
    break;
  case BinaryOperation::LessThan:
    lt(destination, lhs_variable, rhs_variable);
    break;
  case BinaryOperation::LessEqual:
    le(destination, lhs_variable, rhs_variable);
    break;
  case BinaryOperation::GreaterThan:
    gt(destination, lhs_variable, rhs_variable);
    break;
  case BinaryOperation::GreaterEqual:
    ge(destination, lhs_variable, rhs_variable);
    break;
  case BinaryOperation::Equal:
    eq(destination, lhs_variable, rhs_variable);
    break;
  case BinaryOperation::NotEqual:
    ne(destination, lhs_variable, rhs_variable);
    break;
  default:
    unreachable("Unknown binary operation");
  }
}

void SimpleBRILGenerator::visit(BooleanAndExpr &expr) {
  const std::string destination = temp();
  const std::string continue_computing_label = generate_label("andContinue");
  const std::string done_label = generate_label("andDone");

  constant(destination, Literal(0, ::Type::Int));
  expr.lhs->accept_simple(*this);
  const std::string lhs_variable = last_result();
  br(lhs_variable, continue_computing_label, done_label);

  label(continue_computing_label);
  expr.rhs->accept_simple(*this);
  const std::string rhs_variable = last_result();
  id(destination, rhs_variable, Type::Int);

  label(done_label);
}

void SimpleBRILGenerator::visit(BooleanOrExpr &expr) {
  const std::string destination = temp();
  const std::string continue_computing_label = generate_label("orContinue");
  const std::string done_label = generate_label("orDone");

  constant(destination, Literal(1, ::Type::Int));
  expr.lhs->accept_simple(*this);
  const std::string lhs_variable = last_result();
  br(lhs_variable, done_label, continue_computing_label);

  label(continue_computing_label);
  expr.rhs->accept_simple(*this);
  const std::string rhs_variable = last_result();
  id(destination, rhs_variable, Type::Int);

  label(done_label);
}

void SimpleBRILGenerator::visit(AddressOfExpr &expr) {
  const std::string argument = expr.argument->variable.name;
  const std::string destination = temp();
  addressof(destination, argument);
}

void SimpleBRILGenerator::visit(DereferenceExpr &expr) {
  expr.argument->accept_simple(*this);
  const std::string argument = last_result();
  const std::string destination = temp();
  load(destination, argument);
}

void SimpleBRILGenerator::visit(NewExpr &expr) {
  expr.rhs->accept_simple(*this);
  const std::string argument = last_result();
  const std::string destination = temp();
  alloc(destination, argument);
}

void SimpleBRILGenerator::visit(FunctionCallExpr &expr) {
  std::vector<std::string> argument_names;
  for (auto &argument : expr.arguments) {
    argument->accept_simple(*this);
    const std::string argument_name = last_result();
    argument_names.push_back(argument_name);
  }
  const std::string destination = temp();
  const Type type = type_from_ast_type(expr.type);
  call(destination, expr.procedure_name, argument_names, type);
}

void SimpleBRILGenerator::visit(Statements &statements) {
  for (auto &statement : statements.statements) {
    statement->accept_simple(*this);
  }
}

void SimpleBRILGenerator::visit(ExprStatement &statement) {
  statement.expr->accept_simple(*this);
  // Ignore result
}

void SimpleBRILGenerator::visit(AssignmentStatement &statement) {
  statement.rhs->accept_simple(*this);
  const std::string rhs_variable = last_result();

  if (const auto &lhs =
          std::dynamic_pointer_cast<VariableLValueExpr>(statement.lhs)) {
    const Type type = last_type();
    id(lhs->variable.name, rhs_variable, type);
  } else if (const auto &lhs = std::dynamic_pointer_cast<DereferenceLValueExpr>(
                 statement.lhs)) {
    lhs->argument->accept_simple(*this);
    const std::string lhs_variable = last_result();
    store(lhs_variable, rhs_variable);
  } else {
    unreachable("Assigning to unknown kind of lvalue: was neither "
                "variable nor dereference");
  }
}

void SimpleBRILGenerator::visit(IfStatement &statement) {
  const std::string true_label = generate_label("ifTrue");
  const std::string false_label = generate_label("ifFalse");
  const std::string endif_label = generate_label("ifEndif");

  statement.test_expression->accept_simple(*this);
  const std::string cond = last_result();
  br(cond, true_label, false_label);
  label(true_label);
  statement.true_statements.accept_simple(*this);
  jmp(endif_label);
  label(false_label);
  statement.false_statements.accept_simple(*this);
  label(endif_label);
}

void SimpleBRILGenerator::visit(WhileStatement &statement) {
  const std::string loop_label = generate_label("whileLoop");
  const std::string end_label = generate_label("whileEnd");
  const std::string body_label = generate_label("whileBody");

  push_loop(loop_label, end_label);

  label(loop_label);
  statement.test_expression->accept_simple(*this);
  const std::string cond = last_result();
  br(cond, body_label, end_label);
  label(body_label);
  statement.body_statement->accept_simple(*this);
  jmp(loop_label);
  label(end_label);

  pop_loop();
}

void SimpleBRILGenerator::visit(ForStatement &statement) {
  const std::string loop_label = generate_label("forLoop");
  const std::string end_label = generate_label("forEnd");
  const std::string update_label = generate_label("forUpdate");
  const std::string body_label = generate_label("forBody");

  statement.init_expression->accept_simple(*this);

  push_loop(update_label, end_label);

  label(loop_label);
  statement.test_expression->accept_simple(*this);
  const std::string cond = last_result();
  br(cond, body_label, end_label);
  label(body_label);
  statement.body_statement->accept_simple(*this);
  label(update_label);
  statement.update_expression->accept_simple(*this);
  jmp(loop_label);
  label(end_label);

  pop_loop();
}

void SimpleBRILGenerator::visit(PrintStatement &statement) {
  statement.expression->accept_simple(*this);
  const std::string result = last_result();
  print(result);
}

void SimpleBRILGenerator::visit(DeleteStatement &statement) {
  statement.expression->accept_simple(*this);
  const std::string result = last_result();
  free(result);
}

void SimpleBRILGenerator::visit(BreakStatement &) {
  jmp(get_break_label());
  const std::string break_label = generate_label("postBreak");
  label(break_label);
}

void SimpleBRILGenerator::visit(ContinueStatement &) {
  jmp(get_continue_label());
  const std::string continue_label = generate_label("postContinue");
  label(continue_label);
}

void SimpleBRILGenerator::visit(ReturnStatement &statement) {
  statement.expr->accept_simple(*this);
  ret(last_result());
}

} // namespace bril

// namespace bril
