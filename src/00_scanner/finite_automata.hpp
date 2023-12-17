
#pragma once

#include "token_kind.hpp"
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct DFA {
  using TransitionMap = std::array<uint64_t, 128>;
  size_t num_states = 0;
  std::vector<TokenKind> accepting_states;
  std::vector<TransitionMap> transitions;
  const static inline uint64_t ERROR_STATE = -1;

  void add_state(const TokenKind kind, const TransitionMap &state_transitions);

  friend std::ostream &operator<<(std::ostream &os, const DFA &dfa);
};

struct NFA {
  using NFAEntry = std::unordered_map<char, std::unordered_set<int>>;
  std::unordered_map<int, TokenKind> accepting_states;
  std::vector<NFAEntry> entries;

  NFA(const int num_states) : entries(num_states) {}

  void add_accepting_state(const int state, const TokenKind kind);
  void add_transitions(const int source, const int target,
                       const std::string &transitions);
  void add_transitions(const int source, const int target,
                       const std::function<bool(char)> &pred);
  void add_string(const std::string &lexeme, const TokenKind state);

  DFA to_dfa() const;

  friend std::ostream &operator<<(std::ostream &os, const NFA &nfa);
};

NFA construct_nfa();
DFA construct_dfa();
std::unordered_map<std::string, TokenKind> get_keywords();
