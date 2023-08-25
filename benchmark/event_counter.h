#ifndef __EVENT_COUNTER_H
#define __EVENT_COUNTER_H

#ifndef SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
#ifdef __aarch64__
// on ARM, we use just cycles and instructions
#define SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS 1
#else
// elsewhere, we try to use four counters.
#define SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS 0
#endif
#endif
#include <cassert>
#include <cctype>
#ifndef _MSC_VER
#include <dirent.h>
#endif
#include <unistd.h>
#include <cinttypes>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifdef __linux__
#include "linux-perf-events.h"
#include <libgen.h>
#endif

#if __APPLE__ &&  __aarch64__
#include "apple/apple_arm_events.h"
#endif

#include "simdjson.h"

using std::string;
using std::vector;
using std::chrono::steady_clock;
using std::chrono::time_point;
using std::chrono::duration;

struct event_count {
  duration<double> elapsed;
  vector<unsigned long long> event_counts;
  event_count() : elapsed(0), event_counts{0,0,0,0,0} {}
  event_count(const duration<double> _elapsed, const vector<unsigned long long> _event_counts) : elapsed(_elapsed), event_counts(_event_counts) {}
  event_count(const event_count& other): elapsed(other.elapsed), event_counts(other.event_counts) { }

  // The types of counters (so we can read the getter more easily)
  #if SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
  enum event_counter_types {
    CPU_CYCLES,
    INSTRUCTIONS
  };
  #else
  enum event_counter_types {
    CPU_CYCLES,
    INSTRUCTIONS,
    BRANCH_MISSES,
    CACHE_REFERENCES,
    CACHE_MISSES
  };
  #endif
  double elapsed_sec() const { return duration<double>(elapsed).count(); }
  double elapsed_ns() const { return duration<double, std::nano>(elapsed).count(); }
  double cycles() const { return static_cast<double>(event_counts[CPU_CYCLES]); }
  double instructions() const { return static_cast<double>(event_counts[INSTRUCTIONS]); }
#if !SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
  double branch_misses() const { return static_cast<double>(event_counts[BRANCH_MISSES]); }
  double cache_references() const { return static_cast<double>(event_counts[CACHE_REFERENCES]); }
  double cache_misses() const { return static_cast<double>(event_counts[CACHE_MISSES]); }
#endif
  event_count& operator=(const event_count& other) {
    this->elapsed = other.elapsed;
    this->event_counts = other.event_counts;
    return *this;
  }
  event_count operator+(const event_count& other) const {
    return event_count(elapsed+other.elapsed, {
      event_counts[0]+other.event_counts[0],
      event_counts[1]+other.event_counts[1],
      event_counts[2]+other.event_counts[2],
      event_counts[3]+other.event_counts[3],
      event_counts[4]+other.event_counts[4],
    });
  }

  void operator+=(const event_count& other) {
    *this = *this + other;
  }
};

struct event_aggregate {
  int iterations = 0;
  event_count total{};
  event_count best{};
  event_count worst{};

  event_aggregate() {}

  void operator<<(const event_count& other) {
    if (iterations == 0 || other.elapsed < best.elapsed) {
      best = other;
    }
    if (iterations == 0 || other.elapsed > worst.elapsed) {
      worst = other;
    }
    iterations++;
    total += other;
  }

  double elapsed_sec() const { return total.elapsed_sec() / iterations; }
  double elapsed_ns() const { return total.elapsed_ns() / iterations; }
  double cycles() const { return total.cycles() / iterations; }
  double instructions() const { return total.instructions() / iterations; }
#if !SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
  double branch_misses() const { return total.branch_misses() / iterations; }
  double cache_references() const { return total.cache_references() / iterations; }
  double cache_misses() const { return total.cache_misses() / iterations; }
#endif
};

struct event_collector {
  event_count count{};
  time_point<steady_clock> start_clock{};

#if defined(__linux__)
  LinuxEvents<PERF_TYPE_HARDWARE> linux_events;
  event_collector() : linux_events(vector<int>{
  #if SIMDJSON_SIMPLE_PERFORMANCE_COUNTERS
    PERF_COUNT_HW_CPU_CYCLES,
    PERF_COUNT_HW_INSTRUCTIONS,
  #else
    PERF_COUNT_HW_CPU_CYCLES,
    PERF_COUNT_HW_INSTRUCTIONS,
    PERF_COUNT_HW_BRANCH_MISSES,
    PERF_COUNT_HW_CACHE_REFERENCES,
    PERF_COUNT_HW_CACHE_MISSES
  #endif
  }) {}
  bool has_events() {
    return linux_events.is_working();
  }
#elif __APPLE__ &&  __aarch64__
  AppleEvents apple_events;
  performance_counters diff;
  event_collector() : diff(0) {
    apple_events.setup_performance_counters();
  }
  bool has_events() {
    return apple_events.setup_performance_counters();
  }
#else
  event_collector() {}
  bool has_events() {
    return false;
  }
#endif

  simdjson_inline void start() {
#if defined(__linux)
    linux_events.start();
#elif __APPLE__ &&  __aarch64__
    if(has_events()) { diff = apple_events.get_counters(); }
#endif
    start_clock = steady_clock::now();
  }
  simdjson_inline event_count& end() {
    time_point<steady_clock> end_clock = steady_clock::now();
#if defined(__linux)
    linux_events.end(count.event_counts);
#elif __APPLE__ &&  __aarch64__
    if(has_events()) {
      performance_counters end = apple_events.get_counters();
      diff = end - diff;
    }
    count.event_counts[0] = diff.cycles;
    count.event_counts[1] = diff.instructions;
    count.event_counts[2] = diff.missed_branches;
    count.event_counts[3] = 0;
    count.event_counts[4] = 0;
#endif
    count.elapsed = end_clock - start_clock;
    return count;
  }
};

#endif
