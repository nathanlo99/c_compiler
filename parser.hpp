
#pragma once

#include <cassert>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "lexer.hpp"

struct CFG {
  struct Production {
    std::string product;
    std::vector<std::string> ingredients;
    Production(const std::string &target,
               const std::vector<std::string> &ingredients)
        : product(target), ingredients(ingredients) {}

    bool operator==(const Production &other) const = default;
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

  void print() const;

  bool operator==(const StateItem &other) const = default;
};

struct EarleyTable {
  std::vector<std::vector<StateItem>> data;
  const std::vector<Token> &token_stream;
  const CFG &cfg;

  EarleyTable(const std::vector<Token> &token_stream, const CFG &cfg)
      : data(), token_stream(token_stream), cfg(cfg) {
    data.reserve(token_stream.size());
  }

  void prepare(const size_t row) {
    if (row >= data.size())
      data.resize(row + 1);
  }

  void insert_unique(const size_t i, const StateItem &item);

  void complete(const size_t i, const size_t j);
  void predict(const size_t i, const size_t j, const std::string &symbol);
  void scan(const size_t i, const size_t j, const std::string &symbol);

  void print() const;
};

struct EarleyParser {
  CFG cfg;

  EarleyParser(const CFG &cfg) : cfg(cfg) {}

public:
  EarleyTable construct_table(const std::vector<Token> &token_stream) const;

  // std::shared_ptr<ASTNode> parse(const std::vector<Token> &token_stream)
  // const;
};
