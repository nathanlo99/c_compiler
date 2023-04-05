
#pragma once

#include <sstream>
#include <string>
#include <vector>

[[maybe_unused]] static void runtime_assert(const bool expr,
                                            const std::string &message) {
  if (!expr)
    throw std::runtime_error(message);
}

[[maybe_unused]] static void unreachable(const std::string &message) {
  runtime_assert(false, "Should be unreachable: " + message);
  __builtin_unreachable();
}

namespace util {
[[maybe_unused]] static std::vector<std::string> split(const std::string &str) {
  std::stringstream ss(str);
  std::string token;
  std::vector<std::string> result;
  while (ss >> token)
    result.push_back(token);
  return result;
}

static inline bool contains(const std::vector<std::string> &list,
                            const std::string &str) {
  return std::find(list.begin(), list.end(), str) == list.end();
}

} // namespace util
