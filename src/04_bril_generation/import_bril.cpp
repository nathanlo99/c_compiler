
#include "import_bril.hpp"
#include "bril.hpp"

#include <ifstream>

namespace bril {

bril::Program import_bril(const std::string &filename) {
  std::ifstream ifs(filename);
  if (!ifs.good()) {
    std::cerr << "Error: cannot open file " << filename << std::endl;
    exit(1);
  }

  std::string line;
  std::stringstream tokens;
  while (std::getline(ifs, line)) {
    tokens << line << std::endl;
    // TODO: Do this later
    std::cout << tokens.str() << std::endl;
  }
}

} // namespace bril
