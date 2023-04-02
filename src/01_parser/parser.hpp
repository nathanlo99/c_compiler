
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

struct ParseNode; // From parse_node.hpp

struct CFG {
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

  std::map<std::string, bool> is_non_terminal_symbol;
  std::string start_symbol;
  std::vector<Production> productions;

  std::set<std::string> nullable_symbols;

  void add_production(const std::string &product,
                      const std::vector<std::string> &ingredients);

private:
  bool definitely_nullable(const std::string &symbol) const;
  void compute_nullable();

public:
  friend std::ostream &operator<<(std::ostream &os, const CFG &cfg);
  void print() const;
};

CFG load_cfg_from_file(const std::string &filename);

struct StateItem {
  CFG::Production production;
  size_t origin_idx;
  size_t dot = 0;

  StateItem(const CFG::Production &production, const size_t origin_idx,
            const size_t dot = 0)
      : production(production), origin_idx(origin_idx), dot(dot) {}

  bool complete() const { return dot >= production.ingredients.size(); }

  std::string next_symbol() const {
    if (complete())
      return "";
    return production.ingredients[dot];
  }

  StateItem step() const {
    assert(!complete());
    return StateItem(production, origin_idx, dot + 1);
  }

  bool is_similar_to(const StateItem &other) const {
    return production == other.production && origin_idx == other.origin_idx;
  }

  friend std::ostream &operator<<(std::ostream &os, const StateItem &item);

  bool operator==(const StateItem &other) const {
    return origin_idx == other.origin_idx && dot == other.dot && production == other.production;
  }
};

struct EarleyTable {
  std::vector<std::vector<StateItem>> data;
  const std::vector<Token> &token_stream;
  const CFG &cfg;

  EarleyTable(const std::vector<Token> &token_stream, const CFG &cfg)
      : data(token_stream.size() + 1), token_stream(token_stream), cfg(cfg) {}

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
  const CFG &cfg;
  EarleyParser(const CFG &cfg) : cfg(cfg) {}

public:
  EarleyTable construct_table(const std::vector<Token> &token_stream) const;
};
