
#include "import_bril.hpp"
#include "bril.hpp"

#include <fstream>

namespace bril {

Program import_bril(const std::string &filename) {
  std::ifstream ifs(filename);
  debug_assert(ifs.good(), "Cannot open file {}", filename);

  std::string line;
  std::stringstream tokens;
  while (std::getline(ifs, line)) {
    tokens << line << std::endl;
    // TODO: Do this later
    // std::cout << tokens.str() << std::endl;
  }

  Program program;
  return program;
}

} // namespace bril
