
#include "lexer.hpp"

#include <bit>
#include <cassert>

std::vector<int> get_bits(uint64_t value) {
  std::vector<int> result;
  while (value != 0) {
    result.push_back(std::countr_zero(value));
    value &= value - 1;
  }
  return result;
}

void DFA::add_state(const TokenKind kind,
                    const std::array<uint64_t, 128> &state_transitions) {
  num_states++;
  accepting_states.push_back(kind);
  transitions.push_back(state_transitions);
}

void DFA::print() const {
  std::cout << "DFA with " << num_states << " states" << std::endl;
  for (int state = 0; state < num_states; ++state) {
    const TokenKind token_kind = accepting_states[state];
    const bool is_accepting = token_kind != TokenKind::None;
    const std::string token_str = token_kind_to_string(token_kind);
    std::cout << "State " << state << ": "
              << (is_accepting ? "(accepting: " + token_str + ")" : "")
              << std::endl;
    for (int ch = 0; ch < 128; ++ch) {
      const int target = transitions[state].at(ch);
      if (target == -1)
        continue;
      std::cout << "  '" << static_cast<char>(ch) << "' (" << ch << ") -> "
                << target << std::endl;
    }
  }
}

void NFA::add_accepting_state(const int state, const TokenKind kind) {
  accepting_states[state] = kind;
}

void NFA::add_transitions(const int source, const int target,
                          const std::string &transitions) {
  for (const char c : transitions) {
    entries[source][c].insert(target);
  }
}

void NFA::add_string(const std::string &lexeme, const TokenKind state) {
  int last_state = 0;
  for (const char c : lexeme) {
    const int next_state = entries.size();
    entries.emplace_back();
    entries[last_state][c].insert(next_state);
    last_state = next_state;
  }
  add_accepting_state(last_state, state);
}

void NFA::print() const {
  std::cout << "NFA with " << entries.size() << " states" << std::endl;
  for (size_t state = 0; state < entries.size(); ++state) {
    const bool is_accepting = accepting_states.count(state) > 0;
    const TokenKind token_kind =
        is_accepting ? accepting_states.at(state) : TokenKind::Whitespace;
    const std::string token_str = token_kind_to_string(token_kind);
    const NFAEntry entry = entries[state];
    std::cout << "State " << state << ": "
              << (is_accepting ? "(accepting: " + token_str + ")" : "")
              << std::endl;
    for (const auto &[ch, targets] : entry) {
      std::cout << "  '" << ch << "' (" << static_cast<int>(ch) << ") -> {";
      for (const auto &target : targets) {
        std::cout << target << ", ";
      }
      std::cout << "\b\b}" << std::endl;
    }
  }
}

DFA NFA::to_dfa() const {
  // For WLP, this is reasonable: the current NFA only has 30 states
  assert(entries.size() <= 64);
  using state_t = uint64_t;
  const state_t start_state = 1 << 0;
  DFA result;
  std::map<state_t, int> state_to_idx;
  state_to_idx[0] = -1;

  std::queue<state_t> active_nodes;
  active_nodes.push(start_state);

  while (!active_nodes.empty()) {
    const state_t state = active_nodes.front();
    active_nodes.pop();
    if (state_to_idx.count(state) > 0)
      continue;
    const std::vector<int> nfa_states = get_bits(state);

    // Compute accepting state, if one exists
    TokenKind accepting_kind = TokenKind::None;
    for (int source : nfa_states) {
      const auto it = accepting_states.find(source);
      const TokenKind kind =
          (it == accepting_states.end()) ? TokenKind::None : it->second;
      if (kind != TokenKind::None) {
        assert(accepting_kind == TokenKind::None || kind == accepting_kind);
        accepting_kind = kind;
      }
    }

    // Compute transitions
    std::array<state_t, 128> transitions;
    for (int ch = 0; ch < 128; ++ch) {
      state_t dfa_target = 0;
      for (int source : nfa_states) {
        if (entries[source].count(ch) == 0)
          continue;
        for (int nfa_target : entries[source].at(ch)) {
          assert(nfa_target <= 64);
          dfa_target |= (1ULL << nfa_target);
        }
      }
      active_nodes.push(dfa_target);
      transitions[ch] = dfa_target;
    }
    const int idx = result.num_states;
    result.add_state(accepting_kind, transitions);
    state_to_idx[state] = idx;
  }

  for (int state = 0; state < result.num_states; ++state) {
    for (int ch = 0; ch < 128; ++ch) {
      result.transitions[state][ch] =
          state_to_idx.at(result.transitions[state][ch]);
    }
  }

  return result;
}

