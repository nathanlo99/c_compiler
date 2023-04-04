
#pragma once

#include "types.hpp"

struct Literal {
  int32_t value = 0;
  Type type = Type::Int;

  constexpr Literal() = default;
  constexpr Literal(const int32_t value, const Type type)
      : value(value), type(type) {}

  void print(const size_t depth) const;
  inline std::string value_to_string() const {
    return *this == null() ? "NULL" : std::to_string(value);
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
