
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

  void print(const size_t depth = 0) {
    const bool is_terminal = token.kind != None;
    const std::string padding(4 * depth, ' ');
    if (is_terminal) {
      std::cout << padding << token_kind_to_string(token.kind) << " ("
                << token.lexeme << ")" << std::endl;
    } else {
      std::cout << padding;
      production.print();
    }

    for (const auto &child : children) {
      child->print(depth + 1);
    }
  }

  void grab_tokens(std::vector<Token> &result) const {
    if (token.kind != None)
      result.push_back(token);
    for (const auto &child : children)
      child->grab_tokens(result);
  }
  std::vector<Token> tokens() const {
    std::vector<Token> result;
    grab_tokens(result);
    return result;
  }
};
