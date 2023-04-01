
#include "ast_node.hpp"
#include "deduce_types.hpp"
#include "fold_constants.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "populate_symbol_table.hpp"
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
  program->visit(symbol_table_visitor);

  // Deduce types of intermediate expressions
  DeduceTypesVisitor deduce_types_visitor;
  program->visit(deduce_types_visitor);
  return program;
}

void debug() {}

int main() {
  try {
    const std::string input = consume_stdin();
    const auto program0 = parse_program(input);
    const auto program = annotate_and_check_types(program0);
    const auto symbol_table = program->table;

    // Fold constants
    FoldConstantsVisitor fold_constants_visitor;
    program->visit(fold_constants_visitor);
    program->print();
    // program->emit_c(std::cout, 0);

  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
}
