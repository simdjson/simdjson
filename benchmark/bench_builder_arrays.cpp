// Serialization of contiguous arrays (integers, floating-point values and
// strings) with string_builder::append and simdjson::to_json.
//
// Each case runs four ways:
//   builder_reuse : clear() and append() into one builder,
//   builder_fresh : a new builder, append(), then move_to() a std::string,
//   to_json_fresh : simdjson::to_json(values), a new std::string each time,
//   to_json_reuse : simdjson::to_json(values, out) into one std::string.
// The output_bytes and capacity counters report the size of the JSON and the
// capacity of the result that the caller keeps.
//
// Filter with --benchmark_filter, for example 'int64/mixed/.*/1000$'.

#include <benchmark/benchmark.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include "simdjson.h"

#if SIMDJSON_SUPPORTS_RANGES && SIMDJSON_SUPPORTS_CONCEPTS

namespace {

using simdjson::builder::string_builder;

enum class mode { builder_reuse, builder_fresh, to_json_fresh, to_json_reuse };

const char *mode_name(mode m) {
  switch (m) {
  case mode::builder_reuse: return "builder_reuse";
  case mode::builder_fresh: return "builder_fresh";
  case mode::to_json_fresh: return "to_json_fresh";
  case mode::to_json_reuse: return "to_json_reuse";
  }
  return "";
}

template <class C>
void report(benchmark::State &state, const C &values, size_t output_bytes,
            size_t capacity) {
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(output_bytes));
  state.SetItemsProcessed(int64_t(state.iterations()) * int64_t(values.size()));
  state.counters["output_bytes"] = double(output_bytes);
  state.counters["capacity"] = double(capacity);
}

template <mode M, class C>
void serialize(benchmark::State &state, const C &values) {
  size_t output_bytes = 0;
  size_t capacity = 0;
  if constexpr (M == mode::builder_reuse) {
    string_builder sb;
    for (auto _ : state) {
      sb.clear();
      sb.append(values);
      std::string_view out;
      if (sb.view().get(out)) {
        state.SkipWithError("string_builder failed");
        return;
      }
      benchmark::DoNotOptimize(out.data());
      output_bytes = out.size();
    }
    capacity = sb.unsafe_capacity();
  } else if constexpr (M == mode::builder_fresh) {
    for (auto _ : state) {
      string_builder sb;
      sb.append(values);
      std::string out;
      if (sb.move_to(out)) {
        state.SkipWithError("string_builder failed");
        return;
      }
      benchmark::DoNotOptimize(out.data());
      output_bytes = out.size();
      capacity = out.capacity();
    }
  } else if constexpr (M == mode::to_json_fresh) {
    for (auto _ : state) {
      std::string out;
      if (simdjson::to_json(values).get(out)) {
        state.SkipWithError("to_json failed");
        return;
      }
      benchmark::DoNotOptimize(out.data());
      output_bytes = out.size();
      capacity = out.capacity();
    }
  } else {
    std::string out;
    for (auto _ : state) {
      if (simdjson::to_json(values, out)) {
        state.SkipWithError("to_json failed");
        return;
      }
      benchmark::DoNotOptimize(out.data());
      output_bytes = out.size();
    }
    capacity = out.capacity();
  }
  report(state, values, output_bytes, capacity);
}

// small: 0..99. mixed: every bit length is equally likely, so the number of
// digits varies from one value to the next.
enum class int_range { small, mixed };

template <typename T>
std::vector<T> make_integers(size_t n, int_range r) {
  std::mt19937_64 rng(1234);
  std::vector<T> out(n);
  for (auto &x : out) {
    const uint64_t bits = rng();
    if (r == int_range::small) {
      x = T(bits % 100);
    } else {
      const unsigned width = unsigned(rng() % (8 * sizeof(T))) + 1;
      uint64_t v = width >= 64 ? bits : bits & ((uint64_t(1) << width) - 1);
      if (std::is_signed_v<T>) {
        v >>= 1;
        x = (bits & 1) ? T(-T(v)) : T(v);
      } else {
        x = T(v);
      }
    }
  }
  return out;
}

// prices: two decimals below 1000. random: random finite bit patterns, which
// need up to 17 significant digits and use the whole exponent range.
enum class float_range { prices, random };

