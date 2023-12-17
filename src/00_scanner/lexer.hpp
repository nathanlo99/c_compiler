
#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "finite_automata.hpp"
#include "token_kind.hpp"
#include "util.hpp"

struct InputLocation {
  const std::string filename;
  const size_t line;
  const size_t column;

  InputLocation(const std::string &filename = "[invalid]", size_t line = 0,
                size_t column = 0)
      : filename(filename), line(line), column(column) {}

  bool operator==(const InputLocation &other) const = default;
};

template <> struct fmt::formatter<InputLocation> : fmt::formatter<std::string> {
  auto format(const InputLocation &location, format_context &ctx) const {
    return fmt::format_to(ctx.out(), "{}:{}:{}", location.filename,
                          location.line, location.column);
  }
};

struct Token {
  const std::string lexeme;
  const TokenKind kind = TokenKind::None;
  const InputLocation start_location;
  const InputLocation end_location;

  Token() = default;
  Token(const std::string &lexeme, const TokenKind &kind,
        const InputLocation &start_location, const InputLocation &end_location)
      : lexeme(lexeme), kind(kind), start_location(start_location),
        end_location(end_location) {}

  bool operator==(const Token &other) const = default;
};

template <> struct fmt::formatter<Token> : fmt::formatter<std::string> {
  auto format(const Token &value, format_context &ctx) const {
    return fmt::format_to(ctx.out(), "{} ({}) at {} - {}", value.kind,
                          value.lexeme, value.start_location,
                          value.end_location);
  }
};

static std::string read_file(const std::string &filename) {
  if (filename == "-") {
    std::ostringstream buffer;
    buffer << std::cin.rdbuf();
    return buffer.str();
  }

  std::ifstream ifs(filename);
  if (!ifs.good())
    throw CompileError(fmt::format("{}: Cannot open file", filename));
  std::ostringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

struct Lexer {
  const std::string filename;
  const std::string input;
  const std::vector<InputLocation> char_locations;
  size_t next_idx = 0;
  const static inline DFA dfa = construct_dfa();
  const static inline std::unordered_map<std::string, TokenKind> keywords =
      get_keywords();

  Lexer(const std::string &filename)
      : filename(filename), input(read_file(filename)),
        char_locations(get_input_locations()) {}

  std::vector<InputLocation> get_input_locations() {
    std::vector<InputLocation> result;
    result.reserve(input.size());
    size_t line = 1, column = 1;
    for (char ch : input) {
      result.emplace_back(filename, line, column);
      if (ch == '\n') {
        line++;
        column = 1;
      } else {
        column++;
      }
    }
    return result;
  }

  Token get_next_token();
  bool is_done() const { return next_idx >= input.size(); }

  std::vector<Token> token_stream() {
    std::vector<Token> result;
    while (!is_done()) {
      const Token next_token = get_next_token();
      if (next_token.kind != TokenKind::Whitespace &&
          next_token.kind != TokenKind::Comment)
        result.push_back(next_token);
    }
    return result;
  }
};
