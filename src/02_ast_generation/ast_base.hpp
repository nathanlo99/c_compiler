
#pragma once

#include "types.hpp"
#include "util.hpp"

struct Literal {
  int64_t value = 0;
  Type type = Type::Int;

  constexpr Literal() = default;
  constexpr Literal(const int64_t value, const Type type)
      : value(value), type(type) {}

  void print(const size_t depth) const;
  inline std::string value_to_string() const {
    if (*this == null())
      return "NULL";
    else if (type == Type::Int)
      return std::to_string(value);
    else if (type == Type::IntStar)
      return fmt::format("0x{:x}", value);
    unreachable("Invalid Literal type");
    return "????";
  }

  static constexpr inline Literal null() { return Literal(1, Type::IntStar); }

  bool operator==(const Literal &other) const {
    return value == other.value && type == other.type;
  }
};

struct Variable {
  std::string name;
  Type type;
  Literal initial_value;
  Variable(const std::string &name, const Type type,
           const Literal initial_value = Literal())
      : name(name), type(type), initial_value(initial_value) {}

  void print(const size_t depth) const;

  bool operator==(const Variable &other) const {
    return type == other.type && name == other.name &&
           initial_value == other.initial_value;
  }
};
