
#pragma once

#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

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

static std::string token_kind_to_string(const TokenKind kind) {
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
                 const std::array<uint64_t, 128> &state_transitions);

  void print() const;
};

struct NFA {
  using NFAEntry = std::map<char, std::set<int>>;
  std::map<int, TokenKind> accepting_states;
  std::vector<NFAEntry> entries;

  NFA(const int num_states) : entries(num_states) {}

  void add_accepting_state(const int state, const TokenKind kind);
  void add_transitions(const int source, const int target,
                       const std::string &transitions);
  void add_string(const std::string &lexeme, const TokenKind state);

  DFA to_dfa() const;

  void print() const;
};

NFA construct_wlp4_nfa();
DFA construct_wlp4_dfa();
std::map<std::string, TokenKind> get_wlp4_keywords();

struct Token {
  const std::string lexeme;
  const TokenKind kind;

  Token() : Token("", None) {}
  Token(const std::string &lexeme, const TokenKind &kind)
      : lexeme(lexeme), kind(kind) {}

  bool operator==(const Token &other) const = default;

  friend std::ostream &operator<<(std::ostream &os, const Token &token) {
    return os << token_kind_to_string(token.kind) << " (" << token.lexeme
              << ")";
  }
};

struct Lexer {
  const std::string input;
  size_t next_idx;
  const DFA dfa;
  const std::map<std::string, TokenKind> keywords;

  Lexer(const std::string &input)
      : input(input), next_idx(0), dfa(construct_wlp4_dfa()),
        keywords(get_wlp4_keywords()) {}

  Token next();
  bool done() { return next_idx >= input.size(); }

  std::vector<Token> token_stream() {
    std::vector<Token> result;
    while (!done()) {
      const Token next_token = next();
      if (next_token.kind != Whitespace && next_token.kind != Comment)
        result.push_back(next_token);
    }
    return result;
  }
};
