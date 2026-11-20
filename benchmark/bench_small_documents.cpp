// Parsing many small documents, one at a time, with a reused parser. For
// tiny inputs the fixed cost of a parse (setup, stage 1 entry, the first
// value) dominates.
//
// dom/<case>      : dom::parser::parse, then read the root value.
// ondemand/<case> : ondemand::parser::iterate, then read the root value.
//
// The atom cases are a document that is a single number, true, false or null.
// The other cases are short strings, objects and arrays, a number longer than
// 64 bytes, and malformed input (which must fail).

#include <benchmark/benchmark.h>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
#include "simdjson.h"

namespace {

using namespace simdjson;

error_code read_dom(dom::element e, uint64_t &sink) {
  switch (e.type()) {
  case dom::element_type::INT64: { int64_t v; SIMDJSON_TRY(e.get(v)); sink += uint64_t(v); break; }
  case dom::element_type::UINT64: { uint64_t v; SIMDJSON_TRY(e.get(v)); sink += v; break; }
  case dom::element_type::DOUBLE: { double v; SIMDJSON_TRY(e.get(v)); sink += uint64_t(v != 0); break; }
  case dom::element_type::BOOL: { bool v; SIMDJSON_TRY(e.get(v)); sink += v; break; }
  case dom::element_type::NULL_VALUE: sink += 1; break;
  case dom::element_type::STRING: { std::string_view v; SIMDJSON_TRY(e.get(v)); sink += v.size(); break; }
  case dom::element_type::ARRAY: { dom::array a; SIMDJSON_TRY(e.get(a)); sink += a.size(); break; }
  case dom::element_type::OBJECT: { dom::object o; SIMDJSON_TRY(e.get(o)); sink += o.size(); break; }
  case dom::element_type::BIGINT: break;
  }
  return SUCCESS;
}

error_code read_ondemand(ondemand::document &doc, uint64_t &sink) {
  ondemand::json_type t;
  SIMDJSON_TRY(doc.type().get(t));
  switch (t) {
  case ondemand::json_type::number: {
    ondemand::number_type nt;
    SIMDJSON_TRY(doc.get_number_type().get(nt));
    if (nt == ondemand::number_type::signed_integer) {
      int64_t v; SIMDJSON_TRY(doc.get_int64().get(v)); sink += uint64_t(v);
    } else if (nt == ondemand::number_type::unsigned_integer) {
      uint64_t v; SIMDJSON_TRY(doc.get_uint64().get(v)); sink += v;
    } else {
      double v; SIMDJSON_TRY(doc.get_double().get(v)); sink += uint64_t(v != 0);
    }
    break;
  }
  case ondemand::json_type::boolean: { bool v; SIMDJSON_TRY(doc.get_bool().get(v)); sink += v; break; }
  case ondemand::json_type::null: { bool v; SIMDJSON_TRY(doc.is_null().get(v)); sink += v; break; }
  case ondemand::json_type::string: { std::string_view v; SIMDJSON_TRY(doc.get_string().get(v)); sink += v.size(); break; }
  case ondemand::json_type::array: { size_t n; SIMDJSON_TRY(doc.count_elements().get(n)); sink += n; break; }
  case ondemand::json_type::object: { size_t n; SIMDJSON_TRY(doc.count_fields().get(n)); sink += n; break; }
  default: return INCORRECT_TYPE;
  }
  return SUCCESS;
}

void bench_dom(benchmark::State &state, const std::string &text, bool expect_error) {
  padded_string json(text);
  dom::parser parser;
  uint64_t sink = 0;
  for (auto _ : state) {
    dom::element e;
    error_code error = parser.parse(json).get(e);
    if (!error) { error = read_dom(e, sink); }
    if (bool(error) != expect_error) {
      state.SkipWithError(error ? error_message(error) : "unexpected success");
      return;
    }
  }
  benchmark::DoNotOptimize(sink);
  state.SetItemsProcessed(int64_t(state.iterations()));
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(text.size()));
}

void bench_ondemand(benchmark::State &state, const std::string &text, bool expect_error) {
  padded_string json(text);
  ondemand::parser parser;
  uint64_t sink = 0;
  for (auto _ : state) {
    ondemand::document doc;
    error_code error = parser.iterate(json).get(doc);
    if (!error) { error = read_ondemand(doc, sink); }
    if (bool(error) != expect_error) {
      state.SkipWithError(error ? error_message(error) : "unexpected success");
      return;
    }
  }
  benchmark::DoNotOptimize(sink);
  state.SetItemsProcessed(int64_t(state.iterations()));
  state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(text.size()));
}

void register_benchmarks() {
  const std::pair<const char *, std::string> valid[] = {
      {"atom/true", "true"},
      {"atom/false", "false"},
      {"atom/null", "null"},
      {"atom/zero", "0"},
      {"atom/int_short", "42"},
      {"atom/int_negative", "-123456789"},
      {"atom/uint64_above_int64_max", "18446744073709551615"},
      {"atom/double_short", "3.25"},
      {"atom/double_long", "-1.2345678901234567e-89"},
      {"atom/with_white_space", "  \n 42 \t\r\n"},
      {"atom/number_62_chars", std::string("1.") + std::string(60, '7')},
      {"long/number_80_chars", std::string("1.") + std::string(78, '7')},
      {"string/hello", "\"hello\""},
      {"string/40B", "\"" + std::string(40, 'x') + "\""},
      {"object/one_field", "{\"a\":1}"},
      {"object/three_fields", "{\"id\":12,\"name\":\"abc\",\"ok\":true}"},
      {"array/three_ints", "[1,2,3]"},
      {"array/empty", "[]"},
  };
  const std::pair<const char *, std::string> invalid[] = {
      {"error/truex", "truex"},
      {"error/double_dot", "1.2.3"},
      {"error/unclosed_object", "{\"a\":1"},
  };
  for (const auto &c : valid) {
    const std::string text = c.second;
    benchmark::RegisterBenchmark((std::string("dom/") + c.first).c_str(),
        [text](benchmark::State &state) { bench_dom(state, text, false); });
    benchmark::RegisterBenchmark((std::string("ondemand/") + c.first).c_str(),
        [text](benchmark::State &state) { bench_ondemand(state, text, false); });
  }
  for (const auto &c : invalid) {
    const std::string text = c.second;
    benchmark::RegisterBenchmark((std::string("dom/") + c.first).c_str(),
        [text](benchmark::State &state) { bench_dom(state, text, true); });
    benchmark::RegisterBenchmark((std::string("ondemand/") + c.first).c_str(),
        [text](benchmark::State &state) { bench_ondemand(state, text, true); });
  }
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
