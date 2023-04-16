
#pragma once

#include <algorithm>
#include <set>
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
static inline std::ostream &operator<<(std::ostream &os,
                                       const std::vector<std::string> &list) {
  os << "[";
  for (size_t i = 0; i < list.size(); i++) {
    os << list[i];
    if (i != list.size() - 1)
      os << ", ";
  }
  os << "]";
  return os;
}

static inline std::ostream &operator<<(std::ostream &os,
                                       const std::set<std::string> &list) {
  os << "[";
  size_t i = 0;
  for (const auto &item : list) {
    os << item;
    if (i != list.size() - 1)
      os << ", ";
    i++;
  }
  os << "]";
  return os;
}

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
