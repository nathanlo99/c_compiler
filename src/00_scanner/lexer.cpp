
#include "lexer.hpp"
#include "util.hpp"

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

std::ostream &operator<<(std::ostream &os, const DFA &dfa) {
  os << "DFA with " << dfa.num_states << " states" << std::endl;
  for (size_t state = 0; state < dfa.num_states; ++state) {
    const TokenKind token_kind = dfa.accepting_states[state];
    const bool is_accepting = token_kind != TokenKind::None;
    const std::string token_str = token_kind_to_string(token_kind);
    os << "State " << state << ": "
       << (is_accepting ? "(accepting: " + token_str + ")" : "") << std::endl;
    for (int ch = 0; ch < 128; ++ch) {
      const uint64_t target = dfa.transitions[state].at(ch);
      if (target == DFA::ERROR_STATE)
        continue;
      os << "  '" << static_cast<char>(ch) << "' (" << ch << ") -> " << target
         << std::endl;
    }
  }
  return os;
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

void NFA::add_transitions(const int source, const int target,
                          const std::function<bool(char)> &pred) {
  for (int c = 0; c < 128; ++c) {
    const char ch = static_cast<char>(c);
    if (pred(ch)) {
      entries[source][ch].insert(target);
    }
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

std::ostream &operator<<(std::ostream &os, const NFA &nfa) {
  os << "NFA with " << nfa.entries.size() << " states" << std::endl;
  for (size_t state = 0; state < nfa.entries.size(); ++state) {
    const bool is_accepting = nfa.accepting_states.count(state) > 0;
    const TokenKind token_kind =
        is_accepting ? nfa.accepting_states.at(state) : TokenKind::Whitespace;
    const std::string token_str = token_kind_to_string(token_kind);
    const NFA::NFAEntry entry = nfa.entries[state];
    os << "State " << state << ": "
       << (is_accepting ? "(accepting: " + token_str + ")" : "") << std::endl;
    for (const auto &[ch, targets] : entry) {
      os << "  '" << ch << "' (" << static_cast<int>(ch) << ") -> {";
      for (const auto &target : targets) {
        os << target << ", ";
      }
      os << "\b\b}" << std::endl;
    }
  }
  return os;
}

DFA NFA::to_dfa() const {
  // For the current NFA, this is reasonable: it only has 35 states
  debug_assert(entries.size() <= 64, "NFA has too many states ({})",
               entries.size());
  using state_t = uint64_t;
  const state_t start_state = 1 << 0;
  DFA result;
  std::unordered_map<state_t, size_t> state_to_idx;
  state_to_idx[0] = DFA::ERROR_STATE;

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
        debug_assert(accepting_kind == TokenKind::None ||
                         kind == accepting_kind,
                     "NFA has multiple accepting states");
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
          debug_assert(nfa_target <= 64, "NFA has too many states ({})",
                       nfa_target);
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

  for (size_t state = 0; state < result.num_states; ++state) {
    for (int ch = 0; ch < 128; ++ch) {
      result.transitions[state][ch] =
          state_to_idx.at(result.transitions[state][ch]);
    }
  }

  return result;
}

NFA construct_nfa() {
  const std::string lower_alpha = "abcdefghijklmnopqrstuvwxyz";
  const std::string upper_alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const std::string non_zero_digits = "123456789";

  const std::string letters = lower_alpha + upper_alpha;
  const std::string digits = "0" + non_zero_digits;
  const std::string alphanumeric = letters + digits;
  const std::string not_new_line = []() {
    std::ostringstream result("\t\r");
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
    result.emplace_back("&&", TokenKind::Booland);
    result.emplace_back("||", TokenKind::Boolor);
    return result;
  }();

  NFA nfa(13);
  // State 0 is the start state
  // State 1 is the ID accepting state
  nfa.add_accepting_state(1, TokenKind::Id);
  // State 2 is the accepting state we reach after seeing a single digit
  nfa.add_accepting_state(2, TokenKind::Num);
  // State 3 is the accepting state we reach after seeing a non-zero digit
  nfa.add_accepting_state(3, TokenKind::Num);
  // State 4 is the state we reach after seeing a single slash
  // State 5 is the accepting state we reach after seeing two slashes
  nfa.add_accepting_state(5, TokenKind::Comment);
  // State 6 is the accepting state we reach after seeing whitespace
  nfa.add_accepting_state(6, TokenKind::Whitespace);
  // State 7 is the state we reach after seeing /*
  // State 8 is the state we reach in a multi-line comment after seeing the
  // first exiting *
  // State 9 is the state we reach in a multi-line comment after seeing the
  // ending */
  nfa.add_accepting_state(9, TokenKind::Comment);
  // State 10 is the state we reach after seeing a single 0
  // State 11 is the state we reach after seeing 0x
  // State 12 is the state we reach after seeing 0x and at least one hex digit
  nfa.add_accepting_state(12, TokenKind::Num);

  nfa.add_transitions(0, 1, letters);
  nfa.add_transitions(1, 1, alphanumeric + "_");
  nfa.add_transitions(0, 2, digits);
  nfa.add_transitions(0, 3, non_zero_digits);
  nfa.add_transitions(3, 3, digits);
  nfa.add_transitions(0, 4, "/");
  nfa.add_transitions(4, 5, "/");
  nfa.add_transitions(4, 7, "*");

  nfa.add_transitions(5, 5, [](char c) { return c != '\n'; });
  nfa.add_transitions(0, 6, "\t\n ");

  nfa.add_transitions(7, 7, [](char c) { return c != '*'; });
  nfa.add_transitions(7, 8, "*");

  nfa.add_transitions(8, 7, [](char c) { return c != '*' && c != '/'; });
  nfa.add_transitions(8, 8, "*");
  nfa.add_transitions(8, 9, "/");

  nfa.add_transitions(0, 10, "0");
  nfa.add_transitions(10, 11, "xX");
  nfa.add_transitions(11, 12, "0123456789abcdefABCDEF");
  nfa.add_transitions(12, 12, "0123456789abcdefABCDEF");

  for (const auto &[lexeme, token_kind] : simple_nfa_rules) {
    nfa.add_string(lexeme, token_kind);
  }
  return nfa;
}

DFA construct_dfa() {
  const static DFA result = construct_nfa().to_dfa();
  return result;
}

std::unordered_map<std::string, TokenKind> get_keywords() {
  static std::unordered_map<std::string, TokenKind> keywords = []() {
    std::unordered_map<std::string, TokenKind> result;
    result["return"] = TokenKind::Return;
    result["if"] = TokenKind::If;
    result["else"] = TokenKind::Else;
    result["for"] = TokenKind::For;
    result["while"] = TokenKind::While;
    result["println"] = TokenKind::Println;
    result["wain"] = TokenKind::Wain;
    result["int"] = TokenKind::Int;
    result["new"] = TokenKind::New;
    result["delete"] = TokenKind::Delete;
    result["NULL"] = TokenKind::Null;
    result["break"] = TokenKind::Break;
    result["continue"] = TokenKind::Continue;
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
  uint64_t state = 0;
  size_t last_accepting_idx = -1;
  TokenKind last_accepting_kind = TokenKind::None;
  while (next_idx < input.size()) {
    const char next_char = input[next_idx];
    state = dfa.transitions[state][next_char];
    if (state == DFA::ERROR_STATE)
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
      debug_assert(false, "Lex: Unexpected character {} at index {}",
                   input[next_idx], next_idx);
    } else {
      debug_assert(false, "Lex: Unexpected end of file");
    }
  }

  const std::string lexeme =
      input.substr(start_idx, last_accepting_idx - start_idx);
  next_idx = last_accepting_idx;

  if (keywords.count(lexeme) > 0)
    last_accepting_kind = keywords.at(lexeme);
  if (last_accepting_kind == TokenKind::Num && !is_valid_number_literal(lexeme))
    throw std::runtime_error("NUM literal out of range: " + lexeme +
                             " at index " + std::to_string(next_idx));

  const InputLocation first_char = input_chars[start_idx];
  const InputLocation last_char = input_chars[last_accepting_idx - 1];
  return Token(lexeme, last_accepting_kind, first_char.line, first_char.column,
               last_char.line, last_char.column);
}
