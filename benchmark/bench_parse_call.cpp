#include <benchmark/benchmark.h>
#include "simdjson.h"
using namespace simdjson;
using namespace benchmark;
using namespace std;

const padded_string EMPTY_ARRAY("[]", 2);
const char *TWITTER_JSON = SIMDJSON_BENCHMARK_DATA_DIR "twitter.json";
const char *GSOC_JSON = SIMDJSON_BENCHMARK_DATA_DIR "gsoc-2018.json";



static void parse_twitter(State& state) {
  dom::parser parser;
  padded_string docdata;
  simdjson::error_code error;
  padded_string::load(TWITTER_JSON).tie(docdata, error);
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
  for (auto _ : state) {
    dom::element doc;
    bytes += docdata.size();
    parser.parse(docdata).tie(doc,error);
    if(error) {
      cerr << "could not parse twitter.json" << error << endl;
      return;
    }
    benchmark::DoNotOptimize(doc);
  }
  state.counters["Bytes"] = benchmark::Counter(
	        bytes, benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1024);
  state.counters["docs"] = Counter(state.iterations(), benchmark::Counter::kIsRate);
}
BENCHMARK(parse_twitter)->Repetitions(10)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);


static void parse_gsoc(State& state) {
  dom::parser parser;
  padded_string docdata;
  simdjson::error_code error;
  padded_string::load(GSOC_JSON).tie(docdata, error);
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
  for (auto _ : state) {
    dom::element doc;
    bytes += docdata.size();
    parser.parse(docdata).tie(doc,error);
    if(error) {
      cerr << "could not parse gsoc-2018.json" << error << endl;
      return;
    }
    benchmark::DoNotOptimize(doc);
  }
  state.counters["Bytes"] = benchmark::Counter(
	        bytes, benchmark::Counter::kIsRate,
	        benchmark::Counter::OneK::kIs1024);
  state.counters["docs"] = Counter(state.iterations(),    benchmark::Counter::kIsRate);
}
BENCHMARK(parse_gsoc)->Repetitions(10)->ComputeStatistics("max", [](const std::vector<double>& v) -> double {
    return *(std::max_element(std::begin(v), std::end(v)));
  })->DisplayAggregatesOnly(true);



SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING
static void json_parse(State& state) {
  ParsedJson pj;
  if (!pj.allocate_capacity(EMPTY_ARRAY.length())) { return; }
  for (auto _ : state) {
    auto error = json_parse(EMPTY_ARRAY, pj);
    if (error) { return; }
  }
}
SIMDJSON_POP_DISABLE_WARNINGS
BENCHMARK(json_parse);
static void parser_parse_error_code(State& state) {
  dom::parser parser;
  if (parser.allocate(EMPTY_ARRAY.length())) { return; }
  for (auto _ : state) {
    auto error = parser.parse(EMPTY_ARRAY).error();
    if (error) { return; }
  }
}
BENCHMARK(parser_parse_error_code);
static void parser_parse_exception(State& state) {
  dom::parser parser;
  if (parser.allocate(EMPTY_ARRAY.length())) { return; }
  for (auto _ : state) {
    try {
      UNUSED dom::element doc = parser.parse(EMPTY_ARRAY);
    } catch(simdjson_error &j) {
      return;
    }
  }
}
BENCHMARK(parser_parse_exception);

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING
static void build_parsed_json(State& state) {
  for (auto _ : state) {
    dom::parser parser = simdjson::build_parsed_json(EMPTY_ARRAY);
    if (!parser.valid) { return; }
  }
}
SIMDJSON_POP_DISABLE_WARNINGS
BENCHMARK(build_parsed_json);
static void document_parse_error_code(State& state) {
  for (auto _ : state) {
    dom::parser parser;
    auto error = parser.parse(EMPTY_ARRAY).error();
    if (error) { return; }
  }
}
BENCHMARK(document_parse_error_code);
static void document_parse_exception(State& state) {
  for (auto _ : state) {
    try {
      dom::parser parser;
      UNUSED dom::element doc = parser.parse(EMPTY_ARRAY);
    } catch(simdjson_error &j) {
      return;
    }
  }
}
BENCHMARK(document_parse_exception);

BENCHMARK_MAIN();