
#include "parser.hpp"
#include "parse_node.hpp"

#include "lexer.hpp"
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
  if (productions.empty()) {
    start_symbol = "start";
    productions.emplace_back(std::string("start"),
                             std::vector<std::string>{"BOF", product, "EOF"});
    is_non_terminal_symbol["start"] = true;
    is_non_terminal_symbol["BOF"] = false;
    is_non_terminal_symbol["EOF"] = false;
  }

  productions.emplace_back(product, ingredients);
  is_non_terminal_symbol[product] = true;
  for (const std::string &ingredient : ingredients) {
    if (is_non_terminal_symbol.count(ingredient) == 0)
      is_non_terminal_symbol[ingredient] = false;
  }
}

std::string CFG::Production::to_string() const {
  std::stringstream ss;
  ss << product << " ->";
  for (const auto &ingredient : ingredients) {
    ss << " " << ingredient;
  }
  return ss.str();
}

std::ostream &operator<<(std::ostream &os, const CFG &cfg) {
  os << "CFG with " << cfg.productions.size() << " productions" << std::endl;
  for (const auto &production : cfg.productions) {
    os << production << std::endl;
  }
  return os;
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

std::ostream &operator<<(std::ostream &os, const StateItem &item) {
  os << " " << (item.complete() ? "✓" : " ") << " ";
  os << "(" << std::setw(3) << item.origin_idx << "): ";
  os << item.production.product << " -> ";
  for (size_t i = 0; i < item.production.ingredients.size(); ++i) {
    if (item.dot == i)
      os << "• ";
    os << item.production.ingredients[i] << " ";
  }
  if (item.dot == item.production.ingredients.size())
    os << "•";
  os << std::endl;
  return os;
}

std::ostream &operator<<(std::ostream &os, const EarleyTable &table) {
  os << "Earley table with " << table.data.size() << " columns" << std::endl;
  for (size_t i = 0; i < table.data.size(); ++i) {
    const Token next_token = (i == 0) ? Token() : table.token_stream[i - 1];
    os << "Column " << i << ": " << token_kind_to_string(next_token.kind) << "("
       << next_token.lexeme << ")" << std::endl;
    for (const auto &state_item : table.data[i]) {
      os << state_item << std::endl;
    }
    os << std::string(100, '-') << std::endl;
  }
  return os;
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

void EarleyTable::report_error(const size_t i) const {
  std::stringstream ss;

  // Compute the set of symbols we expected instead of token_stream[i - 1]
  std::set<std::string> expected_symbols;
  for (const auto &item : data[i - 1]) {
    if (item.complete())
      continue;
    const std::string expected = item.next_symbol();
    if (cfg.is_non_terminal_symbol.at(expected))
      continue;
    expected_symbols.insert(expected);
  }

  ss << "Parse error at " << i << " (" << token_stream[i - 1] << "): expected ";
  if (expected_symbols.empty()) {
    ss << "end of file";
  } else {
    bool first = true;
    for (const auto &expected : expected_symbols) {
      if (!first) {
        ss << ", ";
      } else {
        first = false;
      }
      ss << expected;
    }
  }

  runtime_assert(false, ss.str());
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
    if (table.data[i].size() == 0)
      table.report_error(i);

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

std::optional<StateItem>
EarleyTable::find_item(const size_t start_idx, const size_t end_idx,
                       const std::string &target) const {
  assert(end_idx < data.size());
  for (const auto &item : data[end_idx]) {
    if (item.origin_idx == start_idx && item.complete() &&
        item.production.product == target)
      return item;
  }
  return std::nullopt;
}

std::shared_ptr<ParseNode>
EarleyTable::construct_parse_tree(const size_t start_idx, const size_t end_idx,
                                  const std::string &target_symbol) const {
  // std::cout << "Constructing parse tree starting at " << start_idx
  //           << ", ending at " << end_idx << " for the target '"
  //           << target_symbol << "'" << std::endl;

  // 1. Find the production which starts at start_idx, ends at end_idx, and
  // which produces target
  const auto item = find_item(start_idx, end_idx, target_symbol);
  if (item == std::nullopt)
    return nullptr;
  // std::cout << "Found candidate item:" << std::endl;
  // item->print();

  std::shared_ptr<ParseNode> result =
      std::make_shared<ParseNode>(item->production);

  assert(column_contains(start_idx,
                         StateItem(item->production, item->origin_idx)));

  size_t next_idx = end_idx;
  for (int dot = item->production.ingredients.size() - 1; dot >= 0; --dot) {
    const size_t last_idx = next_idx;
    const StateItem target = StateItem(item->production, item->origin_idx, dot);
    const std::string ingredient = item->production.ingredients[dot];
    const bool is_non_terminal = cfg.is_non_terminal_symbol.at(ingredient);

    // std::cout << "Starting search for target with ingredient " << ingredient
    //           << std::endl;
    // std::cout << target << std::endl;

    bool added_child = false;
    for (size_t idx = last_idx; idx >= start_idx; --idx) {
      if (!column_contains(idx, target))
        continue;

      std::shared_ptr<ParseNode> child_candidate;
      if (is_non_terminal) {
        child_candidate = construct_parse_tree(idx, last_idx, ingredient);
      } else {
        const Token token = token_stream[idx];
        runtime_assert(token_kind_to_string(token.kind) == ingredient,
                       "Expected token type " + ingredient + ", got " +
                           token_kind_to_string(token.kind));
        child_candidate = std::make_shared<ParseNode>(token);
      }
      if (child_candidate == nullptr)
        continue;
      result->children.push_back(child_candidate);
      added_child = true;
      next_idx = idx;
      break;
    }

    if (!added_child)
      return nullptr;
  }

  if (next_idx != start_idx)
    return nullptr;
  std::reverse(result->children.begin(), result->children.end());
  return result;
}

std::shared_ptr<ParseNode> EarleyTable::to_parse_tree() const {
  const auto parse_tree =
      construct_parse_tree(0, data.size() - 1, cfg.start_symbol);
  runtime_assert(parse_tree->tokens() == token_stream,
                 "Bad parse: some tokens were missing");
  return parse_tree;
}
