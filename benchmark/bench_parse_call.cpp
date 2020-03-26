#include <benchmark/benchmark.h>
#include "simdjson.h"
using namespace simdjson;
using namespace benchmark;
using namespace std;

const padded_string EMPTY_ARRAY("[]", 2);

static void json_parse(State& state) {
  document::parser parser;
  if (parser.set_capacity(EMPTY_ARRAY.length())) { return; }
  for (auto _ : state) {
    auto error = simdjson::json_parse(EMPTY_ARRAY, parser);
    if (error) { return; }
  }
}
BENCHMARK(json_parse);
static void parser_parse_error_code(State& state) {
  document::parser parser;
  if (parser.set_capacity(EMPTY_ARRAY.length())) { return; }
  for (auto _ : state) {
    auto [doc, error] = parser.parse(EMPTY_ARRAY);
    if (error) { return; }
  }
}
BENCHMARK(parser_parse_error_code);
static void parser_parse_exception(State& state) {
  document::parser parser;
  if (parser.set_capacity(EMPTY_ARRAY.length())) { return; }
  for (auto _ : state) {
    try {
      UNUSED document::element doc = parser.parse(EMPTY_ARRAY);
    } catch(simdjson_error &j) {
      return;
    }
  }
}
BENCHMARK(parser_parse_exception);

static void build_parsed_json(State& state) {
  for (auto _ : state) {
    document::parser parser = simdjson::build_parsed_json(EMPTY_ARRAY);
    if (!parser.valid) { return; }
  }
}
BENCHMARK(build_parsed_json);
static void document_parse_error_code(State& state) {
  for (auto _ : state) {
    document::parser parser;
    auto [doc, error] = parser.parse(EMPTY_ARRAY);
    if (error) { return; }
  }
}
BENCHMARK(document_parse_error_code);
static void document_parse_exception(State& state) {
  for (auto _ : state) {
    try {
      document::parser parser;
      UNUSED document::element doc = parser.parse(EMPTY_ARRAY);
    } catch(simdjson_error &j) {
      return;
    }
  }
}
BENCHMARK(document_parse_exception);

BENCHMARK_MAIN();