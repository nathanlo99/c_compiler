
#pragma once

#include "util.hpp"
#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <vector>

class Timer {
  struct TimerResult {
    std::string name;
    size_t elapsed_time;
    size_t depth;
    TimerResult(const std::string &name, const size_t depth)
        : name(name), elapsed_time(-1), depth(depth) {}
  };
  struct RunningTimer {
    std::string name;
    size_t start_time;
    size_t idx;
    RunningTimer(const std::string &name, const size_t idx)
        : name(name), start_time(get_time_ns()), idx(idx) {}
  };

  static inline std::vector<RunningTimer> running_timers;
  static inline std::vector<TimerResult> results;

  static size_t get_time_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
        .count();
  }
  static size_t get_time_ns() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(system_clock::now().time_since_epoch())
        .count();
  }

  static void push(const std::string &name) {
    running_timers.emplace_back(name, results.size());
    results.emplace_back(name, running_timers.size() - 1);
  }
  static void pop(const std::string &name) {
    debug_assert(running_timers.size() > 0, "No running timers");
    const auto &timer = running_timers.back();
    debug_assert(timer.name == name, "Timer name mismatch");
    debug_assert(results[timer.idx].elapsed_time == static_cast<size_t>(-1),
                 "Timer already stopped");
    debug_assert(results[timer.idx].name == name, "Timer name mismatch");
    results[timer.idx].elapsed_time = get_time_ns() - timer.start_time;
    running_timers.pop_back();
  }
  friend class ScopedTimer;

public:
  static void print(std::ostream &os, const double threshold_ms = 0.0) {
    os << "Timer data:" << std::endl;
    for (const auto &result : results) {
      const std::string padding(2 * result.depth, ' ');
      if (result.elapsed_time == static_cast<size_t>(-1)) {
        os << padding << result.name << ": (still running)" << std::endl;
        continue;
      }
      const double running_time_ms = result.elapsed_time / 1'000'000.0;
      if (running_time_ms >= threshold_ms)
        os << padding << result.name << ": " << running_time_ms << "ms"
           << std::endl;
    }
  }
};

class ScopedTimer {
  const std::string name;
  mutable bool stopped;

public:
  [[nodiscard]] ScopedTimer(const std::string &name)
      : name(name), stopped(false) {
    Timer::push(name);
  }
  void stop() const {
    if (!stopped) {
      Timer::pop(name);
      stopped = true;
    }
  }
  ~ScopedTimer() { stop(); }
};
