#pragma once

#include "util.hpp"
#include <string>

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
  Booland,
  Boolor,
  Break,
  Continue,
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
  case TokenKind::Booland:
    return "BOOLAND";
  case TokenKind::Boolor:
    return "BOOLOR";
  case TokenKind::Break:
    return "BREAK";
  case TokenKind::Continue:
    return "CONTINUE";
  case TokenKind::Whitespace:
    return "WHITESPACE";
  case TokenKind::Comment:
    return "COMMENT";
  }
}

template <> struct fmt::formatter<TokenKind> : fmt::formatter<std::string> {
  auto format(const TokenKind &value, format_context &ctx) const {
    return fmt::format_to(ctx.out(), "{}", token_kind_to_string(value));
  }
};
