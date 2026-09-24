#include <benchmark/benchmark.h>
#include "simdjson.h"

#include <string>
#include <vector>

using namespace simdjson;

enum class traversal { forward, reverse, buffered_reverse };

// Inputs are parsed outside the measured region. The buffered baseline reuses
// its allocation, measuring the cost of building handles and visiting them.
template<traversal order>
static void array_iteration(benchmark::State &state) {
  const size_t count = size_t(state.range(0));
  const int pattern = int(state.range(1));
  const char *mixed[] = {"null", "true", "1.5", "\"text\"", "-1", "18446744073709551615"};
  std::string json = "[";
  for (size_t i = 0; i < count; ++i) {
    if (i) { json += ','; }
    switch (pattern) {
      case 0: json += std::to_string(i); break;
      case 1: json += mixed[i % 6]; break;
      case 2: json += "{\"a\":[1,2,3],\"b\":{\"c\":true}}"; break;
      case 3:
        // A final nonnumeric value forces the initial marker-run scan.
        json += (i + 1 == count) ? "null" : std::to_string(uint64_t('l') << 56);
        break;
      case 4:
      case 5:
        if (pattern == 5 && i % 2 == 0) {
          json += std::to_string(uint64_t('l') << 56);
          break;
        }
        // Container boundaries can follow a long run of numeric markers.
        json += '[';
        for (size_t j = 0; j < 64; ++j) {
          if (j) { json += ','; }
          json += std::to_string(uint64_t('l') << 56);
        }
        json += ']';
        break;
      case 6: {
        // A numeric payload can point at a fake opening tag inside a sibling.
        const uint64_t child_start = 2 + (i / 2) * 134;
        if (i % 2 == 0) {
          json += '[';
          for (size_t j = 0; j < 64; ++j) {
            json += std::to_string(uint64_t('l') << 56) + ',';
          }
          json += std::to_string((uint64_t('[') << 56) | (child_start + 134));
          json += ']';
        } else {
          json += std::to_string((uint64_t(']') << 56) | (child_start + 130));
        }
        break;
      }
    }
  }
  json += ']';
  dom::parser parser;
  dom::array array;
  const padded_string padded(json);
  if (auto error = parser.parse(padded).get_array().get(array)) {
    state.SkipWithError(error_message(error));
    return;
  }
  std::vector<dom::element> buffer;
  if (order == traversal::buffered_reverse) { buffer.reserve(count); }
  for (simdjson_unused auto _ : state) {
    if (order == traversal::forward) {
      for (auto value : array) { benchmark::DoNotOptimize(value); }
    } else if (order == traversal::reverse) {
      for (auto it = array.rbegin(); it != array.rend(); ++it) {
        auto value = *it;
        benchmark::DoNotOptimize(value);
      }
    } else {
      buffer.clear();
      for (auto value : array) { buffer.push_back(value); }
      for (auto it = buffer.rbegin(); it != buffer.rend(); ++it) {
        auto value = *it;
        benchmark::DoNotOptimize(value);
      }
    }
  }
  state.SetItemsProcessed(state.iterations() * int64_t(count));
}

static void inputs(benchmark::Benchmark *benchmark) {
  for (int pattern = 0; pattern < 7; ++pattern) {
    for (int64_t size : {0, 1, 2, 4, 8, 16, 256, 4096}) {
      benchmark->Args({size, pattern});
    }
  }
  benchmark->ArgNames({"elements", "pattern"});
}

BENCHMARK_TEMPLATE(array_iteration, traversal::forward)->Apply(inputs);
BENCHMARK_TEMPLATE(array_iteration, traversal::reverse)->Apply(inputs);
BENCHMARK_TEMPLATE(array_iteration, traversal::buffered_reverse)->Apply(inputs);
BENCHMARK_MAIN();
