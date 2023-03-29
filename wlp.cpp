
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

void debug() {}

int main() {
  debug();

  try {
    const std::string input = consume_stdin();
    const std::vector<Token> token_stream = Lexer(input).token_stream();
    const CFG cfg = load_cfg_from_file("references/augmented.cfg");
    const EarleyParser parser(cfg);
    const bool valid_parse = parser.parse(token_stream);
  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
}
