#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <endian.h>
#include <string>
#include <counters/event_counter.h>
#include "from/benchmark_helpers.h"
#include "simdjson.h"

using namespace simdjson;
using namespace counters;

namespace {

  // The cost of get_int64(), get_double() and get_number() depends value size,
  // for this reason we use switchs (Short:5 digits, medium:12 and long:19)
  enum class number_length { short_digits, medium_digits, long_digits };

  constexpr size_t VALUE_COUNT = 1000000;

  struct number_dataset {
    padded_string json;
    size_t count{};
  };

  // Deterministic seed, so two runs on the same machine are comparable
  struct xorshift64 {
    uint64_t state;
    uint64_t operator()() {
      state ^= state << 13;
      state ^= state >> 7;
      state ^= state << 17;
      return state;
    }
  };

  number_dataset build_int_dataset(number_length which) {
    xorshift64 rng{0x2545f4914f6cdd1dULL};
    std::string out;

    out.reserve(VALUE_COUNT * 24);
    out += '[';

    for (size_t i = 0; i < VALUE_COUNT; i++) {
      if (i > 0) { out += ','; }
      uint64_t magnitude = 0;

      switch (which) {
        case number_length::short_digits:
          magnitude = 10000ULL + rng() % 90000ULL; // 5 digits
          break;

        case number_length::medium_digits:
          magnitude = 100000000000ULL + rng() % 900000000000ULL; // 12 digits
          break;

        case number_length::long_digits:
          magnitude = 1000000000000000000ULL + rng() % 8000000000000000000ULL; //19
          break;
      }
      if ((rng() & 1) != 0) { out += '-'; }
      out += std::to_string(magnitude);
    }

    out += ']';
    return {padded_string(out), VALUE_COUNT};
  }

  number_dataset build_double_dataset(number_length which) {
    xorshift64 rng{0x9e3779b97f4a7c15ULL};
    std::string out;
    char buffer[64];

    out.reserve(VALUE_COUNT * 28);
    out += '[';

    for (size_t i = 0; i < VALUE_COUNT; i++) {

      if (i > 0) { out += ','; }
      double numerator = double(int64_t(rng() % 2000000000) - 1000000000);
      double denominator = double(1 + rng() % 100000);
      double value = numerator / denominator;

      if (!std::isfinite(value)) { value = 1.5; }

      switch (which) {
        case number_length::short_digits:
          snprintf(buffer, sizeof(buffer), "%.4e", value); // 5 digits
          break;
        case number_length::medium_digits:
          snprintf(buffer, sizeof(buffer), "%.11e", value); // 12 digits
          break;
        case number_length::long_digits:
          snprintf(buffer, sizeof(buffer), "%.18e", value); // 19 digits
          break;
      }
      out += buffer;
    }

    out += ']';
    return {padded_string(out), VALUE_COUNT};
  }

  // method used for get_number, dataset made of half int64 and half double
  number_dataset build_mixed_dataset(number_length which) {
    xorshift64 rng{0x853c49e6748fea9bULL};
    std::string out;
    char buffer[64];

    out.reserve(VALUE_COUNT * 28);
    out += '[';

    for (size_t i = 0; i < VALUE_COUNT; i++) {
      if (i > 0) { out += ','; }

      if ((rng() & 1) != 0) {
        uint64_t magnitude = 0;
        switch (which) {
          case number_length::short_digits:
            magnitude = 10000ULL + rng() % 90000ULL; // 5 digits
            break;
          case number_length::medium_digits:
            magnitude = 100000000000ULL + rng() % 900000000000ULL; // 12 digits
            break;
          case number_length::long_digits:
            magnitude = 1000000000000000000ULL + rng() % 8000000000000000000ULL; //19
            break;
        }
        if ((rng() & 1) != 0) { out += '-'; }
        out += std::to_string(magnitude);
      } else {
        double numerator = double(int64_t(rng() % 2000000000) - 1000000000);
        double denominator = double(1 + rng() % 100000);
        double value = numerator / denominator;

        if (!std::isfinite(value)) { value = 1.5; }

        switch (which) {
          case number_length::short_digits:
            snprintf(buffer, sizeof(buffer), "%.4e", value); // 5 digits
            break;
          case number_length::medium_digits:
            snprintf(buffer, sizeof(buffer), "%.11e", value); //12 digits
            break;
          case number_length::long_digits:
            snprintf(buffer, sizeof(buffer), "%.18e", value); // 19 digits
            break;
        }
        out += buffer;
      }
    }

    out += ']';
    return {padded_string(out), VALUE_COUNT};
  }

