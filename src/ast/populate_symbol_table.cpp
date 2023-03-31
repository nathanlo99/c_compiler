#include "populate_symbol_table.hpp"
#include "ast_node.hpp"

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
