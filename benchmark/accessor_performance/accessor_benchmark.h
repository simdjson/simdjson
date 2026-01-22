#pragma once

#include "json_benchmark/file_runner.h"
#include <string>

namespace accessor_performance {

using namespace json_benchmark;

// Test JSON for accessor benchmarks
static const char* TEST_JSON = R"({
  "name": "Alice",
  "age": 30,
  "email": "alice@example.com",
  "address": {
    "street": "123 Main St",
    "city": "Boston",
    "state": "MA",
    "zip": 12345,
    "coordinates": {
      "lat": 42.3601,
      "lon": -71.0589
    }
  },
  "scores": [95, 87, 92, 88, 91],
  "preferences": {
    "theme": "dark",
    "notifications": {
      "email": true,
      "push": false,
      "sms": true
    }
  }
})";

// Struct definitions for compile-time validation
#if SIMDJSON_STATIC_REFLECTION
struct Coordinates {
  double lat;
  double lon;
};

struct Address {
  std::string street;
  std::string city;
  std::string state;
  int64_t zip;
  Coordinates coordinates;
};

struct Notifications {
  bool email;
  bool push;
  bool sms;
};

struct Preferences {
  std::string theme;
  Notifications notifications;
};

struct TestData {
  std::string name;
  int64_t age;
  std::string email;
  Address address;
  std::vector<int64_t> scores;
  Preferences preferences;
};
#endif // SIMDJSON_STATIC_REFLECTION

// Single-access benchmark runner: measures ONE field access per iteration
template<typename I>
struct single_access_runner : public file_runner<I> {
  std::string result_string;
  int64_t result_int{};
  double result_double{};
  bool result_bool{};

  bool setup(benchmark::State &state) {
    this->json = simdjson::padded_string(TEST_JSON, strlen(TEST_JSON));
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(this->json.size()));
    return true;
  }

  bool before_run(benchmark::State &state) {
    if (!file_runner<I>::before_run(state)) { return false; }
    result_string.clear();
    result_int = 0;
    result_double = 0.0;
    result_bool = false;
    return true;
  }

  bool run(benchmark::State &) {
    return this->implementation.run(this->json, result_string, result_int, result_double, result_bool);
  }

  template<typename R>
  bool diff(benchmark::State &state, single_access_runner<R> &reference) {
    if (result_string != reference.result_string ||
        result_int != reference.result_int ||
        result_double != reference.result_double ||
        result_bool != reference.result_bool) {
      std::cerr << "Accessor benchmark results differ!" << std::endl;
      return false;
    }
    return true;
  }

  size_t items_per_iteration() {
    return 1;
  }
};

// Benchmark template definitions
struct runtime_at_path_simple;
template<typename I> simdjson_inline static void accessor_simple(benchmark::State &state) {
  run_json_benchmark<single_access_runner<I>, single_access_runner<runtime_at_path_simple>>(state);
}

struct runtime_at_path_nested;
template<typename I> simdjson_inline static void accessor_nested(benchmark::State &state) {
  run_json_benchmark<single_access_runner<I>, single_access_runner<runtime_at_path_nested>>(state);
}

struct runtime_at_path_deep;
template<typename I> simdjson_inline static void accessor_deep(benchmark::State &state) {
  run_json_benchmark<single_access_runner<I>, single_access_runner<runtime_at_path_deep>>(state);
}

} // namespace accessor_performance
