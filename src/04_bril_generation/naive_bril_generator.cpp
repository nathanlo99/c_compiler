
#include "naive_bril_generator.hpp"
#include "ast_node.hpp"

namespace bril {

void NaiveBRILGenerator::visit(::Program &program) {
  for (auto &function : program.procedures) {
    function.accept_simple(*this);
  }
}

void NaiveBRILGenerator::visit(Procedure &procedure) {
  // TODO
}

void NaiveBRILGenerator::visit(VariableLValueExpr &) {
  unreachable("BRIL generation for lvalue should be handled in assignment");
}

void NaiveBRILGenerator::visit(DereferenceLValueExpr &) {
  unreachable("BRIL generation for lvalue should be handled in assignment");
}

void NaiveBRILGenerator::visit(TestExpr &expr) {
  expr.lhs->accept_simple(*this);
  const std::string lhs_variable = last_result();
  expr.rhs->accept_simple(*this);
  const std::string rhs_variable = last_result();
  const std::string destination = temp();
  switch (expr.operation) {
  case ComparisonOperation::LessThan:
    lt(destination, lhs_variable, rhs_variable);
    break;
  case ComparisonOperation::LessEqual:
    le(destination, lhs_variable, rhs_variable);
    break;
  case ComparisonOperation::GreaterThan:
    gt(destination, lhs_variable, rhs_variable);
    break;
  case ComparisonOperation::GreaterEqual:
    ge(destination, lhs_variable, rhs_variable);
    break;
  case ComparisonOperation::Equal:
    eq(destination, lhs_variable, rhs_variable);
    break;
  case ComparisonOperation::NotEqual:
    ne(destination, lhs_variable, rhs_variable);
    break;
  }
}

void NaiveBRILGenerator::visit(VariableExpr &expr) {
  const std::string destination = temp();
  id(destination, expr.variable.name);
}

void NaiveBRILGenerator::visit(LiteralExpr &expr) {
  const std::string destination = temp();
  constant(destination, std::to_string(expr.literal.value));
}

void NaiveBRILGenerator::visit(BinaryExpr &expr) {
  // TODO
}

void NaiveBRILGenerator::visit(AddressOfExpr &expr) {
  // TODO
}

void NaiveBRILGenerator::visit(DereferenceExpr &expr) {
  // TODO
}

void NaiveBRILGenerator::visit(NewExpr &expr) {
  // TODO
}

void NaiveBRILGenerator::visit(FunctionCallExpr &expr) {
  // TODO
}

void NaiveBRILGenerator::visit(Statements &statements) {
  // TODO
}

void NaiveBRILGenerator::visit(AssignmentStatement &statement) {
  // TODO
}

void NaiveBRILGenerator::visit(IfStatement &statement) {
  // TODO
}

void NaiveBRILGenerator::visit(WhileStatement &statement) {
  // TODO
}

void NaiveBRILGenerator::visit(PrintStatement &statement) {
  // TODO
}

void NaiveBRILGenerator::visit(DeleteStatement &statement) {
  // TODO
}

} // namespace bril
