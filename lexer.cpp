
#include <array>
#include <bit>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

std::vector<int> get_bits(uint64_t value) {
  std::vector<int> result;
  while (value != 0) {
    result.push_back(std::countr_zero(value));
    value &= value - 1;
  }
  return result;
}

using NFAEntry = std::map<char, std::set<int>>;

enum TokenKind {
  None,
  Id,
  Num,
  Lparen,
  Rparen,
  Lbrace,
  Rbrace,
  Return,
  If,
  Else,
  While,
  Println,
  Wain,
  Becomes,
  Int,
  Eq,
  Ne,
  Lt,
  Gt,
  Le,
  Ge,
  Plus,
  Minus,
  Star,
  Slash,
  Pct,
  Comma,
  Semi,
  New,
  Delete,
  Lbrack,
  Rbrack,
  Amp,
  Null,
  Whitespace,
  Comment,
};

std::string token_kind_to_string(const TokenKind kind) {
  switch (kind) {
  case None:
    return "NONE";
  case Id:
    return "ID";
  case Num:
    return "NUM";
  case Lparen:
    return "LPAREN";
  case Rparen:
    return "RPAREN";
  case Lbrace:
    return "LBRACE";
  case Rbrace:
    return "RBRACE";
  case Return:
    return "RETURN";
  case If:
    return "IF";
  case Else:
    return "ELSE";
  case While:
    return "WHILE";
  case Println:
    return "PRINTLN";
  case Wain:
    return "WAIN";
  case Becomes:
    return "BECOMES";
  case Int:
    return "INT";
  case Eq:
    return "EQ";
  case Ne:
    return "NE";
  case Lt:
    return "LT";
  case Gt:
    return "GT";
  case Le:
    return "LE";
  case Ge:
    return "GE";
  case Plus:
    return "PLUS";
  case Minus:
    return "MINUS";
  case Star:
    return "STAR";
  case Slash:
    return "SLASH";
  case Pct:
    return "PCT";
  case Comma:
    return "COMMA";
  case Semi:
    return "SEMI";
  case New:
    return "NEW";
  case Delete:
    return "DELETE";
  case Lbrack:
    return "LBRACK";
  case Rbrack:
    return "RBRACK";
  case Amp:
    return "AMP";
  case Null:
    return "NULL";
  case Whitespace:
    return "WHITESPACE";
  case Comment:
    return "COMMENT";
  }
}

struct DFA {
  int num_states = 0;
  std::vector<TokenKind> accepting_states;
  std::vector<std::array<uint64_t, 128>> transitions;

  void add_state(const TokenKind kind,
                 const std::array<uint64_t, 128> &state_transitions) {
    num_states++;
    accepting_states.push_back(kind);
    transitions.push_back(state_transitions);
  }

  void print() const {
    std::cout << "DFA with " << num_states << " states" << std::endl;
    for (int state = 0; state < num_states; ++state) {
      const TokenKind token_kind = accepting_states[state];
      const bool is_accepting = token_kind != None;
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
};

struct NFA {
  std::map<int, TokenKind> accepting_states;
  std::vector<NFAEntry> entries;

  NFA(const int num_states) : entries(num_states) {}

  void add_accepting_state(const int state, const TokenKind kind) {
    accepting_states[state] = kind;
  }

  void add_transitions(const int source, const int target,
                       const std::string &transitions) {
    for (const char c : transitions) {
      entries[source][c].insert(target);
    }
  }

  void add_string(const std::string &lexeme, const TokenKind state) {
    int last_state = 0;
    for (const char c : lexeme) {
      const int next_state = entries.size();
      entries.emplace_back();
      entries[last_state][c].insert(next_state);
      last_state = next_state;
    }
    add_accepting_state(last_state, state);
  }

  void print() const {
    std::cout << "NFA with " << entries.size() << " states" << std::endl;
    for (int state = 0; state < entries.size(); ++state) {
      const bool is_accepting = accepting_states.count(state) > 0;
      const TokenKind token_kind =
          is_accepting ? accepting_states.at(state) : Whitespace;
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

  DFA to_dfa() const {
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
      TokenKind accepting_kind = None;
      for (int source : nfa_states) {
        const auto it = accepting_states.find(source);
        const TokenKind kind =
            (it == accepting_states.end()) ? None : it->second;
        if (kind != None) {
          assert(accepting_kind == None || kind == accepting_kind);
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
};

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
  const std::vector<std::pair<std::string, TokenKind>> keywords = []() {
    std::vector<std::pair<std::string, TokenKind>> result;
    result.emplace_back("return", Return);
    result.emplace_back("if", If);
    result.emplace_back("else", Else);
    result.emplace_back("while", While);
    result.emplace_back("println", Println);
    result.emplace_back("wain", Wain);
    result.emplace_back("int", Int);
    result.emplace_back("new", New);
    result.emplace_back("delete", Delete);
    result.emplace_back("NULL", Null);
    return result;
  }();
  const std::vector<std::pair<std::string, TokenKind>> simple_nfa_rules = []() {
    std::vector<std::pair<std::string, TokenKind>> result;
    result.emplace_back("(", Lparen);
    result.emplace_back(")", Rparen);
    result.emplace_back("{", Lbrace);
    result.emplace_back("}", Rbrace);
    result.emplace_back("=", Becomes);
    result.emplace_back("==", Eq);
    result.emplace_back("!=", Ne);
    result.emplace_back("<", Lt);
    result.emplace_back(">", Gt);
    result.emplace_back("<=", Le);
    result.emplace_back(">=", Ge);
    result.emplace_back("+", Plus);
    result.emplace_back("-", Minus);
    result.emplace_back("*", Star);
    result.emplace_back("/", Slash);
    result.emplace_back("%", Pct);
    result.emplace_back(",", Comma);
    result.emplace_back(";", Semi);
    result.emplace_back("[", Lbrack);
    result.emplace_back("]", Rbrack);
    result.emplace_back("&", Amp);
    return result;
  }();

  NFA nfa(7);
  nfa.add_accepting_state(1, Id);
  nfa.add_accepting_state(2, Num);
  nfa.add_accepting_state(3, Num);
  nfa.add_accepting_state(5, Comment);
  nfa.add_accepting_state(6, Whitespace);
  nfa.add_transitions(0, 1, letters);
  nfa.add_transitions(1, 1, alphanumeric);
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

int main() {
  const DFA dfa = construct_wlp4_dfa();
  dfa.print();
}
