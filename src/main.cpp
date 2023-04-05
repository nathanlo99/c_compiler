
#include "ast_node.hpp"
#include "deduce_types.hpp"
#include "fold_constants.hpp"
#include "lexer.hpp"
#include "naive_mips_generator.hpp"
#include "parser.hpp"
#include "populate_symbol_table.hpp"
#include "simple_bril_generator.hpp"
#include "symbol_table.hpp"
#include "util.hpp"

#include "parse_node.hpp"
#include <memory>

std::string read_file(const std::string &filename) {
  std::ifstream ifs(filename);
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

std::string consume_stdin() {
  std::stringstream buffer;
  buffer << std::cin.rdbuf();
  return buffer.str();
}

std::shared_ptr<Program> parse_program(const std::string &input) {
  const std::vector<Token> token_stream = Lexer(input).token_stream();
  const CFG cfg = load_cfg_from_file("references/productions.cfg");
  const EarleyTable table = EarleyParser(cfg).construct_table(token_stream);
  const std::shared_ptr<ParseNode> parse_tree = table.to_parse_tree();
  const std::shared_ptr<Program> program = construct_ast<Program>(parse_tree);
  return program;
}

std::shared_ptr<Program>
annotate_and_check_types(std::shared_ptr<Program> program) {
  // Populate symbol table
  PopulateSymbolTableVisitor symbol_table_visitor;
  program->accept_recursive(symbol_table_visitor);

  // Deduce types of intermediate expressions
  DeduceTypesVisitor deduce_types_visitor;
  program->accept_recursive(deduce_types_visitor);
  return program;
}

void debug() {
  const std::string input = "0 - (b - c)";
  const std::vector<Variable> variables = {
      Variable("a", Type::Int),
      Variable("b", Type::IntStar),
      Variable("c", Type::IntStar),
  };

  const std::vector<Token> token_stream = Lexer(input).token_stream();
  const CFG cfg = load_cfg_from_file("tests/arithmetic.cfg");
  const EarleyTable table = EarleyParser(cfg).construct_table(token_stream);
  const std::shared_ptr<ParseNode> parse_tree = table.to_parse_tree();
  const std::shared_ptr<Expr> expr = construct_ast<Expr>(parse_tree);

  SymbolTable mocked_table;
  mocked_table.add_procedure("MOCK");
  mocked_table.enter_procedure("MOCK");
  for (const Variable &variable : variables) {
    mocked_table.add_variable("MOCK", variable);
  }

  DeduceTypesVisitor deduce_types_visitor(mocked_table);
  expr->accept_recursive(deduce_types_visitor);

  const std::shared_ptr<Expr> simplified_expr = fold_constants(expr);

  simplified_expr->print(0);
}

void emit_bril(std::shared_ptr<Program> program) {
  bril::SimpleBRILGenerator generator;
  program->accept_simple(generator);
  const bril::Program bril_program = generator.program();

  std::cout << bril_program << std::endl;
}

int main() {
  // debug();
  // return 0;

  try {
    const std::string input = consume_stdin();
    const auto program0 = parse_program(input);
    const auto program = annotate_and_check_types(program0);
    const auto symbol_table = program->table;

    // std::cerr << symbol_table << std::endl;

    // Fold constants
    FoldConstantsVisitor fold_constants_visitor;
    program->accept_recursive(fold_constants_visitor);

    // program->print();
    program->emit_c(std::cerr, 0);

    emit_bril(program);
    // NaiveMIPSGenerator generator;
    // program->accept_simple(generator);
    // generator.print();

  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
}
