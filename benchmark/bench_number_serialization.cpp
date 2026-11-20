// Serialization of numbers.
//
// append/<kind>: string_builder::append of 10000 numbers, each followed by a
//   comma, into a reused builder. Floating-point values go through to_chars.
// dom_to_string/<file>: simdjson::to_string of a parsed DOM document.
//
// Filter with --benchmark_filter, for example 'append/double/.*'.

#include <benchmark/benchmark.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <string_view>
#include <vector>
#include "simdjson.h"

namespace {

using simdjson::builder::string_builder;

constexpr size_t COUNT = 10000;

template <typename T>
void append_numbers(benchmark::State &state, const std::vector<T> &values) {
  string_builder sb;
  size_t output_bytes = 0;
  for (auto _ : state) {
    sb.clear();
    for (T v : values) {
      sb.append(v);
      sb.append_comma();
    }
    std::string_view out;
    if (sb.view().get(out)) {
      state.SkipWithError("string_builder failed");
      return;
    }
    benchmark::DoNotOptimize(out.data());
    output_bytes = out.size();
  }
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(output_bytes));
  state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(values.size()));
}

std::vector<double> make_doubles(const std::string &kind) {
  std::mt19937_64 rng(1234);
  std::vector<double> out(COUNT);
  for (auto &x : out) {
    const uint64_t r = rng();
    if (kind == "prices") {
      // Two decimals, below 1000.
      x = double(r % 100000) / 100.0;
    } else if (kind == "integers") {
      // Integer-valued doubles below one million.
      x = double(r % 1000000);
    } else if (kind == "short_decimals") {
      // One to three decimals, below 100.
      const uint64_t scale = (r % 3 == 0) ? 10 : (r % 3 == 1) ? 100 : 1000;
      x = double((r >> 8) % (100 * scale)) / double(scale);
    } else if (kind == "coordinates") {
      // Uniform in [-180, 180]: almost always 15 to 17 significant digits.
      x = std::ldexp(double(r >> 11), -53) * 360.0 - 180.0;
    } else if (kind == "unit_interval") {
      // Uniform in [0, 1): leading zeros after the decimal point.
      x = std::ldexp(double(r >> 11), -53);
    } else {
      // Random finite bit patterns: any exponent, up to 17 digits.
      do {
        const uint64_t bits = rng();
        std::memcpy(&x, &bits, sizeof(x));
      } while (!std::isfinite(x));
    }
  }
  return out;
}

std::vector<int64_t> make_int64(unsigned min_digits, unsigned max_digits) {
  std::mt19937_64 rng(1234);
  std::vector<int64_t> out(COUNT);
  for (auto &x : out) {
    const unsigned digits = min_digits + unsigned(rng() % (max_digits - min_digits + 1));
    uint64_t lo = 1;
    for (unsigned i = 1; i < digits; i++) { lo *= 10; }
    const uint64_t hi = (digits >= 19) ? uint64_t(INT64_MAX) : lo * 10 - 1;
    if (digits == 1) { lo = 0; }
    const int64_t v = int64_t(lo + rng() % (hi - lo + 1));
    x = (rng() & 1) ? -v : v;
  }
  return out;
}

std::vector<uint64_t> make_uint64_full() {
  std::mt19937_64 rng(1234);
  std::vector<uint64_t> out(COUNT);
  for (auto &x : out) { x = rng() | (uint64_t(1) << 63); }
  return out;
}

void dom_to_string(benchmark::State &state, const char *file) {
  simdjson::padded_string json;
  if (simdjson::padded_string::load(std::string(SIMDJSON_BENCHMARK_DATA_DIR) + file).get(json)) {
    state.SkipWithError("could not load the file (simdjson-data missing?)");
    return;
  }
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  if (parser.parse(json).get(doc)) {
    state.SkipWithError("could not parse the file");
    return;
  }
  size_t output_bytes = 0;
  for (auto _ : state) {
    std::string out = simdjson::to_string(doc);
    benchmark::DoNotOptimize(out.data());
    output_bytes = out.size();
  }
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(output_bytes));
}

void register_benchmarks() {
  for (const char *kind : {"prices", "integers", "short_decimals", "coordinates",
                           "unit_interval", "random_bits"}) {
    benchmark::RegisterBenchmark((std::string("append/double/") + kind).c_str(),
                                 [kind](benchmark::State &state) {
                                   append_numbers(state, make_doubles(kind));
                                 });
  }
  benchmark::RegisterBenchmark("append/float/prices", [](benchmark::State &state) {
    std::vector<float> values;
    for (double d : make_doubles("prices")) { values.push_back(float(d)); }
    append_numbers(state, values);
  });
  const unsigned ranges[][2] = {{1, 2}, {3, 4}, {5, 8}, {9, 12}, {13, 19}, {1, 19}};
  for (const auto &r : ranges) {
    const unsigned lo = r[0], hi = r[1];
    benchmark::RegisterBenchmark(
        ("append/int64/" + std::to_string(lo) + "-" + std::to_string(hi) + "_digits").c_str(),
        [lo, hi](benchmark::State &state) { append_numbers(state, make_int64(lo, hi)); });
  }
  benchmark::RegisterBenchmark("append/uint64/20_digits", [](benchmark::State &state) {
    append_numbers(state, make_uint64_full());
  });
  // canada and mesh are mostly floating-point numbers, numbers.json is only
  // numbers, twitter has few.
  for (const char *file : {"canada.json", "mesh.json", "numbers.json", "random.json",
                           "twitter.json"}) {
    benchmark::RegisterBenchmark((std::string("dom_to_string/") + file).c_str(),
                                 [file](benchmark::State &state) { dom_to_string(state, file); });
  }
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
