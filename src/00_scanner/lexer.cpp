
#include "lexer.hpp"
#include "util.hpp"

#include <bit>
#include <cassert>

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

  if (keywords.contains(lexeme))
    last_accepting_kind = keywords.at(lexeme);
  // Range isn't checked here, only shape -- parse_literal (ast_node.cpp)
  // checks range once the value is actually needed.

  InputRange location(filename, start_location.line, start_location.column,
                      end_location.column + 1);
  return Token(lexeme, last_accepting_kind, location);
}
