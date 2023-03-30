
#include "ast_node.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "util.hpp"

#include "parse_node.hpp"

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

void debug() {}

int main() {
  // debug();
  // return 0;
  try {
    const std::string input = consume_stdin();
    const std::vector<Token> token_stream = Lexer(input).token_stream();
    const CFG cfg = load_cfg_from_file("references/productions.cfg");
    const EarleyTable table = EarleyParser(cfg).construct_table(token_stream);
    const std::shared_ptr<ParseNode> parse_tree = table.to_parse_tree();
    const std::shared_ptr<Program> ast =
        convert<Program>(parse_tree_to_ast(parse_tree));
    ast->print();
  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
}
