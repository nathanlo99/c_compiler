
#pragma once

#include <array>
#include <bit>
#include <fstream>
#include <functional>
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
  const static inline uint64_t ERROR_STATE = -1;

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
                       const std::function<bool(char)> &pred);
  void add_string(const std::string &lexeme, const TokenKind state);

  DFA to_dfa() const;

  friend std::ostream &operator<<(std::ostream &os, const NFA &nfa);
};

NFA construct_nfa();
DFA construct_dfa();
std::map<std::string, TokenKind> get_keywords();

struct Token {
  std::string lexeme;
  TokenKind kind;
  size_t start_line = 0, start_column = 0;
  size_t end_line = 0, end_column = 0;

  Token() : Token("", TokenKind::None, 0, 0, 0, 0) {}
  Token(const std::string &lexeme, const TokenKind &kind,
        const size_t start_line, const size_t start_column,
        const size_t end_line, const size_t end_column)
      : lexeme(lexeme), kind(kind), start_line(start_line),
        start_column(start_column), end_line(end_line), end_column(end_column) {
  }

  bool operator==(const Token &other) const {
    return lexeme == other.lexeme && kind == other.kind &&
           start_line == other.start_line &&
           start_column == other.start_column && end_line == other.end_line &&
           end_column == other.end_column;
  }

  friend std::ostream &operator<<(std::ostream &os, const Token &token) {
    return os << token_kind_to_string(token.kind) << " (" << token.lexeme
              << ") at [" << token.start_line << ":" << token.start_column
              << " - " << token.end_line << ":" << token.end_column << "]";
  }
};

struct Lexer {
  struct InputLocation {
    size_t line;
    size_t column;
  };

  const std::string input;
  const std::vector<InputLocation> input_chars;
  size_t next_idx;
  const DFA dfa;
  const std::map<std::string, TokenKind> keywords;

  Lexer(const std::string &input)
      : input(input), input_chars(index_input_string(input)), next_idx(0),
        dfa(construct_dfa()), keywords(get_keywords()) {}

  static std::vector<InputLocation>
  index_input_string(const std::string &input) {
    std::vector<InputLocation> result;
    result.reserve(input.size());
    size_t line = 1, column = 1;
    for (char ch : input) {
      result.push_back({line, column});
      if (ch == '\n') {
        line++;
        column = 1;
      } else {
        column++;
      }
    }
    return result;
  }

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
