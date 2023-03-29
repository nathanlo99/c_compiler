
#include "parser.hpp"
#include "ast.hpp"

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

void CFG::Production::print() const {
  std::cout << product << " -> ";
  for (const auto &ingredient : ingredients) {
    std::cout << ingredient << " ";
  }
  std::cout << std::endl;
}

void CFG::print() const {
  std::cout << "CFG with " << productions.size() << " productions" << std::endl;

  for (const auto &production : productions) {
    production.print();
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
      if (!is_non_terminal || nullable_symbols.count(symbol) > 0)
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

bool EarleyTable::column_contains(const size_t i, const StateItem &item) const {
  for (const auto &existing_item : data[i]) {
    if (existing_item == item)
      return true;
  }
  return false;
}

void EarleyTable::complete(const size_t i, const size_t j) {
  const StateItem item = data[i][j];
  for (size_t k = 0; k < data[item.origin_idx].size(); ++k) {
    const StateItem old_item = data[item.origin_idx][k];
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
  for (const auto &production : cfg.productions) {
    if (production.product == cfg.start_symbol) {
      table.insert_unique(0, StateItem(production, 0));
    }
  }

  for (size_t i = 0; i <= token_stream.size(); ++i) {
    for (size_t j = 0; j < table.data[i].size(); ++j) {
      const StateItem item = table.data[i][j];
      if (item.complete()) {
        table.complete(i, j);
        continue;
      }
      const std::string next_symbol = item.next_symbol();
      const bool is_non_terminal = cfg.is_non_terminal_symbol.at(next_symbol);
      if (is_non_terminal) {
        table.predict(i, j, next_symbol);
      } else {
        table.scan(i, j, next_symbol);
      }
    }
  }
  return table;
}

StateItem EarleyTable::find_item(const size_t start_idx, const size_t end_idx,
                                 const std::string &target) const {
  assert(end_idx < data.size());
  for (const auto &item : data[end_idx]) {
    if (item.origin_idx == start_idx && item.complete() &&
        item.production.product == target)
      return item;
  }
  runtime_assert(false, "Could not find item " + target + " beginning at " +
                            std::to_string(start_idx) + " and ending at " +
                            std::to_string(end_idx));
  __builtin_unreachable();
}

std::shared_ptr<TreeNode>
EarleyTable::construct_parse_tree(const size_t start_idx, const size_t end_idx,
                                  const std::string &target_symbol) const {
  // 1. Find the production which starts at start_idx, ends at end_idx, and
  // which produces target
  const StateItem item = find_item(start_idx, end_idx, target_symbol);

  std::shared_ptr<TreeNode> result =
      std::make_shared<TreeNode>(item.production);

  assert(
      column_contains(start_idx, StateItem(item.production, item.origin_idx)));

  size_t next_idx = start_idx;
  for (size_t dot = 1; dot <= item.production.ingredients.size(); ++dot) {
    const size_t last_idx = next_idx;
    const StateItem target = StateItem(item.production, item.origin_idx, dot);

    while (next_idx <= end_idx && !column_contains(next_idx, target))
      next_idx++;
    if (next_idx > end_idx || !column_contains(next_idx, target))
      return nullptr;

    // Invariants: next_idx <= end_idx and the next_idx'th column contains
    // target

    // Since we've found target(dot-1) at last_idx and target(dot) at next_idx,
    // we know that the item at dot-1 was produced between last_idx and next_idx
    const std::string ingredient = item.production.ingredients[dot - 1];
    const bool is_non_terminal = cfg.is_non_terminal_symbol.at(ingredient);
    if (is_non_terminal) {
      const auto child_tree =
          construct_parse_tree(last_idx, next_idx, ingredient);
      result->children.push_back(child_tree);
    } else {
      const Token token = token_stream[next_idx - 1];
      const auto child_tree = std::make_shared<TreeNode>(token);
      result->children.push_back(child_tree);
    }
  }

  return result;
}

std::shared_ptr<TreeNode> EarleyTable::to_parse_tree() const {
  return construct_parse_tree(0, data.size() - 1, cfg.start_symbol);
}
