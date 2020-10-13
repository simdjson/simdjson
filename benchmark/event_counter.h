#ifndef __EVENT_COUNTER_H
#define __EVENT_COUNTER_H

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

#include "linux-perf-events.h"
#ifdef __linux__
#include <libgen.h>
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
  enum event_counter_types {
    CPU_CYCLES,
    INSTRUCTIONS,
    BRANCH_MISSES,
    CACHE_REFERENCES,
    CACHE_MISSES
  };

  double elapsed_sec() const { return duration<double>(elapsed).count(); }
  double elapsed_ns() const { return duration<double, std::nano>(elapsed).count(); }
  double cycles() const { return static_cast<double>(event_counts[CPU_CYCLES]); }
  double instructions() const { return static_cast<double>(event_counts[INSTRUCTIONS]); }
  double branch_misses() const { return static_cast<double>(event_counts[BRANCH_MISSES]); }
  double cache_references() const { return static_cast<double>(event_counts[CACHE_REFERENCES]); }
  double cache_misses() const { return static_cast<double>(event_counts[CACHE_MISSES]); }

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
  double branch_misses() const { return total.branch_misses() / iterations; }
  double cache_references() const { return total.cache_references() / iterations; }
  double cache_misses() const { return total.cache_misses() / iterations; }
};

struct event_collector {
  event_count count{};
  time_point<steady_clock> start_clock{};

#if defined(__linux__)
  LinuxEvents<PERF_TYPE_HARDWARE> linux_events;
  event_collector(bool quiet = false) : linux_events(vector<int>{
    PERF_COUNT_HW_CPU_CYCLES,
    PERF_COUNT_HW_INSTRUCTIONS,
    PERF_COUNT_HW_BRANCH_MISSES,
    PERF_COUNT_HW_CACHE_REFERENCES,
    PERF_COUNT_HW_CACHE_MISSES
  }, quiet) {}
  bool has_events() {
    return linux_events.is_working();
  }
#else
  event_collector(simdjson_unused bool _quiet = false) {}
  bool has_events() {
    return false;
  }
#endif

  simdjson_really_inline void start() {
#if defined(__linux)
    linux_events.start();
#endif
    start_clock = steady_clock::now();
  }
  simdjson_really_inline event_count& end() {
    time_point<steady_clock> end_clock = steady_clock::now();
#if defined(__linux)
    linux_events.end(count.event_counts);
#endif
    count.elapsed = end_clock - start_clock;
    return count;
  }
};

#endif
