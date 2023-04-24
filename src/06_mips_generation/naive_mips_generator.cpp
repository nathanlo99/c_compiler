
#include "naive_mips_generator.hpp"
#include "ast_node.hpp"
#include "mips_instruction.hpp"
#include "util.hpp"
#include <memory>
#include <sys/errno.h>

void NaiveMIPSGenerator::visit(Program &program) {
  table = program.table;

  // Emit the C code
  comment("// Simplified C code:");
  std::stringstream ss;
  program.emit_c(ss, 0);
  std::string line;
  while (std::getline(ss, line)) {
    comment(line);
  }

  if (table.use_memory) {
    import("init");
    import("new");
    import("delete");
  }

  if (table.use_print) {
    import("print");
    load_const(10, "print");
  }

  init_constants();
  beq(0, 0, "wain");
  annotate("Done prologue, jumping to wain");

  for (auto &procedure : program.procedures) {
    procedure.accept_simple(*this);
  }

  optimize();

  comment("Number of assembly instructions: " +
          std::to_string(num_assembly_instructions()));
}

void NaiveMIPSGenerator::visit(Procedure &procedure) {
  const std::string procedure_name = procedure.name;
  const bool is_wain = procedure_name == "wain";

  table.enter_procedure(procedure_name);

  comment("");
  comment("Generating code for " + procedure_name);
  label(procedure_name);
  if (is_wain) {
    push(1);
    push(2);

    if (table.use_memory) {
      comment("Calling init");
      const bool first_arg_is_array =
          table.get_arguments("wain")[0].type == Type::IntStar;
      if (!first_arg_is_array) {
        add(2, 0, 0);
      }
      push(31);
      load_and_jalr(5, "init");
      pop(31);
      comment("Done calling init");
    }
  }
  sub(29, 30, 4);

  for (const auto &variable : procedure.decls) {
    push_const(3, variable.initial_value.value);
    annotate("Declaration " + variable.name);
  }

  comment("Code for statements:");
  for (const auto &statement : procedure.statements) {
    statement->accept_simple(*this);
  }
  comment("Code for return value:");
  procedure.return_expr->accept_simple(*this);

  // We only have to do clean-up if we aren't wain
  if (procedure_name != "wain") {
    comment("Done evaluating result, popping decls and saved registers");
    pop_and_discard(procedure.decls.size());
  }

  jr(31);
  annotate("Done generating code for " + procedure_name);

  table.leave_procedure();
}

void NaiveMIPSGenerator::visit(VariableLValueExpr &) {
  unreachable("Variable lvalue code is handled in addressof and assignment");
}

void NaiveMIPSGenerator::visit(DereferenceLValueExpr &) {
  unreachable("Dereference lvalue code is handled in addressof and assignment");
}

void NaiveMIPSGenerator::visit(AssignmentExpr &expr) {
  std::stringstream ss;
  expr.emit_c(ss, 0);
  ss << ";";
  comment(ss.str());
  if (const auto lvalue =
          std::dynamic_pointer_cast<VariableLValueExpr>(expr.lhs)) {
    const int offset = table.get_offset(lvalue->variable);
    expr.rhs->accept_simple(*this);
    sw(3, offset, 29);
  } else if (const auto lvalue =
                 std::dynamic_pointer_cast<DereferenceLValueExpr>(expr.lhs)) {
    lvalue->argument->accept_simple(*this);
    push(3);
    expr.rhs->accept_simple(*this);
    pop(5);
    sw(3, 0, 5);
  } else {
    unreachable("Unknown lvalue type");
  }
}

void NaiveMIPSGenerator::visit(TestExpr &) {
  unreachable(
      "Test expression code is handled in if statement and while statement");
}

void NaiveMIPSGenerator::visit(VariableExpr &expr) {
  const int offset = table.get_offset(expr.variable);
  lw(3, offset, 29);
  annotate("Loading " + expr.variable.name);
}

