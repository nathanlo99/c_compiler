
#pragma once

#include "lexer.hpp"
#include "parser.hpp"

struct ParseNode {
  Token token;
  const CFG::Production production;
  std::vector<std::shared_ptr<ParseNode>> children;

  ParseNode(const Token &token) : token(token) {}
  ParseNode(const CFG::Production &production) : production(production) {}

  void print(const size_t depth = 0);

  void grab_tokens(std::vector<Token> &result) const;
  std::vector<Token> tokens() const;

  // Exists solely to debug against existing tests
  void print_cs241() const;
};
