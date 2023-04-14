
#include "ast_node.hpp"

inline std::string pad(const size_t depth) {
  return std::string(2 * depth, ' ');
}

void Literal::print(const size_t depth) const {
  std::cout << pad(depth) << value_to_string() << ": " << type_to_string(type)
            << std::endl;
}

void Variable::print(const size_t depth) const {
  std::cout << pad(depth) << name << ": " << type_to_string(type) << " = "
            << initial_value.value << std::endl;
}

void Statements::print(const size_t depth) const {
  for (const auto &statement : statements) {
    statement->print(depth);
  }
}

void Procedure::print(const size_t depth) const {
  std::cout << pad(depth) << "Procedure {" << std::endl;
  std::cout << pad(depth + 1) << "name: " << name << std::endl;
  std::cout << pad(depth + 1) << "return_type: " << type_to_string(return_type)
            << std::endl;
  std::cout << pad(depth + 1) << "parameters: " << std::endl;
  for (const Variable &variable : params) {
    const bool is_used = table.is_variable_used(variable.name);
    std::cout << pad(depth + 2) << variable.name << ": "
              << type_to_string(variable.type) << " "
              << (is_used ? "(used)" : "(unused)") << std::endl;
  }
  std::cout << pad(depth + 1) << "declarations: " << std::endl;
  for (const Variable &variable : decls) {
    const bool is_used = table.is_variable_used(variable.name);
    std::cout << pad(depth + 2) << variable.name << ": "
              << type_to_string(variable.type) << " = "
              << variable.initial_value.value << " "
              << (is_used ? "(used)" : "(unused)") << std::endl;
  }
  std::cout << pad(depth + 1) << "statements: " << std::endl;
  for (const auto &statement : statements) {
    statement->print(depth + 2);
  }
  std::cout << pad(depth + 1) << "return_expr: " << std::endl;
  return_expr->print(depth + 2);
  std::cout << pad(depth) << "}" << std::endl;
}

void Program::print(const size_t depth) const {
  for (const auto &procedure : procedures) {
    procedure.print(depth);
    std::cout << std::endl;
  }
}

void VariableLValueExpr::print(const size_t depth) const {
  std::cout << pad(depth) << "VariableLValueExpr(" << variable.name << ")";
  if (type != Type::Unknown)
    std::cout << " : " << type_to_string(type);
  std::cout << std::endl;
}

void DereferenceLValueExpr::print(const size_t depth) const {
  std::cout << pad(depth) << "DereferenceLValueExpr {" << std::endl;
  argument->print(depth + 1);
  std::cout << pad(depth) << "}";
  if (type != Type::Unknown)
    std::cout << " : " << type_to_string(type);
  std::cout << std::endl;
}

void TestExpr::print(const size_t depth) const {
  std::cout << pad(depth) << "TestExpr {" << std::endl;
  std::cout << pad(depth + 1) << "lhs: " << std::endl;
  lhs->print(depth + 2);
  std::cout << pad(depth + 1)
            << "operation: " << comparison_operation_to_string(operation)
            << std::endl;
  std::cout << pad(depth + 1) << "rhs: " << std::endl;
  rhs->print(depth + 2);
  std::cout << pad(depth) << "}" << std::endl;
}

void VariableExpr::print(const size_t depth) const {
  std::cout << pad(depth) << "VariableExpr(" << variable.name << ")";
  if (type != Type::Unknown)
    std::cout << " : " << type_to_string(type);
  std::cout << std::endl;
}

void LiteralExpr::print(const size_t depth) const {
  std::cout << pad(depth) << literal.value << ": "
            << type_to_string(literal.type) << std::endl;
}

void BinaryExpr::print(const size_t depth) const {
  std::cout << pad(depth) << "BinaryExpr {" << std::endl;
  std::cout << pad(depth + 1) << "lhs: " << std::endl;
  lhs->print(depth + 2);
  std::cout << pad(depth + 1)
            << "operation: " << binary_operation_to_string(operation)
            << std::endl;
  std::cout << pad(depth + 1) << "rhs: " << std::endl;
  rhs->print(depth + 2);
  std::cout << pad(depth) << "}";
  if (type != Type::Unknown)
    std::cout << " : " << type_to_string(type);
  std::cout << std::endl;
}

void AddressOfExpr::print(const size_t depth) const {
  std::cout << pad(depth) << "AddressOfExpr {" << std::endl;
  argument->print(depth + 1);
  std::cout << pad(depth) << "}";
  if (type != Type::Unknown)
    std::cout << " : " << type_to_string(type);
  std::cout << std::endl;
}

void DereferenceExpr::print(const size_t depth) const {
  std::cout << pad(depth) << "DereferenceExpr {" << std::endl;
  argument->print(depth + 1);
  std::cout << pad(depth) << "}";
  if (type != Type::Unknown)
    std::cout << " : " << type_to_string(type);
  std::cout << std::endl;
}

void NewExpr::print(const size_t depth) const {
  std::cout << pad(depth) << "NewExpr {" << std::endl;
  rhs->print(depth + 1);
  std::cout << pad(depth) << "}";
  if (type != Type::Unknown)
    std::cout << " : " << type_to_string(type);
  std::cout << std::endl;
}

void FunctionCallExpr::print(const size_t depth) const {
  std::cout << pad(depth) << "FunctionCall {" << std::endl;
  std::cout << pad(depth + 1) << "procedure_name: " << procedure_name << ","
            << std::endl;
  std::cout << pad(depth + 1) << "arguments: [" << std::endl;
  for (const auto &expr : arguments) {
    expr->print(depth + 2);
  }
  std::cout << pad(depth + 1) << "]" << std::endl;
  std::cout << pad(depth) << "}";
  if (type != Type::Unknown)
    std::cout << " : " << type_to_string(type);
  std::cout << std::endl;
}

void AssignmentStatement::print(const size_t depth) const {
  std::cout << pad(depth) << "AssignmentStatement {" << std::endl;
  std::cout << pad(depth + 1) << "lhs: " << std::endl;
  lhs->print(depth + 2);
  std::cout << pad(depth + 1) << "rhs: " << std::endl;
  rhs->print(depth + 2);
  std::cout << pad(depth) << ")" << std::endl;
}

void IfStatement::print(const size_t depth) const {
  std::cout << pad(depth) << "IfStatement {" << std::endl;
  std::cout << pad(depth + 1) << "condition: " << std::endl;
  test_expression->print(depth + 2);
  std::cout << pad(depth + 1) << "true_statement: " << std::endl;
  true_statements.print(depth + 2);
  std::cout << pad(depth + 1) << "false_statement: " << std::endl;
  false_statements.print(depth + 2);
  std::cout << pad(depth) << "}" << std::endl;
}

void WhileStatement::print(const size_t depth) const {
  std::cout << pad(depth) << "WhileStatement {" << std::endl;
  std::cout << pad(depth + 1) << "condition: " << std::endl;
  test_expression->print(depth + 2);
  std::cout << pad(depth + 1) << "body: " << std::endl;
  body_statement->print(depth + 2);
  std::cout << pad(depth) << "}" << std::endl;
}

void PrintStatement::print(const size_t depth) const {
  std::cout << pad(depth) << "PrintStatement {" << std::endl;
  expression->print(depth + 1);
  std::cout << pad(depth) << "}" << std::endl;
}

void DeleteStatement::print(const size_t depth) const {
  std::cout << pad(depth) << "DeleteStatement {" << std::endl;
  expression->print(depth + 1);
  std::cout << pad(depth) << "}" << std::endl;
}
