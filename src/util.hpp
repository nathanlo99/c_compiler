
#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "assert.hpp"

// [[maybe_unused]] static void debug_assert(const bool expr,
//                                           const std::string &message) {
//   if (!expr)
//     throw std::runtime_error(message);
// }

[[maybe_unused]] static void unreachable(const std::string &message) {
  debug_assert(false, "Should be unreachable: " + message);
  __builtin_unreachable();
}

namespace util {

template <typename T, typename U>
static std::ostream &operator<<(std::ostream &os, const std::pair<T, U> &pair);
template <typename T>
static std::ostream &operator<<(std::ostream &os, const std::vector<T> &list);
template <typename T>
static std::ostream &operator<<(std::ostream &os, const std::set<T> &list);
template <typename T, typename U>
static std::ostream &operator<<(std::ostream &os, const std::map<T, U> &dict);

template <typename T, typename U>
static inline std::ostream &operator<<(std::ostream &os,
                                       const std::pair<T, U> &pair) {
  return os << "(" << pair.first << ", " << pair.second << ")";
}

template <typename T>
static inline std::ostream &operator<<(std::ostream &os,
                                       const std::vector<T> &list) {
  os << "[";
  for (size_t i = 0; i < list.size(); i++) {
    os << list[i];
    if (i != list.size() - 1)
      os << ", ";
  }
  os << "]";
  return os;
}

template <typename T>
static inline std::ostream &operator<<(std::ostream &os,
                                       const std::set<T> &list) {
  os << "{";
  size_t i = 0;
  for (const auto &item : list) {
    os << item;
    if (i != list.size() - 1)
      os << ", ";
    i++;
  }
  os << "}";
  return os;
}

template <typename T, typename U>
static inline std::ostream &operator<<(std::ostream &os,
                                       const std::map<T, U> &dict) {
  bool first = true;
  os << "{";
  for (const auto &[key, value] : dict) {
    if (first)
      first = false;
    else
      os << ", ";
    os << key << ": " << value;
  }
  os << "}";
  return os;
}

inline std::vector<std::string> split(const std::string &str) {
  std::stringstream ss(str);
  std::string token;
  std::vector<std::string> result;
  while (ss >> token)
    result.push_back(token);
  return result;
}

template <typename T>
inline bool contains(const std::vector<T> &list, const T &elem) {
  return std::find(list.begin(), list.end(), elem) == list.end();
}

} // namespace util
