
#include "ast_simple_visitor.hpp"
#include "mips_generator.hpp"
#include "symbol_table.hpp"

#include <iostream>

struct NaiveMIPSGenerator : ASTSimpleVisitor, MIPSGenerator {
  using ASTSimpleVisitor::visit;

  SymbolTable table;

  ~NaiveMIPSGenerator() = default;

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
