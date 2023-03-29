
#include "parser.hpp"

#include "util.hpp"

CFG load_cfg_from_file(const std::string &filename) {
  std::ifstream ifs(filename);
  std::string line;
  CFG result;
  while (std::getline(ifs, line)) {
    if (line.size() > 0 && line[0] == '#')
      continue;
    const auto tokens = split(line);
    runtime_assert(tokens.size() >= 2 && tokens[1] == "->", "Invalid CFG line");
    const std::string product = tokens[0];
    const std::vector<std::string> ingredients(tokens.begin() + 2,
                                               tokens.end());
    result.add_production(product, ingredients);
  }
  return result;
}

void CFG::add_production(const std::string &product,
                         const std::vector<std::string> &ingredients) {
  if (productions.empty())
    start_symbol = product;

  productions.emplace_back(product, ingredients);
  is_non_terminal_symbol[product] = true;
  for (const std::string &ingredient : ingredients) {
    if (is_non_terminal_symbol.count(ingredient) == 0)
      is_non_terminal_symbol[ingredient] = false;
  }
}

void CFG::print() const {
  std::cout << "CFG with " << productions.size() << " productions" << std::endl;

  for (const auto &production : productions) {
    std::cout << production.product << " -> ";
    for (const auto &ingredient : production.ingredients) {
      std::cout << ingredient << " ";
    }
    std::cout << std::endl;
  }
}

bool CFG::definitely_nullable(const std::string &symbol) const {
  if (nullable_symbols.count(symbol) > 0)
    return true;
  for (const auto &production : productions) {
    if (production.product != symbol)
      continue;
    bool all_nullable = true;
    for (const auto &ingredient : production.ingredients) {
      if (nullable_symbols.count(ingredient) == 0) {
        all_nullable = false;
        break;
      }
    }
    if (all_nullable)
      return true;
  }
  return false;
}

void CFG::compute_nullable() {
  while (true) {
    const size_t old_size = nullable_symbols.size();
    for (const auto &[symbol, is_non_terminal] : is_non_terminal_symbol) {
      if (!is_non_terminal)
        continue;
      if (nullable_symbols.count(symbol) > 0)
        continue;
      if (definitely_nullable(symbol)) {
        nullable_symbols.insert(symbol);
      }
    }
    const size_t new_size = nullable_symbols.size();
    if (old_size == new_size)
      break;
  }
}

void StateItem::print() const {
  std::cout << " " << (complete() ? "✓" : " ") << " ";
  std::cout << "(" << std::setw(3) << origin_idx << "): ";
  std::cout << production.product << " -> ";
  for (int i = 0; i < production.ingredients.size(); ++i) {
    if (dot == i)
      std::cout << "• ";
    std::cout << production.ingredients[i] << " ";
  }
  if (dot == production.ingredients.size())
    std::cout << "•";
  std::cout << std::endl;
}

void EarleyTable::print() const {
  std::cout << "Earley table with " << data.size() << " columns" << std::endl;
  for (size_t i = 0; i < data.size(); ++i) {
    const Token next_token = (i == 0) ? Token("", None) : token_stream[i - 1];
    std::cout << "Column " << i << ": " << token_kind_to_string(next_token.kind)
              << "(" << next_token.lexeme << ")" << std::endl;
    for (const auto &state_item : data[i]) {
      state_item.print();
    }
    std::cout << std::string(100, '-') << std::endl;
  }
}

void EarleyTable::insert_unique(const size_t i, const StateItem &item) {
  while (i >= data.size())
    data.emplace_back();
  for (const auto &existing_item : data[i]) {
    if (existing_item == item)
      return;
  }
  data[i].push_back(item);
}

void EarleyTable::complete(const size_t i, const size_t j) {
  const StateItem item = data[i][j];
  for (size_t k = 0; k < data[item.origin_idx].size(); ++k) {
    const StateItem old_item = data[item.origin_idx][k];
    if (old_item.complete())
      continue;
    if (old_item.next_symbol() == item.production.product) {
      insert_unique(i, old_item.step());
    }
  }
}

void EarleyTable::predict(const size_t i, const size_t j,
                          const std::string &symbol) {
  for (const auto &production : cfg.productions) {
    if (production.product == symbol) {
      insert_unique(i, StateItem(production, i));
      if (cfg.nullable_symbols.count(symbol) > 0) {
        insert_unique(i, data[i][j].step());
      }
    }
  }
}

void EarleyTable::scan(const size_t i, const size_t j,
                       const std::string &symbol) {
  if (i >= token_stream.size())
    return;
  const StateItem item = data[i][j];
  const Token next_token = token_stream[i];
  if (token_kind_to_string(next_token.kind) == symbol) {
    insert_unique(i + 1, item.step());
  }
}

EarleyTable
EarleyParser::construct_table(const std::vector<Token> &token_stream) const {
  EarleyTable table(token_stream, cfg);

  // Set up first column
  table.prepare(0);
  for (const auto &production : cfg.productions) {
    if (production.product == cfg.start_symbol) {
      table.insert_unique(0, StateItem(production, 0));
    }
  }

  for (size_t i = 0; i <= token_stream.size(); ++i) {
    table.prepare(i);
    for (size_t j = 0; j < table.data[i].size(); ++j) {
      const StateItem item = table.data[i][j];
      if (item.dot == item.production.ingredients.size()) {
        table.complete(i, j);
      } else {
        const std::string next_symbol = item.production.ingredients[item.dot];
        const bool is_non_terminal = cfg.is_non_terminal_symbol.at(next_symbol);
        if (is_non_terminal) {
          table.predict(i, j, next_symbol);
        } else {
          table.scan(i, j, next_symbol);
        }
      }
    }
  }
  return table;
}
