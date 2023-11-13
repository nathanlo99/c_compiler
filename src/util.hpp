
#pragma once

#include <algorithm>
#include <fmt/core.h>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// #include "assert.hpp"

#define debug_assert(expr, message, ...)                                       \
  if (!(expr)) [[unlikely]]                                                    \
    throw std::runtime_error(fmt::format("{}:{} -- ", __FILE__, __LINE__) +    \
                             fmt::format(message, ##__VA_ARGS__));

#define unreachable(message, ...)                                              \
  do {                                                                         \
    debug_assert(false, "Should be unreachable: {}", message, ##__VA_ARGS__);  \
    __builtin_unreachable();                                                   \
  } while (0)

#define log(message, ...)                                                      \
  std::clog << fmt::format("LOG: {}:{} -- ", __FILE__, __NAME__)               \
            << fmt::format(message, ##__VA_ARGS__) << std::endl;

namespace util {

template <typename T, typename U>
static std::ostream &operator<<(std::ostream &os, const std::pair<T, U> &pair);
template <typename T>
static std::ostream &operator<<(std::ostream &os, const std::vector<T> &list);
template <typename T, typename Hash>
static std::ostream &operator<<(std::ostream &os,
                                const std::set<T, Hash> &list);
template <typename T, typename U, typename Hash>
static std::ostream &operator<<(std::ostream &os,
                                const std::map<T, U, Hash> &dict);
template <typename T, typename Hash>
static std::ostream &operator<<(std::ostream &os,
                                const std::unordered_set<T, Hash> &list);
template <typename T, typename U, typename Hash>
static std::ostream &operator<<(std::ostream &os,
                                const std::unordered_map<T, U, Hash> &dict);

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

template <typename T, typename Hash>
static inline std::ostream &operator<<(std::ostream &os,
                                       const std::set<T, Hash> &list) {
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

template <typename T, typename U, typename Hash>
static inline std::ostream &operator<<(std::ostream &os,
                                       const std::map<T, U, Hash> &dict) {
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

template <typename T, typename Hash>
static inline std::ostream &
operator<<(std::ostream &os, const std::unordered_set<T, Hash> &list) {
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

template <typename T, typename U, typename Hash>
static inline std::ostream &
operator<<(std::ostream &os, const std::unordered_map<T, U, Hash> &dict) {
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