  const number_dataset &get_int_dataset(number_length which) {
    static const number_dataset short_ints =
      build_int_dataset(number_length::short_digits);
    static const number_dataset medium_ints =
      build_int_dataset(number_length::medium_digits);
    static const number_dataset long_ints =
      build_int_dataset(number_length::long_digits);

    switch (which) {
      case number_length::short_digits:
        return short_ints;
      case number_length::medium_digits:
        return medium_ints;
      case number_length::long_digits:
        return long_ints;
    }
    return short_ints;
  }

  const number_dataset &get_double_dataset(number_length which) {
    static const number_dataset short_doubles =
      build_double_dataset(number_length::short_digits);
    static const number_dataset medium_doubles =
      build_double_dataset(number_length::medium_digits);
    static const number_dataset long_doubles =
      build_double_dataset(number_length::long_digits);

    switch (which) {
      case number_length::short_digits:
        return short_doubles;
      case number_length::medium_digits:
        return medium_doubles;
      case number_length::long_digits:
        return long_doubles;
    }
    return short_doubles;
  }

  const number_dataset &get_mixed_dataset(number_length which) {
    static const number_dataset short_mixed =
      build_mixed_dataset(number_length::short_digits);
    static const number_dataset medium_mixed =
      build_mixed_dataset(number_length::medium_digits);
    static const number_dataset long_mixed =
      build_mixed_dataset(number_length::long_digits);

    switch (which) {
      case number_length::short_digits:
        return short_mixed;
      case number_length::medium_digits:
        return medium_mixed;
      case number_length::long_digits:
        return long_mixed;
    }
    return short_mixed;
  }

  const char *label(number_length which) {
    switch (which) {
      case number_length::short_digits:
        return "5";
      case number_length::medium_digits:
        return "12";
      case number_length::long_digits:
        return "19";
    }
    return "5";
  }

  simdjson::error_code warm_up(ondemand::parser &parser, const padded_string &json) {
    ondemand::document doc;
    return parser.iterate(json).get(doc);
  }

  template <class function_type>
    std::pair<event_aggregate, size_t>
    bench(const function_type &&function, size_t min_repeat = 10,
        size_t min_time_ns = 40'000'000, size_t max_repeat = 10'000'000) {
      size_t N = min_repeat ? min_repeat : 1;
      event_aggregate warm_aggregate{};

      for (size_t i = 0; i < N; ++i){
        std::atomic_thread_fence(std::memory_order_acquire);
        collector.start();
        function();
        std::atomic_thread_fence(std::memory_order_release);

        warm_aggregate << collector.end();
        if ((i+1 == N) && (warm_aggregate.total_elapsed_ns() < min_time_ns) && (N < max_repeat)) {
          N *= 10;
        }
      }
      event_aggregate aggregate{};
      for (size_t i = 0; i<10; ++i) {
        std::atomic_thread_fence(std::memory_order_acquire);
        collector.start();
        for (size_t j = 0; j < N; ++j) {function();}
        std::atomic_thread_fence(std::memory_order_release);

        aggregate << collector.end();
      }
      return {aggregate, N};
    }

  double pretty_print_array(const std::string &name, size_t num_chars, size_t num_elements,
      std::pair<event_aggregate, size_t> result) {
    const auto &agg = result.first;
    size_t N = result.second;
    num_chars *= N;
    num_elements *= N;
    printf("    %-28s : %8.2f ns/value %8.2f GB/s", name.c_str(),
        agg.elapsed_ns() / num_elements, num_chars / agg.elapsed_ns());
    if (collector.has_events()) {
      printf(" %8.2f GHz %8.2f cycles/value %8.2f ins./value %8.2f i/c",
          agg.cycles() / agg.elapsed_ns(), agg.cycles() / num_elements,
          agg.instructions() / num_elements, agg.instructions() / agg.cycles());
    }
    printf("\n");
    return num_chars / agg.elapsed_ns();
  }

#if !defined(BENCH_ONLY_DOUBLE) && !defined(BENCH_ONLY_NUMBER)
  void run_ondemand_get_int64(number_length which_len) {
    const auto &dataset =
      get_int_dataset(which_len);
    ondemand::parser parser;
    volatile uint64_t sink = 0;

    if(warm_up(parser, dataset.json)) {return;}

    auto result = bench([&]() -> size_t {
      ondemand::document doc;
      if(parser.iterate(dataset.json).get(doc)) {return 0;}

      ondemand::array array;
      if(doc.get_array().get(array)) {return 0;}

      uint64_t sum = 0;
      for(auto element : array) {
        int64_t value;
        if(element.get_int64().get(value)) {return 0;}
        sum += uint64_t(value);
      }
      sink = sum;
      return size_t(sum);
    });
    pretty_print_array(
        "ondemand_get_int64",
        dataset.json.size(),
        dataset.count,
        result);
  }


