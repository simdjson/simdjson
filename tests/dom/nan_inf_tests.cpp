#include "simdjson.h"
#include "test_macros.h"
#include "test_main.h"
#include <array>
#include <cmath>
#include <limits>
#include <string>

using namespace simdjson;

namespace nan_inf_tests {
#if SIMDJSON_ENABLE_NAN_INF

bool parse_nan() {
  TEST_START();
  for (auto json_str : {"NaN", "nan", "NAN"}) {
    dom::parser parser;
    dom::element doc;
    ASSERT_SUCCESS(
        parser.parse(padded_string(json_str, strlen(json_str))).get(doc));
    double value;
    ASSERT_SUCCESS(doc.get_double().get(value));
    ASSERT_TRUE(std::isnan(value));
  }
  TEST_SUCCEED();
}

bool parse_infinity() {
  TEST_START();
  for (auto json_str : {"infinity", "Infinity", "INFINITY", "inf", "Inf", "INF",
                        // Check that 'Inf' parses correctly even when padded to
                        // the same length as 'Infinity'
                        "inf     ", "Inf     ", "INF     "}) {
    dom::parser parser;
    dom::element doc;
    ASSERT_SUCCESS(
        parser.parse(padded_string(json_str, strlen(json_str))).get(doc));
    double value;
    ASSERT_SUCCESS(doc.get_double().get(value));
    ASSERT_TRUE(std::isinf(value));
    ASSERT_TRUE(value > 0);
  }
  TEST_SUCCEED();
}

bool parse_negative_infinity() {
  TEST_START();
  for (auto json_str :
       {"-infinity", "-Infinity", "-INFINITY", "-inf", "-Inf", "-INF"}) {
    dom::parser parser;
    dom::element doc;
    ASSERT_SUCCESS(
        parser.parse(padded_string(json_str, strlen(json_str))).get(doc));
    double value;
    ASSERT_SUCCESS(doc.get_double().get(value));
    ASSERT_TRUE(std::isinf(value));
    ASSERT_TRUE(value < 0);
  }
  TEST_SUCCEED();
}

bool nan_in_array() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  auto json = R"([5, NaN, NAN, nan, -nan, -NAN, -NaN, 1.25])"_padded;
  ASSERT_SUCCESS(parser.parse(json).get(doc));
  dom::array arr;
  ASSERT_SUCCESS(doc.get_array().get(arr));

  double nan = std::numeric_limits<double>::quiet_NaN();
  std::array<double, 8> expected_values{5, nan, nan, nan, nan, nan, nan, 1.25};

  size_t index = 0;
  for (auto val : arr) {
    if (index == expected_values.size()) {
      TEST_FAIL("Array contained more values than expected");
    }

    double parsed;
    ASSERT_SUCCESS(val.get_double().get(parsed));
    double expected = expected_values[index];
    if (std::isnan(expected)) {
      ASSERT_TRUE(std::isnan(parsed))
    } else {
      ASSERT_EQUAL(parsed, expected_values[index]);
    }

    index++;
  }

  ASSERT_EQUAL(index, expected_values.size());
  TEST_SUCCEED();
}

bool infinity_in_array() {
  TEST_START();
  dom::parser parser;
  auto json = R"([1,
      infinity,
      INFINITY,
      Infinity,
      inf,
      Inf,
      INF,
      -infinity,
      -INFINITY,
      -Infinity,
      -inf,
      -Inf,
      -INF,
      6.5])"_padded;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(json).get(doc));
  dom::array arr;
  ASSERT_SUCCESS(doc.get_array().get(arr));

  double inf = std::numeric_limits<double>::infinity();
  std::array<double, 14> expected_values{
      1, inf, inf, inf, inf, inf, inf, -inf, -inf, -inf, -inf, -inf, -inf, 6.5};

  size_t index = 0;
  for (auto val : arr) {
    if (index == expected_values.size()) {
      TEST_FAIL("Array contained more values than expected");
    }

    double parsed;
    ASSERT_SUCCESS(val.get_double().get(parsed));
    ASSERT_EQUAL(parsed, expected_values[index]);
    index++;
  }

  ASSERT_EQUAL(index, expected_values.size());
  TEST_SUCCEED();
}

