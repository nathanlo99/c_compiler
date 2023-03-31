#include "populate_symbol_table.hpp"
#include "ast_node.hpp"

void PopulateSymbolTableVisitor::post_visit(Program &program) {
  program.table = table;
}

void PopulateSymbolTableVisitor::pre_visit(Procedure &procedure) {
  const auto name = procedure.name;
  for (const auto &variable : procedure.params) {
    table.add_parameter(name, variable);
  }
  table.set_return_type(name, Type::Int);
  for (const auto &variable : procedure.decls) {
    table.add_variable(name, variable);
  }
}

void PopulateSymbolTableVisitor::pre_visit(VariableExpr &expr) {
  table.record_variable_read(expr.variable);
}

void PopulateSymbolTableVisitor::pre_visit(VariableLValueExpr &expr) {
  table.record_variable_write(expr.variable);
}
