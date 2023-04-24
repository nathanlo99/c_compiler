
#pragma once

#include <cassert>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "lexer.hpp"
#include "util.hpp"

struct ParseNode; // From parse_node.hpp

struct ContextFreeGrammar {
  struct Production {
    std::string product;
    std::vector<std::string> ingredients;

    Production() : Production("[terminal]", {}) {}
    Production(const std::string &target,
               const std::vector<std::string> &ingredients)
        : product(target), ingredients(ingredients) {}

    std::string to_string() const;
    friend std::ostream &operator<<(std::ostream &os,
                                    const Production &production) {
      return os << production.to_string();
    }

    bool operator==(const Production &other) const {
      return product == other.product && ingredients == other.ingredients;
    }
  };

  // std::map<std::string, bool> is_non_terminal_symbol;
  std::string start_symbol;
  std::map<std::string, std::vector<Production>> productions_by_product;

  std::set<std::string> symbols;
  std::set<std::string> non_terminal_symbols;
  std::set<std::string> terminal_symbols;
  std::set<std::string> nullable_symbols;

  std::vector<Production> find_productions(const std::string &product) const {
    return productions_by_product.at(product);
  }
  std::set<std::string> get_non_terminal_symbols() const {
    return non_terminal_symbols;
  }
  std::set<std::string> get_terminal_symbols() const {
    return terminal_symbols;
  }
  bool is_non_terminal(const std::string &symbol) const {
    return non_terminal_symbols.count(symbol) > 0;
  }
  bool is_definitely_nullable(const std::string &symbol) const {
    return nullable_symbols.count(symbol) > 0;
  }

  void finalize() {
    for (const auto &[product, productions] : productions_by_product) {
      symbols.insert(product);
      non_terminal_symbols.insert(product);
      for (const auto &production : productions)
        for (const auto &ingredient : production.ingredients)
          symbols.insert(ingredient);
      for (const auto &symbol : symbols) {
        if (non_terminal_symbols.count(symbol) == 0)
          terminal_symbols.insert(symbol);
      }
    }
    compute_nullable();
  }

  void add_production(const std::string &product,
                      const std::vector<std::string> &ingredients);

private:
  bool definitely_nullable(const std::string &symbol) const;
  void compute_nullable();

public:
  friend std::ostream &operator<<(std::ostream &os,
                                  const ContextFreeGrammar &grammar);
  void print() const;
};

ContextFreeGrammar load_default_grammar();
ContextFreeGrammar load_grammar_from_file(const std::string &filename);

struct StateItem {
  ContextFreeGrammar::Production production;
  size_t origin_idx;
  size_t dot = 0;

  StateItem(const ContextFreeGrammar::Production &production,
            const size_t origin_idx, const size_t dot = 0)
      : production(production), origin_idx(origin_idx), dot(dot) {}

  bool complete() const { return dot >= production.ingredients.size(); }

  std::string next_symbol() const {
    if (complete())
      return "";
    return production.ingredients[dot];
  }

  StateItem step() const {
    debug_assert(!complete(), "StateItem::step: Item is already complete");
    return StateItem(production, origin_idx, dot + 1);
  }

  bool is_similar_to(const StateItem &other) const {
    return production == other.production && origin_idx == other.origin_idx;
  }

  friend std::ostream &operator<<(std::ostream &os, const StateItem &item);

  bool operator==(const StateItem &other) const {
    return origin_idx == other.origin_idx && dot == other.dot &&
           production == other.production;
  }
};

struct EarleyTable {
  std::vector<std::vector<StateItem>> data;
  const std::vector<Token> &token_stream;
  const ContextFreeGrammar &grammar;

  EarleyTable(const std::vector<Token> &token_stream,
              const ContextFreeGrammar &grammar)
      : data(token_stream.size() + 1), token_stream(token_stream),
        grammar(grammar) {}

  bool column_contains(const size_t i, const StateItem &item) const;
  std::optional<StateItem> find_item(const size_t start_idx,
                                     const size_t end_idx,
                                     const std::string &target) const;
  void insert_unique(const size_t i, const StateItem &item) {
    if (!column_contains(i, item))
      data[i].push_back(item);
  }

  void complete(const size_t i, const size_t j);
  void predict(const size_t i, const size_t j, const std::string &symbol);
  void scan(const size_t i, const size_t j, const std::string &symbol);

  void report_error(const size_t i) const;

  friend std::ostream &operator<<(std::ostream &os, const EarleyTable &table);

  std::shared_ptr<ParseNode>
  construct_parse_tree(const size_t start_idx, const size_t end_idx,
                       const std::string &target_symbol) const;

  std::shared_ptr<ParseNode> to_parse_tree() const;
};

struct EarleyParser {
  const ContextFreeGrammar &grammar;
  EarleyParser(const ContextFreeGrammar &grammar) : grammar(grammar) {}

public:
  EarleyTable construct_table(const std::vector<Token> &token_stream) const;
};