bool nan_in_object() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(R"({"a": NaN, "b": nan})"_padded).get(doc));
  double a;
  ASSERT_SUCCESS(doc["a"].get_double().get(a));
  ASSERT_TRUE(std::isnan(a));
  double b;
  ASSERT_SUCCESS(doc["b"].get_double().get(b));
  ASSERT_TRUE(std::isnan(b));
  TEST_SUCCEED();
}

bool infinity_in_object() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(R"({"a": Infinity, "b": -inf})"_padded).get(doc));
  double a;
  ASSERT_SUCCESS(doc["a"].get_double().get(a));
  ASSERT_TRUE(std::isinf(a));
  ASSERT_TRUE(a > 0);
  double b;
  ASSERT_SUCCESS(doc["b"].get_double().get(b));
  ASSERT_TRUE(std::isinf(b));
  ASSERT_TRUE(b < 0);
  TEST_SUCCEED();
}

// Bad 'Infinity' atoms should yield TAPE_ERROR (extension, not canonical).
// Bad 'NaN' atoms (capital 'N') should yield TAPE_ERROR.
// Bad 'nan' atoms (lowercase 'n') should yield N_ATOM_ERROR (shares the
// 'null' dispatch, so a bad 'nan' reports the same error as bad 'null').
// Bad negative atoms (any case) should yield NUMBER_ERROR: a leading '-'
// routes through parse_number, which falls back to compute_nan_inf and
// returns NUMBER_ERROR if that fails.
//
// Each reject_* test runs the cases in three contexts: at the document root
// (visit_root_primitive), inside an array, and inside an object
// (visit_primitive). Both dispatch paths must agree on error codes.

padded_string wrap_in_array(const char *atom) {
  return padded_string(std::string("[") + atom + "]");
}

padded_string wrap_in_object(const char *atom) {
  return padded_string(std::string("{\"key\": ") + atom + "}");
}

bool reject_trailing_junk() {
  TEST_START();
  struct {
    const char *json;
    error_code expected;
  } cases[] = {
      {"NaNa", TAPE_ERROR},         {"NaN1", TAPE_ERROR},
      {"nana", N_ATOM_ERROR},       {"InfX", TAPE_ERROR},
      {"Inf_", TAPE_ERROR},         {"Infinityy", TAPE_ERROR},
      {"InfinityX", TAPE_ERROR},    {"-NaNa", NUMBER_ERROR},
      {"-nana", NUMBER_ERROR},      {"-InfX", NUMBER_ERROR},
      {"-Infinityy", NUMBER_ERROR},
  };
  for (auto c : cases) {
    dom::parser parser;
    dom::element doc;
    auto err = parser.parse(padded_string(c.json, strlen(c.json))).get(doc);
    ASSERT_ERROR(err, c.expected);
  }
  for (auto c : cases) {
    dom::parser parser;
    dom::element doc;
    auto err = parser.parse(wrap_in_array(c.json)).get(doc);
    ASSERT_ERROR(err, c.expected);
  }
  for (auto c : cases) {
    dom::parser parser;
    dom::element doc;
    auto err = parser.parse(wrap_in_object(c.json)).get(doc);
    ASSERT_ERROR(err, c.expected);
  }
  TEST_SUCCEED();
}