void NaiveMIPSGenerator::visit(LiteralExpr &expr) {
  load_const(3, expr.literal.value);
  annotate("Loading the literal " + expr.literal.value_to_string());
}

void NaiveMIPSGenerator::visit(BinaryExpr &expr) {
  const auto lhs_type = expr.lhs->type;
  const auto rhs_type = expr.rhs->type;
  expr.lhs->accept_simple(*this);
  push(3);
  expr.rhs->accept_simple(*this);
  pop(5);
  switch (expr.operation) {
  case BinaryOperation::Add:
    if (lhs_type == Type::IntStar) {
      // Multiply rhs ($3) by 4
      mult(3, 3, 4);
    } else if (rhs_type == Type::IntStar) {
      // Multiply lhs ($5) by 4
      mult(5, 5, 4);
    }
    add(3, 5, 3);
    break;
  case BinaryOperation::Sub:
    if (lhs_type == Type::Int && rhs_type == Type::Int) {
      sub(3, 5, 3);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::Int) {
      // lhs - 4 * rhs  -->  $5 - 4 * $3
      mult(3, 3, 4);
      sub(3, 5, 3);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::IntStar) {
      // (lhs - rhs) / 4  --> ($5 - $3) / $4
      sub(3, 5, 3);
      div(3, 3, 4);
    }
    break;
  case BinaryOperation::Mul:
    mult(3, 5, 3);
    break;
  case BinaryOperation::Div:
    div(3, 5, 3);
    break;
  case BinaryOperation::Mod:
    mod(3, 5, 3);
    break;
  default:
    debug_assert(false, "Unknown binary operation");
  }
}

void NaiveMIPSGenerator::visit(AddressOfExpr &expr) {
  if (const auto lhs =
          std::dynamic_pointer_cast<VariableLValueExpr>(expr.argument)) {
    const int offset = table.get_offset(lhs->variable);
    load_const(3, offset);
    add(3, 3, 29);
  } else if (const auto lhs = std::dynamic_pointer_cast<DereferenceLValueExpr>(
                 expr.argument)) {
    lhs->argument->accept_simple(*this);
  } else {
    unreachable("Unknown lvalue type");
  }
}

void NaiveMIPSGenerator::visit(DereferenceExpr &expr) {
  expr.argument->accept_simple(*this);
  lw(3, 0, 3);
}

void NaiveMIPSGenerator::visit(NewExpr &expr) {
  expr.rhs->accept_simple(*this);
  add(1, 3, 0);
  push(31);
  load_and_jalr(5, "new");
  pop(31);
  // The result is stored in $1: copy it to $3, and return NULL (1) if it was 0
  bne(3, 0, 1);
  add(3, 11, 0);
}

void NaiveMIPSGenerator::visit(FunctionCallExpr &expr) {
  const std::string procedure_name = expr.procedure_name;
  const auto params = table.get_arguments(procedure_name);
  const size_t num_arguments = params.size();
  push(29);
  push(31);
  for (size_t i = 0; i < num_arguments; ++i) {
    comment("Pushing argument " + params[i].name);
    auto &argument = expr.arguments[i];
    argument->accept_simple(*this);
    push(3);
    comment("Done pushing argument " + params[i].name);
  }
  load_and_jalr(5, procedure_name);
  pop_and_discard(num_arguments);
  pop(31);
  pop(29);
}

void NaiveMIPSGenerator::visit(Statements &statements) {
  for (auto &statement : statements.statements) {
    statement->accept_simple(*this);
  }
}

void NaiveMIPSGenerator::visit(ExprStatement &statement) {
  statement.expr->accept_simple(*this);
}

