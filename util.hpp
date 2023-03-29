
#pragma once

#include <sstream>
#include <string>
#include <vector>

static void runtime_assert(const bool expr, const std::string &message) {
  if (!expr)
    throw std::runtime_error(message);
}

static void unreachable(const std::string &message) {
  runtime_assert(false, "Should be unreachable: " + message);
}

static std::vector<std::string> split(const std::string &str) {
  std::stringstream ss(str);
  std::string token;
  std::vector<std::string> result;
  while (ss >> token)
    result.push_back(token);
  return result;
}
