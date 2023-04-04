
#pragma once

#include "ast_simple_visitor.hpp"
#include "bril_generator.hpp"

namespace bril {

struct NaiveBRILGenerator : BRILGenerator, ASTSimpleVisitor {
  using ASTSimpleVisitor::visit;

  SymbolTable table;

  void enter_procedure(const std::string &procedure) {
    BRILGenerator::enter_function(procedure);
    table.enter_procedure(procedure);
  }

  ~NaiveBRILGenerator() = default;

  void visit(Program &) override;
  void visit(Procedure &) override;
  void visit(VariableLValueExpr &) override;
  void visit(DereferenceLValueExpr &) override;
  void visit(TestExpr &) override;
  void visit(VariableExpr &) override;
  void visit(LiteralExpr &) override;
  void visit(BinaryExpr &) override;
  void visit(AddressOfExpr &) override;
  void visit(DereferenceExpr &) override;
  void visit(NewExpr &) override;
  void visit(FunctionCallExpr &) override;
  void visit(Statements &) override;
  void visit(AssignmentStatement &) override;
  void visit(IfStatement &) override;
  void visit(WhileStatement &) override;
  void visit(PrintStatement &) override;
  void visit(DeleteStatement &) override;
};
} // namespace bril
