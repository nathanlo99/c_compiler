
#pragma once

#include "util.hpp"
#include <chrono>
#include <iostream>
#include <map>
#include <string>

class Timer {
  static inline std::map<std::string, size_t> start_times;
  static inline std::map<std::string, size_t> end_times;
  static inline std::vector<std::string> names;

public:
  static size_t get_time_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
        .count();
  }

  static inline bool is_running(const std::string &name) {
    return start_times.count(name) > 0 && end_times.count(name) == 0;
  }
  static void start(const std::string &name) {
    runtime_assert(start_times.count(name) == 0,
                   "Timer '" + name + "' already started");
    start_times[name] = get_time_ms();
    names.push_back(name);
  }
  static void stop(const std::string &name) {
    runtime_assert(start_times.count(name) > 0,
                   "Timer '" + name + "' not started");
    runtime_assert(end_times.count(name) == 0,
                   "Timer '" + name + "' already ended");
    end_times[name] = get_time_ms();
  }

  static void print(std::ostream &os) {
    os << "Timer data:" << std::endl;
    for (const auto &name : names) {
      const auto &start_time = start_times[name];
      auto it = end_times.find(name);
      if (it == end_times.end()) {
        os << "  " << name << ": (still running)" << std::endl;
      } else {
        const auto &end_time = it->second;
        os << "  " << name << ": " << (end_time - start_time) << "ms"
           << std::endl;
      }
    }
  }
};
