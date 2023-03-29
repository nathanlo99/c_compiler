
#include "ast.hpp"
#include "lexer.hpp"

void TreeNode::print(const size_t depth) {
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

void TreeNode::grab_tokens(std::vector<Token> &result) const {
  if (token.kind != None)
    result.push_back(token);
  for (const auto &child : children)
    child->grab_tokens(result);
}

std::vector<Token> TreeNode::tokens() const {
  std::vector<Token> result;
  grab_tokens(result);
  return result;
}

void TreeNode::print_cs241() const {
  if (token.kind == None) {
    std::cout << production.product;
    if (production.ingredients.empty()) {
      std::cout << " .EMPTY" << std::endl;
    } else {
      for (const auto &ingredient : production.ingredients) {
        std::cout << " " << ingredient;
      }
      std::cout << std::endl;
    }
  } else {
    std::cout << token_kind_to_string(token.kind) << " " << token.lexeme
              << std::endl;
  }
  for (const auto &child : children) {
    child->print_cs241();
  }
}
