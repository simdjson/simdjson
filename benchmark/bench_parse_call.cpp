#include <benchmark/benchmark.h>
#include <cstring>
#include <iostream>
#include "simdjson.h"
using namespace simdjson;
using namespace benchmark;
using namespace std;

const padded_string EMPTY_ARRAY("[]", 2);
const char *TWITTER_JSON = SIMDJSON_BENCHMARK_DATA_DIR "twitter.json";
const char *GSOC_JSON = SIMDJSON_BENCHMARK_DATA_DIR "gsoc-2018.json";




static void fast_minify_twitter(State& state) {
  dom::parser parser;
  padded_string docdata;
  auto error = padded_string::load(TWITTER_JSON).get(docdata);
  if(error) {
      cerr << "could not parse twitter.json" << error << endl;
      return;
  }
  std::unique_ptr<char[]> buffer{new char[docdata.size()]};

  size_t bytes = 0;
  for (simdjson_unused auto _ : state) {
    size_t new_length{}; // It will receive the minified length.
    auto error = simdjson::minify(docdata.data(), docdata.size(), buffer.get(), new_length);
    bytes += docdata.size();
    benchmark::DoNotOptimize(error);
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["Gigabytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
}
BENCHMARK(fast_minify_twitter)->Repetitions(10)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);




static void fast_minify_gsoc(State& state) {
  dom::parser parser;
  padded_string docdata;
  auto error = padded_string::load(GSOC_JSON).get(docdata);
  if(error) {
      cerr << "could not parse gsoc-2018.json" << error << endl;
      return;
  }
  std::unique_ptr<char[]> buffer{new char[docdata.size()]};

  size_t bytes = 0;
  for (simdjson_unused auto _ : state) {
    size_t new_length{}; // It will receive the minified length.
    auto error = simdjson::minify(docdata.data(), docdata.size(), buffer.get(), new_length);
    bytes += docdata.size();
    benchmark::DoNotOptimize(error);
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["Gigabytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
}
BENCHMARK(fast_minify_gsoc)->Repetitions(10)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);

static void unicode_validate_twitter(State& state) {
  dom::parser parser;
  padded_string docdata;
  auto error = padded_string::load(TWITTER_JSON).get(docdata);
  if(error) {
      cerr << "could not parse twitter.json" << error << endl;
      return;
  }
  // we do not want mem. alloc. in the loop.
  error = parser.allocate(docdata.size());
  if(error) {
      cout << error << endl;
      return;
  }
  size_t bytes = 0;
  for (simdjson_unused auto _ : state) {
    bool is_ok = simdjson::validate_utf8(docdata.data(), docdata.size());
    bytes += docdata.size();
    benchmark::DoNotOptimize(is_ok);
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["Gigabytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
}
BENCHMARK(unicode_validate_twitter)->Repetitions(10)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);

static void benchmark_validate_utf8(State& state, const padded_string& docdata, bool expected) {
  if (simdjson::validate_utf8(docdata.data(), docdata.size()) != expected) {
    state.SkipWithError("unexpected UTF-8 validation result");
    return;
  }
  for (simdjson_unused auto _ : state) {
    benchmark::DoNotOptimize(simdjson::validate_utf8(docdata.data(), docdata.size()));
  }
  state.counters["Gigabytes"] = benchmark::Counter(
      double(state.iterations()) * double(docdata.size()), benchmark::Counter::kIsRate,
      benchmark::Counter::OneK::kIs1000);
}

static padded_string make_ascii_utf8_input(size_t size) {
  padded_string input(size);
  std::memset(input.data(), 'a', size);
  return input;
}

static void unicode_validate_ascii(State& state) {
  const padded_string docdata = make_ascii_utf8_input(size_t(state.range(0)));
  benchmark_validate_utf8(state, docdata, true);
}
BENCHMARK(unicode_validate_ascii)->Arg(64 * 1024)->Arg(1024 * 1024)->UseRealTime()->Repetitions(10);

static void unicode_validate_multilingual(State& state) {
  padded_string docdata((1024 * 1024 / 3) * 3);
  for (size_t i = 0; i < docdata.size(); i += 3) {
    docdata.data()[i] = char(0xe2);
    docdata.data()[i + 1] = char(0x82);
    docdata.data()[i + 2] = char(0xac);
  }
  benchmark_validate_utf8(state, docdata, true);
}
BENCHMARK(unicode_validate_multilingual)->UseRealTime()->Repetitions(10);

static void unicode_validate_sparse(State& state) {
  padded_string docdata = make_ascii_utf8_input(1024 * 1024);
  const size_t stride = size_t(state.range(0));
  for (size_t i = 0; i + 1 < docdata.size(); i += stride) {
    docdata.data()[i] = char(0xc3);
    docdata.data()[i + 1] = char(0xa9);
  }
  benchmark_validate_utf8(state, docdata, true);
}
BENCHMARK(unicode_validate_sparse)->Arg(100)->Arg(150)->Arg(200)->Arg(300)->Arg(500)->UseRealTime()->Repetitions(10);

static void unicode_validate_boundary_error(State& state) {
  padded_string docdata = make_ascii_utf8_input(size_t(state.range(0)));
  docdata.data()[63] = char(0xe2);
  docdata.data()[64] = 'a';
  benchmark_validate_utf8(state, docdata, false);
}
BENCHMARK(unicode_validate_boundary_error)->Arg(4 * 1024)->Arg(64 * 1024)->Arg(1024 * 1024)->UseRealTime()->Repetitions(10);

static void parse_twitter(State& state) {
  dom::parser parser;
  padded_string docdata;
  auto error = padded_string::load(TWITTER_JSON).get(docdata);
  if(error) {
      cerr << "could not parse twitter.json" << error << endl;
      return;
  }
  // we do not want mem. alloc. in the loop.
  error = parser.allocate(docdata.size());
  if(error) {
      cout << error << endl;
      return;
  }
  size_t bytes = 0;
  for (simdjson_unused auto _ : state) {
    dom::element doc;
    bytes += docdata.size();
    if ((error = parser.parse(docdata).get(doc))) {
      cerr << "could not parse twitter.json" << error << endl;
      return;
    }
    benchmark::DoNotOptimize(doc);
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["Gigabytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
}
BENCHMARK(parse_twitter)->Repetitions(10)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);


static void parse_gsoc(State& state) {
  dom::parser parser;
  padded_string docdata;
  auto error = padded_string::load(GSOC_JSON).get(docdata);
  if(error) {
      cerr << "could not parse gsoc-2018.json" << error << endl;
      return;
  }
  // we do not want mem. alloc. in the loop.
  error = parser.allocate(docdata.size());
  if(error) {
      cout << error << endl;
      return;
  }
  size_t bytes = 0;
  for (simdjson_unused auto _ : state) {
    bytes += docdata.size();
    dom::element doc;
    if ((error = parser.parse(docdata).get(doc))) {
      cerr << "could not parse gsoc-2018.json" << error << endl;
      return;
    }
    benchmark::DoNotOptimize(doc);
  }
  // Gigabyte: https://en.wikipedia.org/wiki/Gigabyte
  state.counters["Gigabytes"] = benchmark::Counter(
	        double(bytes), benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1000); // For GiB : kIs1024
  state.counters["docs"] = Counter(double(state.iterations()), benchmark::Counter::kIsRate);
}
BENCHMARK(parse_gsoc)->Repetitions(10)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);


static void parser_parse_error_code(State& state) {
  dom::parser parser;
  if (parser.allocate(EMPTY_ARRAY.length())) { return; }
  for (simdjson_unused auto _ : state) {
    auto error = parser.parse(EMPTY_ARRAY).error();
    if (error) { return; }
  }
}
BENCHMARK(parser_parse_error_code);

#if SIMDJSON_EXCEPTIONS

static void parser_parse_exception(State& state) {
  dom::parser parser;
  if (parser.allocate(EMPTY_ARRAY.length())) { return; }
  for (simdjson_unused auto _ : state) {
    try {
      simdjson_unused dom::element doc = parser.parse(EMPTY_ARRAY);
    } catch(simdjson_error &j) {
      cout << j.what() << endl;
      return;
    }
  }
}
BENCHMARK(parser_parse_exception);

#endif // SIMDJSON_EXCEPTIONS

static void document_parse_error_code(State& state) {
  for (simdjson_unused auto _ : state) {
    dom::parser parser;
    auto error = parser.parse(EMPTY_ARRAY).error();
    if (error) { return; }
  }
}
BENCHMARK(document_parse_error_code);

#if SIMDJSON_EXCEPTIONS

static void document_parse_exception(State& state) {
  for (simdjson_unused auto _ : state) {
    try {
      dom::parser parser;
      simdjson_unused dom::element doc = parser.parse(EMPTY_ARRAY);
    } catch(simdjson_error &j) {
      cout << j.what() << endl;
      return;
    }
  }
}
BENCHMARK(document_parse_exception);

#endif // SIMDJSON_EXCEPTIONS

BENCHMARK_MAIN();
