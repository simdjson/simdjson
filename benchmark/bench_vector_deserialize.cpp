// On-Demand deserialization of arrays into std::vector with get<T>(), for
// trivial elements, strings, small and large user-defined structs, and nested
// vectors.
//
// <type>/<n>: the root array holds n elements, parsed with a reused parser
//   into a fresh vector.
// alternating/<type>: one large array, then one small array, again and
//   again, with the same parser; a library that keeps buffers between calls
//   shows its cost here. The capacity counter is the capacity of the last
//   (small) vector returned.

#include <benchmark/benchmark.h>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <vector>
#include "simdjson.h"

#if SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_SUPPORTS_DESERIALIZATION

// 16 bytes, trivially copyable.
struct point {
  double x{};
  double y{};
};

// Larger than 32 bytes, with strings, like a recipe ingredient.
struct ingredient {
  std::string name{};
  double amount{};
  std::string unit{};
};

namespace simdjson {
template <typename simdjson_value>
auto tag_invoke(deserialize_tag, simdjson_value &val, point &p) {
  ondemand::object obj;
  SIMDJSON_TRY(val.get_object().get(obj));
  SIMDJSON_TRY(obj["x"].get_double().get(p.x));
  SIMDJSON_TRY(obj["y"].get_double().get(p.y));
  return SUCCESS;
}

template <typename simdjson_value>
auto tag_invoke(deserialize_tag, simdjson_value &val, ingredient &i) {
  ondemand::object obj;
  SIMDJSON_TRY(val.get_object().get(obj));
  SIMDJSON_TRY(obj["name"].get_string(i.name));
  SIMDJSON_TRY(obj["amount"].get_double().get(i.amount));
  SIMDJSON_TRY(obj["unit"].get_string(i.unit));
  return SUCCESS;
}
} // namespace simdjson

namespace {

std::string element(const std::string &kind, size_t i, std::mt19937_64 &rng) {
  if (kind == "int64") {
    return std::to_string(int64_t(rng() % 2000000) - 1000000);
  } else if (kind == "double") {
    return std::to_string(rng() % 1000) + "." + std::to_string(10 + rng() % 90);
  } else if (kind == "string") {
    return "\"item-" + std::to_string(i) + "\"";
  } else if (kind == "point") {
    return "{\"x\":" + std::to_string(rng() % 1000) + ".5,\"y\":" +
           std::to_string(rng() % 1000) + ".25}";
  } else if (kind == "ingredient") {
    return "{\"name\":\"ingredient number " + std::to_string(i) +
           "\",\"amount\":" + std::to_string(1 + rng() % 500) +
           ".5,\"unit\":\"grams\"}";
  } else {
    // vector_int64: rows of 8 integers.
    std::string row = "[";
    for (size_t j = 0; j < 8; j++) {
      if (j) { row += ','; }
      row += std::to_string(rng() % 1000);
    }
    return row + "]";
  }
}

std::string make_array(const std::string &kind, size_t n) {
  std::mt19937_64 rng(1234);
  std::string s = "[";
  for (size_t i = 0; i < n; i++) {
    if (i) { s += ','; }
    s += element(kind, i, rng);
  }
  return s + "]";
}

template <typename T>
simdjson::error_code parse_into(simdjson::ondemand::parser &parser,
                                const simdjson::padded_string &json, std::vector<T> &out) {
  simdjson::ondemand::document doc;
  SIMDJSON_TRY(parser.iterate(json).get(doc));
  return doc.get<std::vector<T>>().get(out);
}

template <typename T>
void bench_vector(benchmark::State &state, const std::string &kind, size_t n) {
  const simdjson::padded_string json(make_array(kind, n));
  simdjson::ondemand::parser parser;
  for (auto _ : state) {
    std::vector<T> out;
    if (parse_into(parser, json, out)) {
      state.SkipWithError("deserialization failed");
      return;
    }
    benchmark::DoNotOptimize(out.data());
  }
  state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(n));
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(json.size()));
}

template <typename T>
void bench_alternating(benchmark::State &state, const std::string &kind) {
  const simdjson::padded_string large(make_array(kind, 30000));
  const simdjson::padded_string small(make_array(kind, 1));
  simdjson::ondemand::parser parser;
  size_t capacity = 0;
  for (auto _ : state) {
    std::vector<T> big_out;
    std::vector<T> small_out;
    if (parse_into(parser, large, big_out) || parse_into(parser, small, small_out)) {
      state.SkipWithError("deserialization failed");
      return;
    }
    benchmark::DoNotOptimize(big_out.data());
    benchmark::DoNotOptimize(small_out.data());
    capacity = small_out.capacity();
  }
  state.SetItemsProcessed(int64_t(state.iterations()) * 30001);
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(large.size() + small.size()));
  state.counters["capacity"] = double(capacity);
}

template <typename T>
void register_type(const std::string &kind) {
  for (size_t n : {size_t(1), size_t(16), size_t(1000), size_t(30000)}) {
    benchmark::RegisterBenchmark((kind + "/" + std::to_string(n)).c_str(),
        [kind, n](benchmark::State &state) { bench_vector<T>(state, kind, n); });
  }
  benchmark::RegisterBenchmark(("alternating/" + kind).c_str(),
      [kind](benchmark::State &state) { bench_alternating<T>(state, kind); });
}

void register_benchmarks() {
  register_type<int64_t>("int64");
  register_type<double>("double");
  register_type<std::string>("string");
  register_type<point>("point");
  register_type<ingredient>("ingredient");
  register_type<std::vector<int64_t>>("vector_int64");
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
  printf("bench_vector_deserialize needs C++20 concepts.\n");
  return 0;
}
#endif