void NaiveMIPSGenerator::visit(AssignmentStatement &statement) {
  std::stringstream ss;
  statement.emit_c(ss, 0);
  comment(ss.str());
  if (const auto lvalue =
          std::dynamic_pointer_cast<VariableLValueExpr>(statement.lhs)) {
    const int offset = table.get_offset(lvalue->variable);
    statement.rhs->accept_simple(*this);
    sw(3, offset, 29);
  } else if (const auto lvalue =
                 std::dynamic_pointer_cast<DereferenceLValueExpr>(
                     statement.lhs)) {
    lvalue->argument->accept_simple(*this);
    push(3);
    statement.rhs->accept_simple(*this);
    pop(5);
    sw(3, 0, 5);
  } else {
    unreachable("Unknown lvalue type");
  }
}

void NaiveMIPSGenerator::visit(IfStatement &statement) {
  const auto else_label = generate_label("ifelse");
  const auto endif_label = generate_label("ifendif");

  const auto &test_expr = statement.test_expression;
  const bool uses_pointers = test_expr->lhs->type == Type::IntStar ||
                             test_expr->rhs->type == Type::IntStar;
  const auto &compare = [&](const int d, const int s, const int t) {
    uses_pointers ? sltu(d, s, t) : slt(d, s, t);
  };
  const auto &lhs = test_expr->lhs;
  const auto &rhs = test_expr->rhs;

  lhs->accept_simple(*this);
  push(3);
  rhs->accept_simple(*this);
  pop(5);

  switch (test_expr->operation) {
  case ComparisonOperation::LessThan: {
    compare(3, 5, 3);
    beq(3, 0, else_label);
  } break;
  case ComparisonOperation::Equal: {
    bne(3, 5, else_label);
  } break;
  default: {
    unreachable("Non-canonical comparison operation");
  } break;
  }

  statement.true_statements.accept_simple(*this);
  beq(0, 0, endif_label);
  label(else_label);
  statement.false_statements.accept_simple(*this);
  label(endif_label);
}

void NaiveMIPSGenerator::visit(WhileStatement &statement) {
  const auto loop_label = generate_label("whileloop");
  const auto end_label = generate_label("whileend");

  const auto &test_expr = statement.test_expression;
  const bool uses_pointers = test_expr->lhs->type == Type::IntStar ||
                             test_expr->rhs->type == Type::IntStar;
  const auto &compare = [&](const int d, const int s, const int t) {
    uses_pointers ? sltu(d, s, t) : slt(d, s, t);
  };

  label(loop_label);
  test_expr->lhs->accept_simple(*this);
  push(3);
  test_expr->rhs->accept_simple(*this);
  pop(5);

  // $5 = lhs, $3 = rhs
  // Jump to end if the condition is false
  switch (test_expr->operation) {
  case ComparisonOperation::LessThan: {
    compare(3, 5, 3);
    beq(3, 0, end_label);
  } break;
  case ComparisonOperation::LessEqual: {
    compare(3, 3, 5);
    bne(3, 0, end_label);
  } break;
  case ComparisonOperation::GreaterThan: {
    compare(3, 3, 5);
    beq(3, 0, end_label);
  } break;
  case ComparisonOperation::GreaterEqual: {
    compare(3, 5, 3);
    bne(3, 0, end_label);
  } break;
  case ComparisonOperation::Equal: {
    bne(3, 5, end_label);
  } break;
  case ComparisonOperation::NotEqual: {
    beq(3, 5, end_label);
  } break;
  }

  statement.body_statement->accept_simple(*this);
  beq(0, 0, loop_label);
  label(end_label);
}

void NaiveMIPSGenerator::visit(PrintStatement &statement) {
  statement.expression->accept_simple(*this);
  add(1, 3, 0);
  push(31);
  load_and_jalr(5, "print");
  pop(31);
}

void NaiveMIPSGenerator::visit(DeleteStatement &statement) {
  const auto skip_label = generate_label("deleteskip");
  statement.expression->accept_simple(*this);
  beq(3, 11, skip_label);
  add(1, 3, 0);
  push(31);
  load_and_jalr(5, "delete");
  pop(31);
  label(skip_label);
}
