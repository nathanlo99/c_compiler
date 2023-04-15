
#pragma once

#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

enum class TokenKind {
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
  For,
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
  case TokenKind::None:
    return "NONE";
  case TokenKind::Id:
    return "ID";
  case TokenKind::Num:
    return "NUM";
  case TokenKind::Lparen:
    return "LPAREN";
  case TokenKind::Rparen:
    return "RPAREN";
  case TokenKind::Lbrace:
    return "LBRACE";
  case TokenKind::Rbrace:
    return "RBRACE";
  case TokenKind::Return:
    return "RETURN";
  case TokenKind::If:
    return "IF";
  case TokenKind::Else:
    return "ELSE";
  case TokenKind::For:
    return "FOR";
  case TokenKind::While:
    return "WHILE";
  case TokenKind::Println:
    return "PRINTLN";
  case TokenKind::Wain:
    return "WAIN";
  case TokenKind::Becomes:
    return "BECOMES";
  case TokenKind::Int:
    return "INT";
  case TokenKind::Eq:
    return "EQ";
  case TokenKind::Ne:
    return "NE";
  case TokenKind::Lt:
    return "LT";
  case TokenKind::Gt:
    return "GT";
  case TokenKind::Le:
    return "LE";
  case TokenKind::Ge:
    return "GE";
  case TokenKind::Plus:
    return "PLUS";
  case TokenKind::Minus:
    return "MINUS";
  case TokenKind::Star:
    return "STAR";
  case TokenKind::Slash:
    return "SLASH";
  case TokenKind::Pct:
    return "PCT";
  case TokenKind::Comma:
    return "COMMA";
  case TokenKind::Semi:
    return "SEMI";
  case TokenKind::New:
    return "NEW";
  case TokenKind::Delete:
    return "DELETE";
  case TokenKind::Lbrack:
    return "LBRACK";
  case TokenKind::Rbrack:
    return "RBRACK";
  case TokenKind::Amp:
    return "AMP";
  case TokenKind::Null:
    return "NULL";
  case TokenKind::Whitespace:
    return "WHITESPACE";
  case TokenKind::Comment:
    return "COMMENT";
  default:
    return "??";
  }
}

struct DFA {
  size_t num_states = 0;
  std::vector<TokenKind> accepting_states;
  std::vector<std::array<uint64_t, 128>> transitions;

  void add_state(const TokenKind kind,
                 const std::array<uint64_t, 128> &state_transitions);

  friend std::ostream &operator<<(std::ostream &os, const DFA &dfa);
};

struct NFA {
  using NFAEntry = std::map<char, std::set<int>>;
  std::map<int, TokenKind> accepting_states;
  std::vector<NFAEntry> entries;

  NFA(const int num_states) : entries(num_states) {}

  void add_accepting_state(const int state, const TokenKind kind);
  void add_transitions(const int source, const int target,
                       const std::string &transitions);
  void add_transitions(const int source, const int target,
                       const std::function<bool(int)> &pred);
  void add_string(const std::string &lexeme, const TokenKind state);

  DFA to_dfa() const;

  friend std::ostream &operator<<(std::ostream &os, const NFA &nfa);
};

NFA construct_wlp4_nfa();
DFA construct_wlp4_dfa();
std::map<std::string, TokenKind> get_wlp4_keywords();

struct Token {
  std::string lexeme;
  TokenKind kind;

  Token() : Token("", TokenKind::None) {}
  Token(const std::string &lexeme, const TokenKind &kind)
      : lexeme(lexeme), kind(kind) {}

  bool operator==(const Token &other) const {
    return lexeme == other.lexeme && kind == other.kind;
  }

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
      if (next_token.kind != TokenKind::Whitespace &&
          next_token.kind != TokenKind::Comment)
        result.push_back(next_token);
    }
    return result;
  }
};
