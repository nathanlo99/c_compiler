
#include "lexer.hpp"
#include "util.hpp"

#include <bit>
#include <cassert>

bool is_valid_number_literal(const std::string &lexeme) {
  try {
    std::stoi(lexeme);
    return true;
  } catch (const std::exception &e) {
    return false;
  }
}

Token Lexer::get_next_token() {
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
      throw CompileError(fmt::format("{}: Unexpected character {}",
                                     char_locations[next_idx],
                                     input[next_idx]));
    } else {
      throw CompileError("Unexpected end of file");
    }
  }

  const std::string lexeme =
      input.substr(start_idx, last_accepting_idx - start_idx);
  const InputLocation start_location = char_locations[start_idx];
  const InputLocation end_location = char_locations[last_accepting_idx - 1];

  next_idx = last_accepting_idx;

  if (keywords.count(lexeme) > 0)
    last_accepting_kind = keywords.at(lexeme);
  if (last_accepting_kind == TokenKind::Num && !is_valid_number_literal(lexeme))
    throw CompileError(fmt::format("{}: numeric literal out of range ({})",
                                   start_location, lexeme));

  return Token(lexeme, last_accepting_kind, start_location, end_location);
}
