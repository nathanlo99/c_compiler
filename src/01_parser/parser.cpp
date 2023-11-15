
#include "parser.hpp"
#include "parse_node.hpp"

#include "lexer.hpp"
#include "productions.hpp"
#include "util.hpp"

#include <algorithm>
#include <iomanip>

ContextFreeGrammar load_grammar(std::stringstream &ss) {
  std::string line;
  ContextFreeGrammar result;
  while (std::getline(ss, line)) {
    const auto tokens = util::split(line);
    if (tokens.empty() || tokens[0][0] == '#')
      continue;
    debug_assert(tokens.size() >= 2 && tokens[1] == "->", "Invalid production");
    const std::string &product = tokens[0];
    const std::vector<std::string> ingredients(tokens.begin() + 2,
                                               tokens.end());
    result.add_production(product, ingredients);
  }
  result.finalize();
  return result;
}

ContextFreeGrammar load_grammar_from_file(const std::string &filename) {
  std::ifstream ifs(filename);
  std::stringstream ss;
  ss << ifs.rdbuf();
  return load_grammar(ss);
}

ContextFreeGrammar load_default_grammar() {
  std::stringstream iss(context_free_grammar);
  return load_grammar(iss);
}

void ContextFreeGrammar::add_production(
    const std::string &product, const std::vector<std::string> &ingredients) {
  if (productions_by_product.empty())
    start_symbol = product;
  productions_by_product[product].emplace_back(product, ingredients);
}

std::string ContextFreeGrammar::Production::to_string() const {
  std::stringstream ss;
  ss << product << " ->";
  for (const auto &ingredient : ingredients) {
    ss << " " << ingredient;
  }
  return ss.str();
}

std::ostream &operator<<(std::ostream &os, const ContextFreeGrammar &grammar) {
  size_t num_productions = 0;
  for (const auto &[product, productions] : grammar.productions_by_product)
    num_productions += productions.size();

  os << "Context-free grammar with " << num_productions << " productions"
     << std::endl;
  for (const auto &[product, productions] : grammar.productions_by_product)
    for (const auto &production : productions)
      os << production << std::endl;
  return os;
}

bool ContextFreeGrammar::definitely_nullable(const std::string &symbol) const {
  debug_assert(!is_definitely_nullable(symbol),
               "Redundant call to definitely_nullable: symbol {} already "
               "determined to be definitely nullable",
               symbol);
  for (const auto &production : find_productions(symbol)) {
    bool all_nullable = true;
    for (const auto &ingredient : production.ingredients) {
      if (!is_definitely_nullable(ingredient)) {
        all_nullable = false;
        break;
      }
    }
    if (all_nullable)
      return true;
  }
  return false;
}

