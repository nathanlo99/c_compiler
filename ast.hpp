
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "lexer.hpp"
#include "parser.hpp"

struct TreeNode {
  Token token;

  CFG::Production production;
  std::vector<std::shared_ptr<TreeNode>> children;
};
