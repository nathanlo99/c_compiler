#include "populate_symbol_table.hpp"
#include "ast_node.hpp"

void PopulateSymbolTableVisitor::post_visit(Program &program) {
  program.table = table;

  for (Procedure &procedure : program.procedures) {
    procedure.table = table.get_table(procedure.name);
  }
}

void PopulateSymbolTableVisitor::pre_visit(Procedure &procedure) {
  const auto name = procedure.name;
  table.add_procedure(name);
  for (const auto &variable : procedure.params) {
    table.add_parameter(name, variable);
  }
  table.set_return_type(name, Type::Int);
  for (const auto &variable : procedure.decls) {
    table.add_variable(name, variable);
  }
  table.enter_procedure(name);
}

void PopulateSymbolTableVisitor::post_visit(Procedure &) {
  table.leave_procedure();
}

void PopulateSymbolTableVisitor::pre_visit(VariableExpr &expr) {
  table.record_variable_read(expr.variable);
}

void PopulateSymbolTableVisitor::pre_visit(VariableLValueExpr &expr) {
  table.record_variable_write(expr.variable);
}