NFA construct_wlp4_nfa() {
  const std::string lower_alpha = "abcdefghijklmnopqrstuvwxyz";
  const std::string upper_alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const std::string non_zero_digits = "123456789";

  const std::string letters = lower_alpha + upper_alpha;
  const std::string digits = "0" + non_zero_digits;
  const std::string alphanumeric = letters + digits;
  const std::string not_new_line = []() {
    std::stringstream result("\t\r");
    for (char k = 32; k > 0; ++k)
      result << k;
    return result.str();
  }();
  const std::vector<std::pair<std::string, TokenKind>> simple_nfa_rules = []() {
    std::vector<std::pair<std::string, TokenKind>> result;
    result.emplace_back("(", TokenKind::Lparen);
    result.emplace_back(")", TokenKind::Rparen);
    result.emplace_back("{", TokenKind::Lbrace);
    result.emplace_back("}", TokenKind::Rbrace);
    result.emplace_back("=", TokenKind::Becomes);
    result.emplace_back("==", TokenKind::Eq);
    result.emplace_back("!=", TokenKind::Ne);
    result.emplace_back("<", TokenKind::Lt);
    result.emplace_back(">", TokenKind::Gt);
    result.emplace_back("<=", TokenKind::Le);
    result.emplace_back(">=", TokenKind::Ge);
    result.emplace_back("+", TokenKind::Plus);
    result.emplace_back("-", TokenKind::Minus);
    result.emplace_back("*", TokenKind::Star);
    result.emplace_back("/", TokenKind::Slash);
    result.emplace_back("%", TokenKind::Pct);
    result.emplace_back(",", TokenKind::Comma);
    result.emplace_back(";", TokenKind::Semi);
    result.emplace_back("[", TokenKind::Lbrack);
    result.emplace_back("]", TokenKind::Rbrack);
    result.emplace_back("&", TokenKind::Amp);
    return result;
  }();

  NFA nfa(7);
  nfa.add_accepting_state(1, TokenKind::Id);
  nfa.add_accepting_state(2, TokenKind::Num);
  nfa.add_accepting_state(3, TokenKind::Num);
  nfa.add_accepting_state(5, TokenKind::Comment);
  nfa.add_accepting_state(6, TokenKind::Whitespace);
  nfa.add_transitions(0, 1, letters);
  nfa.add_transitions(1, 1, alphanumeric + "_");
  nfa.add_transitions(0, 2, digits);
  nfa.add_transitions(0, 3, non_zero_digits);
  nfa.add_transitions(3, 3, digits);
  nfa.add_transitions(0, 4, "/");
  nfa.add_transitions(4, 5, "/");
  nfa.add_transitions(5, 5, not_new_line);
  nfa.add_transitions(0, 6, "\t\n ");
  for (const auto &[lexeme, token_kind] : simple_nfa_rules) {
    nfa.add_string(lexeme, token_kind);
  }
  return nfa;
}

DFA construct_wlp4_dfa() {
  const static DFA result = construct_wlp4_nfa().to_dfa();
  return result;
}

std::map<std::string, TokenKind> get_wlp4_keywords() {
  const std::map<std::string, TokenKind> keywords = []() {
    std::map<std::string, TokenKind> result;
    result["return"] = TokenKind::Return;
    result["if"] = TokenKind::If;
    result["else"] = TokenKind::Else;
    result["while"] = TokenKind::While;
    result["println"] = TokenKind::Println;
    result["wain"] = TokenKind::Wain;
    result["int"] = TokenKind::Int;
    result["new"] = TokenKind::New;
    result["delete"] = TokenKind::Delete;
    result["NULL"] = TokenKind::Null;
    return result;
  }();
  return keywords;
}

bool is_valid_number_literal(const std::string &lexeme) {
  try {
    std::stoi(lexeme);
    return true;
  } catch (const std::exception &e) {
    return false;
  }
}

Token Lexer::next() {
  const size_t start_idx = next_idx;
  int state = 0;
  size_t last_accepting_idx = -1;
  TokenKind last_accepting_kind = TokenKind::None;
  while (next_idx < input.size()) {
    const char next_char = input[next_idx];
    state = dfa.transitions[state][next_char];
    if (state == -1)
      break;
    const TokenKind accepting = dfa.accepting_states[state];
    if (accepting != TokenKind::None) {
      last_accepting_idx = next_idx + 1;
      last_accepting_kind = accepting;
    }
    next_idx++;
  }
  if (last_accepting_kind == TokenKind::None) {
    if (next_idx < input.size()) {
      throw std::runtime_error(std::string("Unexpected character ") +
                               input[next_idx] + " at index " +
                               std::to_string(next_idx));
    } else {
      throw std::runtime_error("Unexpected end of file");
    }
  }

  const std::string lexeme =
      input.substr(start_idx, last_accepting_idx - start_idx);
  next_idx = last_accepting_idx;

  if (keywords.count(lexeme) > 0)
    last_accepting_kind = keywords.at(lexeme);
  if (last_accepting_kind == TokenKind::Num && !is_valid_number_literal(lexeme))
    throw std::runtime_error(std::string("NUM literal out of range: ") +
                             lexeme + " at index " + std::to_string(next_idx));

  return Token(lexeme, last_accepting_kind);
}
