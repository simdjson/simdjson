#include <benchmark/benchmark.h>
#include "simdjson.h"
using namespace simdjson;
using namespace benchmark;
using namespace std;

const padded_string EMPTY_ARRAY("[]", 2);

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
    auto [doc, error] = parser.parse(EMPTY_ARRAY);
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
    auto [doc, error] = parser.parse(EMPTY_ARRAY);
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