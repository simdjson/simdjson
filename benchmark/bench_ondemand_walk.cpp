// A full On-Demand walk: iterate every array and object, read every scalar,
// then check that the document ends. A validator that must look at every
// value does this, so the cost of entering and stepping through containers
// dominates when the containers are small.
//
// tiny_objects/<size> : an array of {"k1":"v1"} objects, about <size> bytes.
// records/<size>      : an array of objects with six fields of mixed types.
// numbers/<size>      : an array of arrays of eight integers.
// nested/<size>       : an array of objects that each hold an object and an
//                       array.
//
// bench_walk     : the walk, written against the API.
// bench_validate : ondemand::document::validate(), which checks the same things
//                  without creating values.
// bench_dom      : dom::parser::parse, which validates while building a tape.

#include <benchmark/benchmark.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include "simdjson.h"

namespace {

using namespace simdjson;

// The walk is a template called on the document and then on each value, as a
// validator written against the API would be (Apache Velox validates its JSON
// input this way). The shape matters: the same walk split into functions that
// take an ondemand::value by reference compiles to different code.
template <typename T>
error_code walk(T value, uint64_t &sink) {
  ondemand::json_type type;
  SIMDJSON_TRY(value.type().get(type));
  switch (type) {
  case ondemand::json_type::array: {
    ondemand::array array;
    SIMDJSON_TRY(value.get_array().get(array));
    for (auto element_result : array) {
      ondemand::value element;
      SIMDJSON_TRY(element_result.get(element));
      SIMDJSON_TRY(walk(element, sink));
    }
    return SUCCESS;
  }
  case ondemand::json_type::object: {
    ondemand::object object;
    SIMDJSON_TRY(value.get_object().get(object));
    for (auto field_result : object) {
      ondemand::field field;
      SIMDJSON_TRY(std::move(field_result).get(field));
      SIMDJSON_TRY(walk(field.value(), sink));
    }
    return SUCCESS;
  }
  case ondemand::json_type::number: {
    double v;
    SIMDJSON_TRY(value.get_double().get(v));
    sink += uint64_t(v != 0);
    return SUCCESS;
  }
  case ondemand::json_type::string: {
    std::string_view v;
    SIMDJSON_TRY(value.get_string().get(v));
    sink += v.size();
    return SUCCESS;
  }
  case ondemand::json_type::boolean: {
    bool v;
    SIMDJSON_TRY(value.get_bool().get(v));
    sink += v;
    return SUCCESS;
  }
  case ondemand::json_type::null: {
    bool v;
    SIMDJSON_TRY(value.is_null().get(v));
    if (!v) { return N_ATOM_ERROR; }
    sink += 1;
    return SUCCESS;
  }
  default:
    return INCORRECT_TYPE;
  }
}

error_code walk_document(ondemand::parser &parser, const padded_string &json, uint64_t &sink) {
  ondemand::document doc;
  SIMDJSON_TRY(parser.iterate(json).get(doc));
  SIMDJSON_TRY(walk<ondemand::document &>(doc, sink));
  if (!doc.at_end()) { return TRAILING_CONTENT; }
  return SUCCESS;
}

// Repeats item, comma separated, inside [ and ] until the text has about
// size bytes (at least one item).
std::string repeat(const std::string &item, size_t size) {
  std::string out = "[";
  out.append(item);
  while (out.size() + item.size() + 2 <= size) {
    out.push_back(',');
    out.append(item);
  }
  out.push_back(']');
  return out;
}

std::string tiny_objects(size_t size) {
  return R"({"key":)" + repeat(R"({"k1":"v1"})", size - 8) + "}";
}

std::string records(size_t size) {
  return repeat(R"({"id":12345,"name":"alice","score":98.5,"active":true,"tags":null,"group":"b"})", size);
}

std::string numbers(size_t size) {
  return repeat("[1,22,333,4444,55555,666666,7777777,88888888]", size);
}

std::string nested(size_t size) {
  return repeat(R"({"a":{"x":1,"y":2},"b":[true,false],"c":"z"})", size);
}

void bench_walk(benchmark::State &state, std::string (*make)(size_t)) {
  const std::string text = make(size_t(state.range(0)));
  padded_string json(text);
  ondemand::parser parser;
  uint64_t sink = 0;
  for (auto _ : state) {
    error_code error = walk_document(parser, json, sink);
    if (error) {
      state.SkipWithError(error_message(error));
      return;
    }
  }
  benchmark::DoNotOptimize(sink);
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(text.size()));
}

void bench_validate(benchmark::State &state, std::string (*make)(size_t)) {
  const std::string text = make(size_t(state.range(0)));
  padded_string json(text);
  ondemand::parser parser;
  for (auto _ : state) {
    ondemand::document doc;
    error_code error = parser.iterate(json).get(doc);
    if (!error) { error = doc.validate(); }
    if (error) {
      state.SkipWithError(error_message(error));
      return;
    }
  }
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(text.size()));
}

void bench_dom(benchmark::State &state, std::string (*make)(size_t)) {
  const std::string text = make(size_t(state.range(0)));
  padded_string json(text);
  dom::parser parser;
  for (auto _ : state) {
    error_code error = parser.parse(json).error();
    if (error) {
      state.SkipWithError(error_message(error));
      return;
    }
  }
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(text.size()));
}

} // namespace

#define WALK_CASES(fn) \
  BENCHMARK_CAPTURE(fn, tiny_objects, tiny_objects)->Arg(32)->Arg(1000)->Arg(10000); \
  BENCHMARK_CAPTURE(fn, records, records)->Arg(1000)->Arg(10000); \
  BENCHMARK_CAPTURE(fn, numbers, numbers)->Arg(1000)->Arg(10000); \
  BENCHMARK_CAPTURE(fn, nested, nested)->Arg(1000)->Arg(10000)

WALK_CASES(bench_walk);
WALK_CASES(bench_validate);
WALK_CASES(bench_dom);

BENCHMARK_MAIN();
