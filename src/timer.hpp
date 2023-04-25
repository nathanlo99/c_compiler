
#pragma once

#include "util.hpp"
#include <chrono>
#include <iostream>
#include <map>
#include <string>

class Timer {
  static inline std::unordered_map<std::string, size_t> start_times;
  static inline std::unordered_map<std::string, size_t> end_times;
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
    debug_assert(start_times.count(name) == 0, "Timer '{}' already started",
                 name);
    start_times[name] = get_time_ms();
    names.push_back(name);
  }
  static void stop(const std::string &name) {
    debug_assert(start_times.count(name) > 0, "Timer '{}' not started", name);
    debug_assert(end_times.count(name) == 0, "Timer '{}' not started", name);
    end_times[name] = get_time_ms();
  }

  static void print(std::ostream &os, const size_t threshold_ms = 0) {
    os << "Timer data:" << std::endl;
    for (const auto &name : names) {
      const auto &start_time = start_times[name];
      auto it = end_times.find(name);
      if (it == end_times.end()) {
        os << "  " << name << ": (still running)" << std::endl;
      } else {
        const auto &end_time = it->second;
        const size_t running_time_ms = end_time - start_time;
        if (running_time_ms > threshold_ms)
          os << "  " << name << ": " << running_time_ms << "ms" << std::endl;
      }
    }
  }
};
