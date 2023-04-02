
#include "ast_simple_visitor.hpp"
#include "mips_instruction.hpp"

#include <iostream>

struct NaiveCodeGenerator : ASTSimpleVisitor {
  std::vector<MIPSInstruction> instructions;

  virtual ~NaiveCodeGenerator() = default;

  virtual void visit(Program &) override;
  virtual void visit(Procedure &) override;
  virtual void visit(VariableLValueExpr &) override;
  virtual void visit(DereferenceLValueExpr &) override;
  virtual void visit(TestExpr &) override;
  virtual void visit(VariableExpr &) override;
  virtual void visit(LiteralExpr &) override;
  virtual void visit(BinaryExpr &) override;
  virtual void visit(AddressOfExpr &) override;
  virtual void visit(NewExpr &) override;
  virtual void visit(FunctionCallExpr &) override;
  virtual void visit(Statements &) override;
  virtual void visit(AssignmentStatement &) override;
  virtual void visit(IfStatement &) override;
  virtual void visit(WhileStatement &) override;
  virtual void visit(PrintStatement &) override;
  virtual void visit(DeleteStatement &) override;

  void print() {
    for (const auto &instruction : instructions) {
      std::cout << instruction.to_string() << std::endl;
    }
  }
};