bool reject_similar_prefix() {
  TEST_START();
  struct {
    const char *json;
    error_code expected;
  } cases[] = {
      {"Nope", TAPE_ERROR},      {"Napalm", TAPE_ERROR},
      {"nope", N_ATOM_ERROR},    {"Infant", TAPE_ERROR},
      {"Inform", TAPE_ERROR},    {"Information", TAPE_ERROR},
      {"-Nope", NUMBER_ERROR},   {"-nope", NUMBER_ERROR},
      {"-Infant", NUMBER_ERROR}, {"-Information", NUMBER_ERROR},
  };
  for (auto c : cases) {
    dom::parser parser;
    dom::element doc;
    auto err = parser.parse(padded_string(c.json, strlen(c.json))).get(doc);
    ASSERT_ERROR(err, c.expected);
  }
  for (auto c : cases) {
    dom::parser parser;
    dom::element doc;
    auto err = parser.parse(wrap_in_array(c.json)).get(doc);
    ASSERT_ERROR(err, c.expected);
  }
  for (auto c : cases) {
    dom::parser parser;
    dom::element doc;
    auto err = parser.parse(wrap_in_object(c.json)).get(doc);
    ASSERT_ERROR(err, c.expected);
  }
  TEST_SUCCEED();
}

bool reject_truncated_atoms() {
  TEST_START();
  struct {
    const char *json;
    error_code expected;
  } cases[] = {
      {"N", TAPE_ERROR},    {"Na", TAPE_ERROR},    {"na", N_ATOM_ERROR},
      {"I", TAPE_ERROR},    {"In", TAPE_ERROR},    {"Infinit", TAPE_ERROR},
      {"-N", NUMBER_ERROR}, {"-Na", NUMBER_ERROR}, {"-na", NUMBER_ERROR},
      {"-I", NUMBER_ERROR}, {"-In", NUMBER_ERROR}, {"-Infinit", NUMBER_ERROR},
  };
  for (auto c : cases) {
    dom::parser parser;
    dom::element doc;
    auto err = parser.parse(padded_string(c.json, strlen(c.json))).get(doc);
    ASSERT_ERROR(err, c.expected);
  }
  for (auto c : cases) {
    dom::parser parser;
    dom::element doc;
    auto err = parser.parse(wrap_in_array(c.json)).get(doc);
    ASSERT_ERROR(err, c.expected);
  }
  for (auto c : cases) {
    dom::parser parser;
    dom::element doc;
    auto err = parser.parse(wrap_in_object(c.json)).get(doc);
    ASSERT_ERROR(err, c.expected);
  }
  TEST_SUCCEED();
}

// DOM printer (to_string / minify / prettify) tests. When NaN/Infinity
// parsing is enabled, the writer must emit the same literals on output so
// that round-tripping through the parser preserves the value.

bool print_nan() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse("NaN"_padded).get(doc));
  ASSERT_EQUAL(simdjson::to_string(doc), "NaN");
  ASSERT_EQUAL(simdjson::minify(doc), "NaN");
  TEST_SUCCEED();
}

bool print_infinity() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse("Infinity"_padded).get(doc));
  ASSERT_EQUAL(simdjson::to_string(doc), "Infinity");
  ASSERT_EQUAL(simdjson::minify(doc), "Infinity");
  TEST_SUCCEED();
}

bool print_negative_infinity() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse("-Infinity"_padded).get(doc));
  ASSERT_EQUAL(simdjson::to_string(doc), "-Infinity");
  ASSERT_EQUAL(simdjson::minify(doc), "-Infinity");
  TEST_SUCCEED();
}

bool print_nan_in_array() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(
      parser.parse("[1.5, NaN, Infinity, -Infinity, 2.5]"_padded).get(doc));
  ASSERT_EQUAL(simdjson::to_string(doc), "[1.5,NaN,Infinity,-Infinity,2.5]");
  TEST_SUCCEED();
}

bool print_nan_in_object() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(
      parser.parse(R"({"a": NaN, "b": Infinity, "c": -Infinity})"_padded)
          .get(doc));
  ASSERT_EQUAL(simdjson::to_string(doc),
               "{\"a\":NaN,\"b\":Infinity,\"c\":-Infinity}");
  TEST_SUCCEED();
}

