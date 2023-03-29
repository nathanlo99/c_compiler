
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "lexer.hpp"
#include "parser.hpp"

struct TreeNode {
  Token token;
  const CFG::Production production;
  std::vector<std::shared_ptr<TreeNode>> children;

  TreeNode(const Token &token) : token(token) {}
  TreeNode(const CFG::Production &production) : production(production) {}

  void print(const size_t depth = 0);

  void grab_tokens(std::vector<Token> &result) const;
  std::vector<Token> tokens() const;

  // Exists solely to debug against existing tests
  void print_cs241() const;
};