void ContextFreeGrammar::compute_nullable() {
  while (true) {
    bool changed = false;
    for (const auto &symbol : non_terminal_symbols) {
      if (is_definitely_nullable(symbol))
        continue;
      if (definitely_nullable(symbol)) {
        nullable_symbols.insert(symbol);
        changed = true;
      }
    }
    if (!changed)
      return;
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
  return os;
}

std::ostream &operator<<(std::ostream &os, const EarleyTable &table) {
  os << "Earley table with " << table.data.size() << " columns" << std::endl;
  for (size_t i = 0; i < table.data.size(); ++i) {
    os << std::string(100, '-') << std::endl;
    const Token next_token = (i == 0) ? Token() : table.token_stream[i - 1];
    os << "Column " << i << ": " << token_kind_to_string(next_token.kind) << "("
       << next_token.lexeme << ")" << std::endl;
    for (const auto &state_item : table.data[i]) {
      os << state_item << std::endl;
    }
  }
  os << std::string(100, '-') << std::endl;
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
  // NOTE: We can't simply use a range-based for loop here because we're
  // modifying the data vector while iterating over it
  for (size_t k = 0; k < data[item.origin_idx].size(); ++k) {
    const StateItem old_item = data[item.origin_idx][k];
    if (old_item.next_symbol() == item.production.product) {
      insert_unique(i, old_item.step());
    }
  }
}

void EarleyTable::predict(const size_t i, const size_t j,
                          const std::string &symbol) {
  for (const auto &production : grammar.find_productions(symbol)) {
    insert_unique(i, StateItem(production, i));
    if (grammar.is_definitely_nullable(symbol)) {
      insert_unique(i, data[i][j].step());
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

  if (i == 0) {
    debug_assert(false, "Unexpected token of type {}",
                 token_kind_to_string(token_stream[i].kind));
    return;
  }

  // Compute the set of symbols we expected instead of token_stream[i - 1]
  std::unordered_set<std::string> expected_symbols;
  for (const auto &item : data[i - 1]) {
    if (item.complete())
      continue;
    const std::string expected = item.next_symbol();
    if (grammar.is_non_terminal(expected))
      expected_symbols.insert(expected);
  }

  std::ostringstream ss;
  ss << "Parse error at " << i << " (" << token_stream[i - 1] << "): expected ";
  if (expected_symbols.empty()) {
    ss << "end of file";
  } else {
    using util::operator<<;
    ss << expected_symbols;
  }
  const size_t num_tokens = 16;
  const size_t end_idx = std::min<int>(token_stream.size(), i + num_tokens / 2);
  const size_t begin_idx =
      std::max<int>(0, static_cast<int>(end_idx) - num_tokens);
  ss << std::endl << "Context:      ";
  for (size_t j = begin_idx; j < end_idx; ++j) {
    if (j == i - 1)
      ss << "• ";
    ss << token_stream[j].lexeme << " ";
  }
  ss << "     ";

  debug_assert(false, "{}", ss.str());
}

EarleyTable
EarleyParser::construct_table(const std::vector<Token> &token_stream) const {
  EarleyTable table(token_stream, grammar);

  // Set up first column
  for (const auto &production :
       grammar.find_productions(grammar.start_symbol)) {
    table.insert_unique(0, StateItem(production, 0));
  }

  for (size_t i = 0; i <= token_stream.size(); ++i) {
    if (table.data[i].empty())
      table.report_error(i);

    for (size_t j = 0; j < table.data[i].size(); ++j) {
      const StateItem item = table.data[i][j];
      if (item.complete()) {
        table.complete(i, j);
        continue;
      }
      const std::string next_symbol = item.next_symbol();
      const bool is_non_terminal = grammar.is_non_terminal(next_symbol);
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
  debug_assert(end_idx < data.size(),
               "EarleyTable::find_item: cannot find item since end index {} "
               "is out of bounds ({} >= {})",
               end_idx, end_idx, data.size());
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
  // 1. Find the production which starts at start_idx, ends at end_idx, and
  // which produces target
  const auto item = find_item(start_idx, end_idx, target_symbol);
  if (item == std::nullopt)
    return nullptr;

  std::shared_ptr<ParseNode> result =
      std::make_shared<ParseNode>(item->production);

  debug_assert(
      column_contains(start_idx, StateItem(item->production, item->origin_idx)),
      "Internal parse error");

  size_t next_idx = end_idx;
  for (int dot = item->production.ingredients.size() - 1; dot >= 0; --dot) {
    const size_t last_idx = next_idx;
    const StateItem target = StateItem(item->production, item->origin_idx, dot);
    const std::string ingredient = item->production.ingredients[dot];
    const bool is_non_terminal = grammar.is_non_terminal(ingredient);

    bool added_child = false;
    for (size_t idx = last_idx; idx >= start_idx; --idx) {
      if (!column_contains(idx, target))
        continue;

      std::shared_ptr<ParseNode> child_candidate;
      if (is_non_terminal) {
        child_candidate = construct_parse_tree(idx, last_idx, ingredient);
      } else {
        const Token token = token_stream[idx];
        debug_assert(token_kind_to_string(token.kind) == ingredient,
                     "Expected token type {}, got {}", ingredient,
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
  auto parse_tree =
      construct_parse_tree(0, data.size() - 1, grammar.start_symbol);
  debug_assert(parse_tree->tokens() == token_stream,
               "Bad parse: some tokens were missing");
  return parse_tree;
}