bool print_roundtrip() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse("[NaN, Infinity, -Infinity]"_padded).get(doc));
  std::string serialized = simdjson::to_string(doc);

  dom::parser parser2;
  dom::element doc2;
  ASSERT_SUCCESS(parser2.parse(padded_string(serialized)).get(doc2));
  dom::array arr;
  ASSERT_SUCCESS(doc2.get_array().get(arr));

  std::array<double, 3> expected{
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),
  };
  size_t index = 0;
  for (auto val : arr) {
    double parsed;
    ASSERT_SUCCESS(val.get_double().get(parsed));
    if (std::isnan(expected[index])) {
      ASSERT_TRUE(std::isnan(parsed));
    } else {
      ASSERT_EQUAL(parsed, expected[index]);
    }
    index++;
  }
  ASSERT_EQUAL(index, expected.size());
  TEST_SUCCEED();
}

// FracturedJson aligns values into columns in table mode. The column width
// is driven by the estimator for unseen elements and by measure_value_length
// for the chosen cells; if either undercounts NaN/Infinity, column 1's
// padding won't match column 2's emitted width and rows visibly misalign.
// Force table mode with min_table_rows = 2 and max_inline_length = 0.

bool table_aligns_nan() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(
      parser.parse(R"([{"a": 1, "b": 1},{"a": NaN, "b": 1}])"_padded).get(doc));
  fractured_json_options opts;
  opts.min_table_rows = 2;
  opts.max_inline_length = 0;
  ASSERT_EQUAL(simdjson::fractured_json(doc, opts),
               "[\n"
               "    { \"a\": 1  , \"b\": 1 },\n"
               "    { \"a\": NaN, \"b\": 1 }\n"
               "]");
  TEST_SUCCEED();
}

bool table_aligns_inf() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(
      parser.parse(R"([{"a": 1, "b": 1},{"a": Infinity, "b": 1}])"_padded)
          .get(doc));
  fractured_json_options opts;
  opts.min_table_rows = 2;
  opts.max_inline_length = 0;
  ASSERT_EQUAL(simdjson::fractured_json(doc, opts),
               "[\n"
               "    { \"a\": 1       , \"b\": 1 },\n"
               "    { \"a\": Infinity, \"b\": 1 }\n"
               "]");
  TEST_SUCCEED();
}

bool table_aligns_neg_inf() {
  TEST_START();
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(
      parser.parse(R"([{"a": 1, "b": 1},{"a": -Infinity, "b": 1}])"_padded)
          .get(doc));
  fractured_json_options opts;
  opts.min_table_rows = 2;
  opts.max_inline_length = 0;
  ASSERT_EQUAL(simdjson::fractured_json(doc, opts),
               "[\n"
               "    { \"a\": 1        , \"b\": 1 },\n"
               "    { \"a\": -Infinity, \"b\": 1 }\n"
               "]");
  TEST_SUCCEED();
}

bool run() {
  return parse_nan()                  //
         && parse_infinity()          //
         && parse_negative_infinity() //
         && nan_in_array()            //
         && infinity_in_array()       //
         && nan_in_object()           //
         && infinity_in_object()      //
         && reject_trailing_junk()    //
         && reject_similar_prefix()   //
         && reject_truncated_atoms()  //
         && print_nan()               //
         && print_infinity()          //
         && print_negative_infinity() //
         && print_nan_in_array()      //
         && print_nan_in_object()     //
         && print_roundtrip()         //
         && table_aligns_nan()        //
         && table_aligns_inf()        //
         && table_aligns_neg_inf()    //
      ;
}

#else // !SIMDJSON_ENABLE_NAN_INF

bool run() {
  std::cout << "NaN/Infinity parsing is disabled (SIMDJSON_ENABLE_NAN_INF=0), "
               "skipping tests."
            << std::endl;
  return true;
}

#endif // SIMDJSON_ENABLE_NAN_INF
} // namespace nan_inf_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, nan_inf_tests::run);
}
