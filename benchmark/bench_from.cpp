// simdjson::from<T>() on root scalars, strings and flat arrays, next to the
// same work done with an On-Demand parser (iterate, then get).
//
// from/<case>     : simdjson::from(json).get(value)
// ondemand/<case> : parser.iterate(json), then doc.get(value)
//
// The late_* cases are flat arrays where the last element is unusual (a
// float in an integer array, an escape or a non-ASCII character in the last
// string). error_* cases are documents that do not convert: they measure the
// cost of failing.
//
// benchmark/from/from_benchmark covers user-defined types (Car).

#include <benchmark/benchmark.h>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <vector>
#include "simdjson.h"

#if SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_SUPPORTS_DESERIALIZATION

namespace {

template <typename T>
void bench_from(benchmark::State &state, const std::string &text, bool expect_error) {
  simdjson::padded_string json(text);
  for (auto _ : state) {
    T value{};
    auto error = simdjson::from(json).get(value);
    if (bool(error) != expect_error) {
      state.SkipWithError(error ? simdjson::error_message(error) : "unexpected success");
      return;
    }
    benchmark::DoNotOptimize(value);
  }
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(text.size()));
}

template <typename T>
void bench_ondemand(benchmark::State &state, const std::string &text, bool expect_error) {
  simdjson::padded_string json(text);
  simdjson::ondemand::parser parser;
  for (auto _ : state) {
    T value{};
    simdjson::ondemand::document doc;
    auto error = parser.iterate(json).get(doc);
    if (!error) { error = doc.get(value); }
    if (bool(error) != expect_error) {
      state.SkipWithError(error ? simdjson::error_message(error) : "unexpected success");
      return;
    }
    benchmark::DoNotOptimize(value);
  }
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(text.size()));
}

template <typename T>
void register_case(const std::string &name, std::string text, bool expect_error = false) {
  benchmark::RegisterBenchmark(("from/" + name).c_str(),
                               [text, expect_error](benchmark::State &state) {
                                 bench_from<T>(state, text, expect_error);
                               });
  benchmark::RegisterBenchmark(("ondemand/" + name).c_str(),
                               [text, expect_error](benchmark::State &state) {
                                 bench_ondemand<T>(state, text, expect_error);
                               });
}

std::string int_array(size_t n, const std::string &last) {
  std::mt19937_64 rng(1234);
  std::string s = "[";
  for (size_t i = 0; i < n; i++) {
    if (i) { s += ','; }
    if (i + 1 == n && !last.empty()) {
      s += last;
    } else {
      s += std::to_string(int64_t(rng() % 2000000) - 1000000);
    }
  }
  s += ']';
  return s;
}

std::string double_array(size_t n) {
  std::mt19937_64 rng(1234);
  std::string s = "[";
  for (size_t i = 0; i < n; i++) {
    if (i) { s += ','; }
    s += std::to_string(rng() % 100000 / 100);
    s += '.';
    s += std::to_string(10 + rng() % 90);
  }
  s += ']';
  return s;
}

std::string string_array(size_t n, size_t length, const std::string &last) {
  std::mt19937_64 rng(1234);
  std::string s = "[";
  for (size_t i = 0; i < n; i++) {
    if (i) { s += ','; }
    s += '"';
    for (size_t j = 0; j < length; j++) { s += char('a' + rng() % 26); }
    if (i + 1 == n) { s += last; }
    s += '"';
  }
  s += ']';
  return s;
}

void register_benchmarks() {
  register_case<int64_t>("int64", "-1234567");
  register_case<uint64_t>("uint64", "1234567");
  register_case<uint64_t>("uint64_above_int64_max", "18446744073709551615");
  register_case<bool>("bool", "true");
  register_case<double>("double", "3.141592653589793");
  register_case<std::string>("string/16B", "\"abcdefghijklmnop\"");
  register_case<std::string>("string/escaped", "\"abc\\\"def\\nghi\\u00e9jkl\"");
  register_case<std::string>("string/utf8", "\"caf\xc3\xa9 na\xc3\xafve \xe2\x82\xac\"");
  register_case<std::vector<int64_t>>("vector_int64/1000", int_array(1000, ""));
  register_case<std::vector<double>>("vector_double/1000", double_array(1000));
  register_case<std::vector<std::string>>("vector_string/1000x8B", string_array(1000, 8, ""));
  register_case<std::vector<std::string>>("vector_string/100x400B", string_array(100, 400, ""));
  register_case<std::vector<std::string>>("vector_string/late_escape",
                                          string_array(1000, 8, "\\n"));
  register_case<std::vector<std::string>>("vector_string/late_utf8",
                                          string_array(1000, 8, "\xc3\xa9"));
  register_case<std::vector<int64_t>>("vector_int64/error_late_float",
                                      int_array(1000, "1.5"), true);
  register_case<std::vector<int64_t>>("vector_int64/error_late_overflow",
                                      int_array(1000, "99999999999999999999"), true);
}

} // namespace

int main(int argc, char **argv) {
  register_benchmarks();
  benchmark::Initialize(&argc, argv);
  if (benchmark::ReportUnrecognizedArguments(argc, argv)) {
    return 1;
  }
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}

#else
int main() {
  printf("bench_from needs C++20 concepts.\n");
  return 0;
}
#endif