template <typename T>
std::vector<T> make_floats(size_t n, float_range r) {
  std::mt19937_64 rng(1234);
  std::vector<T> out(n);
  for (auto &x : out) {
    if (r == float_range::prices) {
      x = T(double(rng() % 100000) / 100.0);
    } else {
      do {
        const uint64_t bits = rng();
        if constexpr (sizeof(T) == 8) {
          std::memcpy(&x, &bits, 8);
        } else {
          const uint32_t b32 = uint32_t(bits);
          std::memcpy(&x, &b32, 4);
        }
      } while (!std::isfinite(x));
    }
  }
  return out;
}

enum class string_kind { short_ascii, bytes64, bytes400, escaped10 };

std::vector<std::string> make_strings(size_t n, string_kind k) {
  std::mt19937_64 rng(1234);
  std::vector<std::string> out;
  out.reserve(n);
  for (size_t i = 0; i < n; i++) {
    switch (k) {
    case string_kind::short_ascii:
      out.push_back("hello-world-" + std::to_string(i));
      break;
    case string_kind::bytes64:
    case string_kind::bytes400: {
      std::string s(k == string_kind::bytes64 ? 64 : 400, ' ');
      for (auto &c : s) { c = char('a' + rng() % 26); }
      out.push_back(std::move(s));
      break;
    }
    case string_kind::escaped10: {
      // 16 bytes; one string in ten holds a quote and a newline.
      std::string s(16, ' ');
      for (auto &c : s) { c = char('a' + rng() % 26); }
      if (rng() % 10 == 0) {
        s[3] = '"';
        s[11] = '\n';
      }
      out.push_back(std::move(s));
      break;
    }
    }
  }
  return out;
}

// Arrays of views or pointers come with the strings they point into.
template <class T> const T &payload(const T &values) { return values; }
template <class H> const auto &payload(const std::shared_ptr<H> &h) { return h->items; }

const size_t sizes[] = {16, 1000, 100000};

template <mode M, class make_type>
void register_case(const std::string &name, make_type make) {
  for (size_t n : sizes) {
    benchmark::RegisterBenchmark(
        (name + "/" + mode_name(M) + "/" + std::to_string(n)).c_str(),
        [make, n](benchmark::State &state) {
          const auto values = make(n);
          serialize<M>(state, payload(values));
        });
  }
}

template <class make_type>
void register_all_modes(const std::string &name, make_type make) {
  register_case<mode::builder_reuse>(name, make);
  register_case<mode::builder_fresh>(name, make);
  register_case<mode::to_json_fresh>(name, make);
  register_case<mode::to_json_reuse>(name, make);
}

template <typename T>
void register_integers(const char *type_name) {
  register_all_modes(std::string(type_name) + "/small",
                     [](size_t n) { return make_integers<T>(n, int_range::small); });
  register_all_modes(std::string(type_name) + "/mixed",
                     [](size_t n) { return make_integers<T>(n, int_range::mixed); });
}

void register_benchmarks() {
  register_integers<uint16_t>("uint16");
  register_integers<int32_t>("int32");
  register_integers<int64_t>("int64");
  register_integers<uint64_t>("uint64");
  register_all_modes("double/prices",
                     [](size_t n) { return make_floats<double>(n, float_range::prices); });
  register_all_modes("double/random",
                     [](size_t n) { return make_floats<double>(n, float_range::random); });
  register_all_modes("float/prices",
                     [](size_t n) { return make_floats<float>(n, float_range::prices); });
  register_all_modes("string/short",
                     [](size_t n) { return make_strings(n, string_kind::short_ascii); });
  register_all_modes("string/64B",
                     [](size_t n) { return make_strings(n, string_kind::bytes64); });
  register_all_modes("string/400B",
                     [](size_t n) { return make_strings(n, string_kind::bytes400); });
  register_all_modes("string/16B_10pct_escaped",
                     [](size_t n) { return make_strings(n, string_kind::escaped10); });
  register_all_modes("string_view/short", [](size_t n) {
    struct holder {
      std::vector<std::string> storage;
      std::vector<std::string_view> items;
    };
    auto h = std::make_shared<holder>();
    h->storage = make_strings(n, string_kind::short_ascii);
    h->items.assign(h->storage.begin(), h->storage.end());
    return h;
  });
  register_all_modes("const_char_ptr/short", [](size_t n) {
    struct holder {
      std::vector<std::string> storage;
      std::vector<const char *> items;
    };
    auto h = std::make_shared<holder>();
    h->storage = make_strings(n, string_kind::short_ascii);
    for (const auto &s : h->storage) { h->items.push_back(s.c_str()); }
    return h;
  });
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
  printf("bench_builder_arrays needs C++20 ranges and concepts.\n");
  return 0;
}
#endif
