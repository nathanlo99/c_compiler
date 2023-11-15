
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
    import_module("init");
    import_module("new");
    import_module("delete");
  }

  if (table.use_print) {
    import_module("print");
    load_const(Reg::R10, "print");
  }

  init_constants();
  jmp("wain");
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
    push(Reg::R1);
    push(Reg::R2);

    if (table.use_memory) {
      comment("Calling init");
      const bool first_arg_is_array =
          table.get_arguments("wain")[0].type == Type::IntStar;
      if (!first_arg_is_array) {
        add(Reg::R2, Reg::R0, Reg::R0);
      }
      push(Reg::R31);
      load_and_jalr(Reg::R5, "init");
      pop(Reg::R31);
      comment("Done calling init");
    }
  }
  sub(Reg::R29, Reg::R30, Reg::R4);

  for (const auto &variable : procedure.decls) {
    push_const(Reg::R3, variable.initial_value.value);
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

  jr(Reg::R31);
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
    sw(Reg::R3, offset, Reg::R29);
  } else if (const auto lvalue =
                 std::dynamic_pointer_cast<DereferenceLValueExpr>(expr.lhs)) {
    lvalue->argument->accept_simple(*this);
    push(Reg::R3);
    expr.rhs->accept_simple(*this);
    pop(Reg::R5);
    sw(Reg::R3, 0, Reg::R5);
  } else {
    unreachable("Unknown lvalue type");
  }
}

void NaiveMIPSGenerator::visit(VariableExpr &expr) {
  const int offset = table.get_offset(expr.variable);
  lw(Reg::R3, offset, Reg::R29);
  annotate("Loading " + expr.variable.name);
}

void NaiveMIPSGenerator::visit(LiteralExpr &expr) {
  load_const(Reg::R3, expr.literal.value);
  annotate("Loading the literal " + expr.literal.value_to_string());
}

void NaiveMIPSGenerator::visit(BinaryExpr &expr) {
  const auto lhs_type = expr.lhs->type;
  const auto rhs_type = expr.rhs->type;
  expr.lhs->accept_simple(*this);
  push(Reg::R3);
  expr.rhs->accept_simple(*this);
  pop(Reg::R5);
  switch (expr.operation) {
  case BinaryOperation::Add:
    if (lhs_type == Type::IntStar) {
      // Multiply rhs ($3) by 4
      mult(Reg::R3, Reg::R3, Reg::R4);
    } else if (rhs_type == Type::IntStar) {
      // Multiply lhs ($5) by 4
      mult(Reg::R5, Reg::R5, Reg::R4);
    }
    add(Reg::R3, Reg::R5, Reg::R3);
    break;
  case BinaryOperation::Sub:
    if (lhs_type == Type::Int && rhs_type == Type::Int) {
      sub(Reg::R3, Reg::R5, Reg::R3);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::Int) {
      // lhs - 4 * rhs  -->  $5 - 4 * $3
      mult(Reg::R3, Reg::R3, Reg::R4);
      sub(Reg::R3, Reg::R5, Reg::R3);
    } else if (lhs_type == Type::IntStar && rhs_type == Type::IntStar) {
      // (lhs - rhs) / 4  --> ($5 - $3) / $4
      sub(Reg::R3, Reg::R5, Reg::R3);
      div(Reg::R3, Reg::R3, Reg::R4);
    }
    break;
  case BinaryOperation::Mul:
    mult(Reg::R3, Reg::R5, Reg::R3);
    break;
  case BinaryOperation::Div:
    div(Reg::R3, Reg::R5, Reg::R3);
    break;
  case BinaryOperation::Mod:
    mod(Reg::R3, Reg::R5, Reg::R3);
    break;
  case BinaryOperation::Equal: {
    const std::string equal_label = generate_label("equalEqual");
    const std::string done_label = generate_label("equalEnd");

    beq(Reg::R5, Reg::R3, equal_label);
    add(Reg::R3, Reg::R0, Reg::R0);
    jmp(done_label);

    label(equal_label);
    add(Reg::R3, Reg::R0, Reg::R11);

    label(done_label);
  } break;
  case BinaryOperation::NotEqual: {
    const std::string equal_label = generate_label("equalEqual");
    const std::string done_label = generate_label("equalEnd");

    beq(Reg::R5, Reg::R3, equal_label);
    add(Reg::R3, Reg::R0, Reg::R11);
    jmp(done_label);

    label(equal_label);
    add(Reg::R3, Reg::R0, Reg::R0);

    label(done_label);
  } break;
  case BinaryOperation::LessThan:
    slt(Reg::R3, Reg::R5, Reg::R3);
    break;
  case BinaryOperation::LessEqual:
    slt(Reg::R3, Reg::R3, Reg::R5);  // $3 =   $3 < $5   = (rhs < lhs)
    sub(Reg::R3, Reg::R11, Reg::R3); // $3 =    not $3   = (rhs >= lhs)
    break;
  case BinaryOperation::GreaterThan:
    slt(Reg::R3, Reg::R3, Reg::R5);
    break;
  case BinaryOperation::GreaterEqual:
    slt(Reg::R3, Reg::R5, Reg::R3);  // $3 =   $5 < $3   = (lhs < rhs)
    sub(Reg::R3, Reg::R11, Reg::R3); // $3 =    not $3   = (lhs >= rhs)
  }
}

