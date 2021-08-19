#include <benchmark/benchmark.h>
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



#ifndef SIMDJSON_DISABLE_DEPRECATED_API
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING
static void json_parse(State& state) {
  ParsedJson pj;
  if (!pj.allocate_capacity(EMPTY_ARRAY.length())) { return; }
  for (simdjson_unused auto _ : state) {
    auto error = json_parse(EMPTY_ARRAY, pj);
    if (error) { return; }
  }
}
SIMDJSON_POP_DISABLE_WARNINGS
BENCHMARK(json_parse);
#endif // SIMDJSON_DISABLE_DEPRECATED_API

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

#ifndef SIMDJSON_DISABLE_DEPRECATED_API
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING
static void build_parsed_json(State& state) {
  for (simdjson_unused auto _ : state) {
    dom::parser parser = simdjson::build_parsed_json(EMPTY_ARRAY);
    if (!parser.valid) { return; }
  }
}
SIMDJSON_POP_DISABLE_WARNINGS

BENCHMARK(build_parsed_json);
#endif

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
