
#include "ast_node.hpp"

#include "types.hpp"
#include <iostream>
#include <map>
#include <string>
#include <utility>

std::string get_padding(const size_t indent_level) {
  return std::string(2 * indent_level, ' ');
}

void Statements::emit_c(std::ostream &os, const size_t indent_level) const {
  for (const auto &statement : statements) {
    statement->emit_c(os, indent_level);
  }
}

void Procedure::emit_c(std::ostream &os, const size_t indent_level) const {
  os << get_padding(indent_level) << "int " << name << "(";
  for (size_t i = 0; i < params.size(); ++i) {
    const auto &variable = params[i];
    os << type_to_string(variable.type) << " " << variable.name;
    if (i + 1 < params.size())
      os << ", ";
  }
  os << ") {" << std::endl;
  for (const auto &decl : decls) {
    os << get_padding(indent_level + 1) << type_to_string(decl.type) << " "
       << decl.name << " = " << decl.initial_value.value_to_string() << ";"
       << std::endl;
  }
  for (const auto &statement : statements) {
    statement->emit_c(os, indent_level + 1);
  }
  os << get_padding(indent_level + 1) << "return ";
  return_expr->emit_c(os, 0);
  os << ";" << std::endl;
  os << get_padding(indent_level) << "}" << std::endl;
}

void Program::emit_c(std::ostream &os, const size_t indent_level) const {
  for (const auto &procedure : procedures) {
    procedure.emit_c(os, indent_level);
    os << std::endl;
  }
}

void VariableLValueExpr::emit_c(std::ostream &os, const size_t) const {
  os << variable.name;
}

void DereferenceLValueExpr::emit_c(std::ostream &os, const size_t) const {
  os << "*";
  argument->emit_c(os, 0);
}

void VariableExpr::emit_c(std::ostream &os, const size_t) const {
  os << variable.name;
}

void LiteralExpr::emit_c(std::ostream &os, const size_t) const {
  os << literal.value_to_string();
}

void TestExpr::emit_c(std::ostream &os, const size_t) const {
  const std::map<ComparisonOperation, const char *> operation_to_string({
      std::make_pair(ComparisonOperation::LessThan, "<"),
      std::make_pair(ComparisonOperation::LessEqual, "<="),
      std::make_pair(ComparisonOperation::GreaterThan, ">"),
      std::make_pair(ComparisonOperation::GreaterEqual, ">="),
      std::make_pair(ComparisonOperation::Equal, "=="),
      std::make_pair(ComparisonOperation::NotEqual, "!="),
  });
  // HACK: TestExpr's will always be surrounded by brackets since they only
  // appear in IfStatements and WhileStatements, so we don't need to surround
  // them
  // os << "(";
  lhs->emit_c(os, 0);
  os << " " << operation_to_string.at(operation) << " ";
  rhs->emit_c(os, 0);
  // os << ")";
}

void BinaryExpr::emit_c(std::ostream &os, const size_t) const {
  const std::map<BinaryOperation, const char *> operation_to_string({
      std::make_pair(BinaryOperation::Add, "+"),
      std::make_pair(BinaryOperation::Sub, "-"),
      std::make_pair(BinaryOperation::Mul, "*"),
      std::make_pair(BinaryOperation::Div, "/"),
      std::make_pair(BinaryOperation::Mod, "%"),
  });
  os << "(";
  lhs->emit_c(os, 0);
  os << " " << operation_to_string.at(operation) << " ";
  rhs->emit_c(os, 0);
  os << ")";
}

void AddressOfExpr::emit_c(std::ostream &os, const size_t) const {
  os << "&";
  argument->emit_c(os, 0);
}

void DereferenceExpr::emit_c(std::ostream &os, const size_t) const {
  os << "*";
  argument->emit_c(os, 0);
}

void NewExpr::emit_c(std::ostream &os, const size_t) const {
  os << "new int[";
  rhs->emit_c(os, 0);
  os << "]";
}

void FunctionCallExpr::emit_c(std::ostream &os, const size_t) const {
  os << procedure_name << "(";
  for (size_t i = 0; i < arguments.size(); ++i) {
    arguments[i]->emit_c(os, 0);
    if (i + 1 < arguments.size())
      os << ", ";
  }
  os << ")";
}

void AssignmentStatement::emit_c(std::ostream &os,
                                 const size_t indent_level) const {
  os << get_padding(indent_level);
  lhs->emit_c(os, 0);
  os << " = ";
  rhs->emit_c(os, 0);
  os << ";" << std::endl;
}

void IfStatement::emit_c(std::ostream &os, const size_t indent_level) const {
  os << get_padding(indent_level) << "if (";
  test_expression->emit_c(os, 0);
  os << ") {" << std::endl;
  true_statement->emit_c(os, indent_level + 1);
  os << get_padding(indent_level) << "} else {" << std::endl;
  false_statement->emit_c(os, indent_level + 1);
  os << get_padding(indent_level) << "}" << std::endl;
}

void WhileStatement::emit_c(std::ostream &os, const size_t indent_level) const {
  os << get_padding(indent_level) << "while (";
  test_expression->emit_c(os, 0);
  os << ") {" << std::endl;
  body_statement->emit_c(os, indent_level + 1);
  os << get_padding(indent_level) << "}" << std::endl;
}

void PrintStatement::emit_c(std::ostream &os, const size_t indent_level) const {
  os << get_padding(indent_level) << "println(";
  expression->emit_c(os, 0);
  os << ");" << std::endl;
}

void DeleteStatement::emit_c(std::ostream &os,
                             const size_t indent_level) const {
  os << get_padding(indent_level) << "delete[] ";
  expression->emit_c(os, 0);
  os << ";" << std::endl;
}
