#include "simdjson.h"
#include "test_ondemand.h"
#include <array>
#include <cmath>
#include <limits>

using namespace simdjson;

namespace nan_inf_tests {
#if SIMDJSON_ENABLE_NAN_INF

bool parse_nan() {
  TEST_START();
  for (auto json_str : {"NaN", "nan", "-nan", "-NAN"}) {
    ondemand::parser parser;
    padded_string json(json_str, strlen(json_str));
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    double value;
    ASSERT_SUCCESS(doc.get_double().get(value));
    ASSERT_TRUE(std::isnan(value));
  }
  TEST_SUCCEED();
}

bool parse_infinity() {
  TEST_START();
  for (auto json_str : {"Infinity", "inf", "INF", "Inf"}) {
    ondemand::parser parser;
    padded_string json(json_str, strlen(json_str));
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    double value;
    ASSERT_SUCCESS(doc.get_double().get(value));
    ASSERT_TRUE(std::isinf(value));
    ASSERT_TRUE(value > 0);
  }
  TEST_SUCCEED();
}

bool parse_negative_infinity() {
  TEST_START();
  for (auto json_str : {"-Infinity", "-inf", "-INF", "-Inf"}) {
    ondemand::parser parser;
    padded_string json(json_str, strlen(json_str));
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    double value;
    ASSERT_SUCCESS(doc.get_double().get(value));
    ASSERT_TRUE(std::isinf(value));
    ASSERT_TRUE(value < 0);
  }
  TEST_SUCCEED();
}

bool nan_in_array() {
  TEST_START();
  ondemand::parser parser;
  auto json = R"([5, NaN, NAN, nan, -nan, -NAN, -NaN, 1.25])"_padded;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  ondemand::array arr;
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
  ondemand::parser parser;
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
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  ondemand::array arr;
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
  ondemand::parser parser;
  auto json = R"({"a": NaN, "b": nan})"_padded;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  double a;
  ASSERT_SUCCESS(doc["a"].get_double().get(a));
  ASSERT_TRUE(std::isnan(a));
  // rewind to access second field
  doc.rewind();
  double b;
  ASSERT_SUCCESS(doc["b"].get_double().get(b));
  ASSERT_TRUE(std::isnan(b));
  TEST_SUCCEED();
}

bool infinity_in_object() {
  TEST_START();
  ondemand::parser parser;
  auto json = R"({"a": Infinity, "b": -inf})"_padded;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  double a;
  ASSERT_SUCCESS(doc["a"].get_double().get(a));
  ASSERT_TRUE(std::isinf(a));
  ASSERT_TRUE(a > 0);
  doc.rewind();
  double b;
  ASSERT_SUCCESS(doc["b"].get_double().get(b));
  ASSERT_TRUE(std::isinf(b));
  ASSERT_TRUE(b < 0);
  TEST_SUCCEED();
}

bool nan_in_string() {
  TEST_START();
  for (auto json_str : {R"("NaN")", R"("nan")"}) {
    ondemand::parser parser;
    padded_string json(json_str, strlen(json_str));
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    double value;
    ASSERT_SUCCESS(doc.get_double_in_string().get(value));
    ASSERT_TRUE(std::isnan(value));
  }
  TEST_SUCCEED();
}

bool infinity_in_string() {
  TEST_START();
  for (auto json_str : {R"("Infinity")", R"("inf")", R"("INF")", R"("Inf")"}) {
    ondemand::parser parser;
    padded_string json(json_str, strlen(json_str));
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    double value;
    ASSERT_SUCCESS(doc.get_double_in_string().get(value));
    ASSERT_TRUE(std::isinf(value));
    ASSERT_TRUE(value > 0);
  }
  TEST_SUCCEED();
}

bool negative_infinity_in_string() {
  TEST_START();
  for (auto json_str :
       {R"("-Infinity")", R"("-inf")", R"("-INF")", R"("-Inf")"}) {
    ondemand::parser parser;
    padded_string json(json_str, strlen(json_str));
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    double value;
    ASSERT_SUCCESS(doc.get_double_in_string().get(value));
    ASSERT_TRUE(std::isinf(value));
    ASSERT_TRUE(value < 0);
  }
  TEST_SUCCEED();
}

bool nan_inf_in_string_in_array() {
  TEST_START();
  ondemand::parser parser;
  auto json = R"(["NaN", "inf", "-Infinity"])"_padded;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  ondemand::array arr;
  ASSERT_SUCCESS(doc.get_array().get(arr));
  size_t index = 0;
  for (auto val : arr) {
    double parsed;
    ASSERT_SUCCESS(val.get_double_in_string().get(parsed));
    if (index == 0) {
      ASSERT_TRUE(std::isnan(parsed));
    } else if (index == 1) {
      ASSERT_TRUE(std::isinf(parsed));
      ASSERT_TRUE(parsed > 0);
    } else {
      ASSERT_TRUE(std::isinf(parsed));
      ASSERT_TRUE(parsed < 0);
    }
    index++;
  }
  ASSERT_EQUAL(index, 3);
  TEST_SUCCEED();
}

bool nan_inf_in_string_in_object() {
  TEST_START();
  ondemand::parser parser;
  auto json = R"({"a": "NaN", "b": "inf", "c": "-Infinity"})"_padded;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  double a;
  ASSERT_SUCCESS(doc["a"].get_double_in_string().get(a));
  ASSERT_TRUE(std::isnan(a));
  doc.rewind();
  double b;
  ASSERT_SUCCESS(doc["b"].get_double_in_string().get(b));
  ASSERT_TRUE(std::isinf(b));
  ASSERT_TRUE(b > 0);
  doc.rewind();
  double c;
  ASSERT_SUCCESS(doc["c"].get_double_in_string().get(c));
  ASSERT_TRUE(std::isinf(c));
  ASSERT_TRUE(c < 0);
  TEST_SUCCEED();
}

// Bad atom tokens should not be parseable as doubles, and the raw JSON
// token should still be extractable via raw_json_token().
bool reject_trailing_junk() {
  TEST_START();
  for (auto atom :
       {"NaNa", "NaN1", "nana", "InfX", "Inf_", "Infinityy", "InfinityX"}) {
    std::string wrapped = std::string("{\"key\": ") + atom + "}";
    ondemand::parser parser;
    padded_string json(wrapped);
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    auto val = doc["key"];
    double num = 0.0;
    ASSERT_ERROR(val.get(num), INCORRECT_TYPE);
    std::string_view str;
    ASSERT_SUCCESS(val.raw_json_token().get(str));
    ASSERT_EQUAL(str, atom);
  }
  TEST_SUCCEED();
}

bool reject_similar_prefix() {
  TEST_START();
  for (auto atom :
       {"Nope", "Napalm", "nope", "Infant", "Inform", "Information"}) {
    std::string wrapped = std::string("{\"key\": ") + atom + "}";
    ondemand::parser parser;
    padded_string json(wrapped);
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    auto val = doc["key"];
    double num = 0.0;
    ASSERT_ERROR(val.get(num), INCORRECT_TYPE);
    std::string_view str;
    ASSERT_SUCCESS(val.raw_json_token().get(str));
    ASSERT_EQUAL(str, atom);
  }
  TEST_SUCCEED();
}

bool reject_truncated_atoms() {
  TEST_START();
  for (auto atom : {"N", "Na", "na", "I", "In", "Infinit"}) {
    std::string wrapped = std::string("{\"key\": ") + atom + "}";
    ondemand::parser parser;
    padded_string json(wrapped);
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    auto val = doc["key"];
    double num = 0.0;
    ASSERT_ERROR(val.get(num), INCORRECT_TYPE);
    std::string_view str;
    ASSERT_SUCCESS(val.raw_json_token().get(str));
    ASSERT_EQUAL(str, atom);
  }
  TEST_SUCCEED();
}

bool run() {
  return parse_nan()                      //
         && parse_infinity()              //
         && parse_negative_infinity()     //
         && nan_in_array()                //
         && infinity_in_array()           //
         && nan_in_object()               //
         && infinity_in_object()          //
         && nan_in_string()               //
         && infinity_in_string()          //
         && negative_infinity_in_string() //
         && nan_inf_in_string_in_array()  //
         && nan_inf_in_string_in_object() //
         && reject_trailing_junk()        //
         && reject_similar_prefix()       //
         && reject_truncated_atoms()      //
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