void NaiveMIPSGenerator::visit(BooleanAndExpr &expr) {
  const auto stop_label = generate_label("andStop");
  expr.lhs->accept_simple(*this);
  beq(Reg::R3, Reg::R0, stop_label);
  expr.rhs->accept_simple(*this);
  label(stop_label);
}

void NaiveMIPSGenerator::visit(BooleanOrExpr &expr) {
  const auto stop_label = generate_label("orStop");
  expr.lhs->accept_simple(*this);
  bne(Reg::R3, Reg::R0, stop_label);
  expr.rhs->accept_simple(*this);
  label(stop_label);
}

void NaiveMIPSGenerator::visit(AddressOfExpr &expr) {
  if (const auto lhs =
          std::dynamic_pointer_cast<VariableLValueExpr>(expr.argument)) {
    const int offset = table.get_offset(lhs->variable);
    load_const(Reg::R3, offset);
    add(Reg::R3, Reg::R3, Reg::R29);
  } else if (const auto lhs = std::dynamic_pointer_cast<DereferenceLValueExpr>(
                 expr.argument)) {
    lhs->argument->accept_simple(*this);
  } else {
    unreachable("Unknown lvalue type");
  }
}

void NaiveMIPSGenerator::visit(DereferenceExpr &expr) {
  expr.argument->accept_simple(*this);
  lw(Reg::R3, 0, Reg::R3);
}

void NaiveMIPSGenerator::visit(NewExpr &expr) {
  const std::string new_success_label = generate_label("newSuccess");
  expr.rhs->accept_simple(*this);
  add(Reg::R1, Reg::R3, Reg::R0);
  push(Reg::R31);
  load_and_jalr(Reg::R5, "new");
  pop(Reg::R31);
  // The result is stored in $1: copy it to $3, and return NULL (1) if it was 0
  bne(Reg::R3, Reg::R0, new_success_label);
  add(Reg::R3, Reg::R11, Reg::R0);
  label(new_success_label);
}

