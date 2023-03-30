
#pragma once

#include <string>

enum class Type {
  Unknown,
  Int,
  IntStar,
};

inline std::string type_to_string(const Type type) {
  switch (type) {
  case Type::Int:
    return "int";
  case Type::IntStar:
    return "int*";
  default:
    return "??";
  }
}
