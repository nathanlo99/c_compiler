
#pragma once

#include "bril.hpp"

namespace bril {

struct CallGraph {
  std::map<std::string, std::set<std::string>> graph;
};

} // namespace bril