void NaiveMIPSGenerator::visit(FunctionCallExpr &expr) {
  const std::string procedure_name = expr.procedure_name;
  const auto params = table.get_arguments(procedure_name);
  const size_t num_arguments = params.size();
  push(Reg::R29);
  push(Reg::R31);
  for (size_t i = 0; i < num_arguments; ++i) {
    comment("Pushing argument " + params[i].name);
    auto &argument = expr.arguments[i];
    argument->accept_simple(*this);
    push(Reg::R3);
    comment("Done pushing argument " + params[i].name);
  }
  load_and_jalr(Reg::R5, procedure_name);
  pop_and_discard(num_arguments);
  pop(Reg::R31);
  pop(Reg::R29);
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
  std::ostringstream ss;
  statement.emit_c(ss, 0);
  comment(ss.str());
  if (const auto lvalue =
          std::dynamic_pointer_cast<VariableLValueExpr>(statement.lhs)) {
    const int offset = table.get_offset(lvalue->variable);
    statement.rhs->accept_simple(*this);
    sw(Reg::R3, offset, Reg::R29);
  } else if (const auto lvalue =
                 std::dynamic_pointer_cast<DereferenceLValueExpr>(
                     statement.lhs)) {
    lvalue->argument->accept_simple(*this);
    push(Reg::R3);
    statement.rhs->accept_simple(*this);
    pop(Reg::R5);
    sw(Reg::R3, 0, Reg::R5);
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
  const auto &compare = [&](const Reg d, const Reg s, const Reg t) {
    uses_pointers ? sltu(d, s, t) : slt(d, s, t);
  };
  const auto &lhs = test_expr->lhs;
  const auto &rhs = test_expr->rhs;

  lhs->accept_simple(*this);
  push(Reg::R3);
  rhs->accept_simple(*this);
  pop(Reg::R5);

  switch (test_expr->operation) {
  case BinaryOperation::LessThan: {
    compare(Reg::R3, Reg::R5, Reg::R3);
    beq(Reg::R3, Reg::R0, else_label);
  } break;
  case BinaryOperation::Equal: {
    bne(Reg::R3, Reg::R5, else_label);
  } break;
  default: {
    unreachable("Non-canonical comparison operation");
  } break;
  }

  statement.true_statements.accept_simple(*this);
  jmp(endif_label);
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
  const auto &compare = [&](const Reg d, const Reg s, const Reg t) {
    uses_pointers ? sltu(d, s, t) : slt(d, s, t);
  };

  label(loop_label);
  test_expr->lhs->accept_simple(*this);
  push(Reg::R3);
  test_expr->rhs->accept_simple(*this);
  pop(Reg::R5);

  // $5 = lhs, $3 = rhs
  // Jump to end if the condition is false
  switch (test_expr->operation) {
  case BinaryOperation::LessThan: {
    compare(Reg::R3, Reg::R5, Reg::R3);
    beq(Reg::R3, Reg::R0, end_label);
  } break;
  case BinaryOperation::LessEqual: {
    compare(Reg::R3, Reg::R3, Reg::R5);
    bne(Reg::R3, Reg::R0, end_label);
  } break;
  case BinaryOperation::GreaterThan: {
    compare(Reg::R3, Reg::R3, Reg::R5);
    beq(Reg::R3, Reg::R0, end_label);
  } break;
  case BinaryOperation::GreaterEqual: {
    compare(Reg::R3, Reg::R5, Reg::R3);
    bne(Reg::R3, Reg::R0, end_label);
  } break;
  case BinaryOperation::Equal: {
    bne(Reg::R3, Reg::R5, end_label);
  } break;
  case BinaryOperation::NotEqual: {
    beq(Reg::R3, Reg::R5, end_label);
  } break;
  default: {
    unreachable("Binary operation in while statement was not boolean");
  } break;
  }

  statement.body_statement->accept_simple(*this);
  jmp(loop_label);
  label(end_label);
}

void NaiveMIPSGenerator::visit(PrintStatement &statement) {
  statement.expression->accept_simple(*this);
  add(Reg::R1, Reg::R3, Reg::R0);
  push(Reg::R31);
  load_and_jalr(Reg::R5, "print");
  pop(Reg::R31);
}

void NaiveMIPSGenerator::visit(DeleteStatement &statement) {
  const auto skip_label = generate_label("deleteskip");
  statement.expression->accept_simple(*this);
  beq(Reg::R3, Reg::R11, skip_label);
  add(Reg::R1, Reg::R3, Reg::R0);
  push(Reg::R31);
  load_and_jalr(Reg::R5, "delete");
  pop(Reg::R31);
  label(skip_label);
}