  void run_dom_get_int64(number_length which_len) {
    const auto &dataset = get_int_dataset(which_len);
    dom::parser parser;
    dom::array array;
    volatile uint64_t sink = 0;
    if (parser.parse(dataset.json).get(array)) { return; }

    auto result = bench([&]() -> size_t {
      uint64_t sum = 0;
      for (auto element : array) {
        int64_t value;
        if (element.get_int64().get(value)) { return 0; }
        sum += uint64_t(value);
      }
      sink = sum;
      return size_t(sum);
    });

    pretty_print_array(
        "dom_get_int64",
        dataset.json.size(),
        dataset.count,
        result);
  }

#endif
#if !defined(BENCH_ONLY_INT) && !defined(BENCH_ONLY_NUMBER)
  void run_ondemand_get_double(number_length which_len) {
    const auto &dataset =
      get_double_dataset(which_len);
    ondemand::parser parser;
    volatile double sink = 0;

    if(warm_up(parser, dataset.json)) {return;}

    auto result = bench([&]() -> size_t {
      ondemand::document doc;
      if(parser.iterate(dataset.json).get(doc)) {return 0;}

      ondemand::array array;
      if(doc.get_array().get(array)) {return 0;}

      double sum = 0;
      for(auto element : array) {
        double value;
        if(element.get_double().get(value)) {return 0;}
        sum += value;
      }
      sink = sum;
      return size_t(sum);
    });
    pretty_print_array(
        "ondemand_get_double",
        dataset.json.size(),
        dataset.count,
        result);
  }

  void run_dom_get_double(number_length which_len) {
    const auto &dataset = get_double_dataset(which_len);
    dom::parser parser;
    dom::array array;
    volatile double sink = 0;
    if (parser.parse(dataset.json).get(array)) { return; }

    auto result = bench([&]() -> size_t {
      double sum = 0;
      for (auto element : array) {
        double value;
        if (element.get_double().get(value)) { return 0; }
        sum += value;
      }
      sink = sum;
      return size_t(sum);
    });

    pretty_print_array(
        "dom_get_double",
        dataset.json.size(),
        dataset.count,
        result);
  }

#endif
#if !defined(BENCH_ONLY_INT) && !defined(BENCH_ONLY_DOUBLE)
  void run_ondemand_get_number(number_length which_len) {
    const auto &dataset =
      get_mixed_dataset(which_len);
    ondemand::parser parser;
    volatile double sink = 0;

    if(warm_up(parser, dataset.json)) {return;}

    auto result = bench([&]() -> size_t {
      ondemand::document doc;
      if(parser.iterate(dataset.json).get(doc)) {return 0;}

      ondemand::array array;
      if(doc.get_array().get(array)) {return 0;}

      double sum = 0;
      for(auto element : array) {
        ondemand::number number;
        if(element.get_number().get(number)) {return 0;}
        sum += number.is_double() ? number.get_double() : double(number.get_int64());
      }
      sink = sum;
      return size_t(sum);
    });
    pretty_print_array(
        "ondemand_get_number",
        dataset.json.size(),
        dataset.count,
        result);
  }


#endif

  // if no BENCH_ONLY_* flag used: run all (int64, double and number)
  void run_benchmark() {
    for (auto len :{
        number_length::short_digits,
        number_length::medium_digits,
        number_length::long_digits}) {

    printf("  %s digits:\n", label(len));

#if !defined(BENCH_ONLY_DOUBLE) && !defined(BENCH_ONLY_NUMBER)
    run_ondemand_get_int64(len);
    run_dom_get_int64(len);
#endif
#if !defined(BENCH_ONLY_INT) && !defined(BENCH_ONLY_NUMBER)
    run_ondemand_get_double(len);
    run_dom_get_double(len);
#endif
#if !defined(BENCH_ONLY_INT) && !defined(BENCH_ONLY_DOUBLE)
    run_ondemand_get_number(len);
#endif

    }
  }
}

int main(){
  for(size_t trial = 0; trial < 3; ++trial) {
    printf("\nTrial %zu:\n", trial+1);
    run_benchmark();
  }
  return EXIT_SUCCESS;
}
