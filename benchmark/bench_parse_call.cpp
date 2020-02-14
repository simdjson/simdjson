#include <benchmark/benchmark.h>
#include "simdjson/document.h"
#include "simdjson/jsonparser.h"
using namespace simdjson;
using namespace benchmark;
using namespace std;

const padded_string EMPTY_ARRAY("[]", 2);

static void json_parse(State& state) {
  document::parser parser;
  if (!parser.allocate_capacity(EMPTY_ARRAY.length())) { return; }
  for (auto _ : state) {
    auto error = simdjson::json_parse(EMPTY_ARRAY, parser);
    if (error) { return; }
  }
}
BENCHMARK(json_parse);
static void parser_parse_error_code(State& state) {
  document::parser parser;
  if (!parser.allocate_capacity(EMPTY_ARRAY.length())) { return; }
  for (auto _ : state) {
    auto [doc, error] = parser.parse(EMPTY_ARRAY);
    if (error) { return; }
  }
}
BENCHMARK(parser_parse_error_code);
static void parser_parse_exception(State& state) {
  document::parser parser;
  if (!parser.allocate_capacity(EMPTY_ARRAY.length())) { return; }
  for (auto _ : state) {
    try {
      UNUSED document &doc = parser.parse(EMPTY_ARRAY);
    } catch(invalid_json &j) {
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
    auto [doc, error] = document::parse(EMPTY_ARRAY);
    if (error) { return; }
  }
}
BENCHMARK(document_parse_error_code);
static void document_parse_exception(State& state) {
  for (auto _ : state) {
    try {
      UNUSED document doc = document::parse(EMPTY_ARRAY);
    } catch(invalid_json &j) {
      return;
    }
  }
}
BENCHMARK(document_parse_exception);

BENCHMARK_MAIN();