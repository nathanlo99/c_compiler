
#include "lexer.hpp"
#include "parser.hpp"

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

void debug() {
  const CFG cfg = load_cfg_from_file("tests/simple.cfg");
  const EarleyParser parser(cfg);
  const std::vector<Token> token_stream = Lexer("1+2").token_stream();
  const EarleyTable table = parser.construct_table(token_stream);
  table.print();
}

int main() {
  debug();
  return 0;
  try {
    const std::string input = consume_stdin();
    const std::vector<Token> token_stream = Lexer(input).token_stream();
    const CFG cfg = load_cfg_from_file("references/productions.cfg");
    const EarleyParser parser(cfg);
    const EarleyTable table = parser.construct_table(token_stream);
    table.print();
  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
}
