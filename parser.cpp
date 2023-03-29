
#include "parser.hpp"

#include "util.hpp"

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

void EarleyParser::StateItem::print() const {
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

void print_earley_table(
    const std::vector<std::vector<EarleyParser::StateItem>> &table,
    const std::vector<Token> &token_stream) {
  std::cout << "Earley table with " << table.size() << " columns" << std::endl;
  for (size_t i = 0; i < table.size(); ++i) {
    const Token next_token = (i == 0) ? Token("", None) : token_stream[i - 1];
    std::cout << "Column " << i << ": " << token_kind_to_string(next_token.kind)
              << "(" << next_token.lexeme << ")" << std::endl;
    for (const auto &state_item : table[i]) {
      state_item.print();
    }
    std::cout << std::string(100, '-') << std::endl;
  }
}

void EarleyParser::insert_unique(EarleyTable &table, const size_t i,
                                 const StateItem &item) const {
  while (i >= table.size())
    table.emplace_back();
  for (const auto &existing_item : table[i]) {
    if (existing_item == item)
      return;
  }
  table[i].push_back(item);
}

void EarleyParser::complete(EarleyTable &table, const size_t i,
                            const size_t j) const {
  const StateItem item = table[i][j];
  for (size_t k = 0; k < table[item.origin_idx].size(); ++k) {
    const StateItem old_item = table[item.origin_idx][k];
    if (old_item.complete())
      continue;
    if (old_item.next_symbol() == item.production.product) {
      insert_unique(table, i, old_item.step());
    }
  }
}

void EarleyParser::predict(EarleyTable &table, const size_t i,
                           const std::string &symbol) const {
  for (const auto &production : cfg.productions) {
    if (production.product == symbol) {
      insert_unique(table, i, StateItem(production, i));
    }
  }
}

void EarleyParser::scan(EarleyTable &table, const size_t i, const size_t j,
                        const std::string &symbol,
                        const std::vector<Token> &token_stream) const {
  if (i >= token_stream.size())
    return;
  const StateItem item = table[i][j];
  const Token next_token = token_stream[i];
  if (token_kind_to_string(next_token.kind) == symbol) {
    insert_unique(table, i + 1, item.step());
  }
}

bool EarleyParser::parse(const std::vector<Token> &token_stream) const {
  EarleyTable table;
  table.reserve(token_stream.size());

  // Set up first column
  table.emplace_back();
  for (const auto &production : cfg.productions) {
    if (production.product == cfg.start_symbol) {
      table[0].emplace_back(production, 0);
    }
  }

  for (size_t i = 0; i <= token_stream.size(); ++i) {
    if (i >= table.size())
      table.emplace_back();
    for (size_t j = 0; j < table[i].size(); ++j) {
      const StateItem &item = table[i][j];
      if (item.dot == item.production.ingredients.size()) {
        complete(table, i, j);
      } else {
        const std::string next_symbol = item.production.ingredients[item.dot];
        const bool is_non_terminal = cfg.is_non_terminal_symbol.at(next_symbol);
        if (is_non_terminal) {
          predict(table, i, next_symbol);
        } else {
          scan(table, i, j, next_symbol, token_stream);
        }
      }
    }
  }
  print_earley_table(table, token_stream);

  return true;
}
