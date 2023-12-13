
#include "ast_simple_visitor.hpp"
#include "mips_generator.hpp"
#include "symbol_table.hpp"

#include <iostream>

struct NaiveMIPSGenerator : ASTSimpleVisitor, MIPSGenerator {
  using ASTSimpleVisitor::visit;

  SymbolTable table;
  std::string current_return_label;
  std::vector<std::pair<std::string, std::string>> loop_label_stack;

  void push_loop(const std::string &start_label, const std::string &end_label) {
    loop_label_stack.emplace_back(start_label, end_label);
  }
  void pop_loop() {
    debug_assert(!loop_label_stack.empty(), "Loop stack is empty");
    loop_label_stack.pop_back();
  }
  std::string get_break_label() const {
    debug_assert(!loop_label_stack.empty(), "Cannot break outside of loop");
    return loop_label_stack.back().second;
  }
  std::string get_continue_label() const {
    debug_assert(!loop_label_stack.empty(), "Cannot continue outside of loop");
    return loop_label_stack.back().first;
  }

  ~NaiveMIPSGenerator() = default;

  void visit(Program &) override;
  void visit(Procedure &) override;
  void visit(VariableLValueExpr &) override;
  void visit(DereferenceLValueExpr &) override;
  void visit(AssignmentExpr &) override;
  void visit(VariableExpr &) override;
  void visit(LiteralExpr &) override;
  void visit(BinaryExpr &) override;
  void visit(BooleanOrExpr &) override;
  void visit(BooleanAndExpr &) override;
  void visit(AddressOfExpr &) override;
  void visit(DereferenceExpr &) override;
  void visit(NewExpr &) override;
  void visit(FunctionCallExpr &) override;
  void visit(Statements &) override;
  void visit(ExprStatement &) override;
  void visit(AssignmentStatement &) override;
  void visit(IfStatement &) override;
  void visit(WhileStatement &) override;
  void visit(ForStatement &) override;
  void visit(PrintStatement &) override;
  void visit(DeleteStatement &) override;
  void visit(BreakStatement &) override;
  void visit(ContinueStatement &) override;
  void visit(ReturnStatement &) override;
};
