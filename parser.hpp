
#pragma once

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

  void add_production(const std::string &product,
                      const std::vector<std::string> &ingredients);

  void print() const;
};

CFG load_cfg_from_file(const std::string &filename);

struct EarleyParser {
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
  using EarleyTable = std::vector<std::vector<StateItem>>;

  CFG cfg;

  EarleyParser(const CFG &cfg) : cfg(cfg) {}

  void insert_unique(EarleyTable &table, const size_t i,
                     const StateItem &item) const;
  void complete(EarleyTable &table, const size_t i, const size_t j) const;
  void predict(EarleyTable &table, const size_t i,
               const std::string &symbol) const;
  void scan(EarleyTable &table, const size_t i, const size_t j,
            const std::string &symbol,
            const std::vector<Token> &token_stream) const;

  bool parse(const std::vector<Token> &token_stream) const;
};
