
#pragma once

#include "lexer.hpp"
#include "parser.hpp"

struct ParseNode {
  Token token;
  const ContextFreeGrammar::Production production;
  std::vector<std::shared_ptr<ParseNode>> children;

  ParseNode(const Token &token) : token(token) {}
  ParseNode(const ContextFreeGrammar::Production &production)
      : production(production) {}

  void print(const size_t depth = 0);

  void grab_tokens(std::vector<Token> &result) const;
  std::vector<Token> tokens() const;
};
