#include "simdjson.h"
#include "test_builder.h"
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#if SIMDJSON_SUPPORTS_RANGES
#include <ranges>
#endif

using namespace simdjson;

struct Car {
  std::string make;
  std::string model;
  int64_t year;
  std::vector<double> tire_pressure;
}; // Car

#if SIMDJSON_SUPPORTS_CONCEPTS
struct Car2549 {
  std::string make;
  std::string model;
  int64_t year;
  std::vector<float> tire_pressure;
};
namespace simdjson {
// we intentionally pass by non-const reference to car.
template <typename builder_type>
void tag_invoke(serialize_tag, builder_type &builder, Car2549 &car) {
  builder.start_object();
  builder.append_key_value("make", car.make);
  builder.append_comma();
  builder.append_key_value("model", car.model);
  builder.append_comma();
  builder.append_key_value("year", car.year);
  builder.append_comma();
  builder.append_key_value("tire_pressure", car.tire_pressure);
  builder.end_object();
}
} // namespace simdjson

static_assert(simdjson::require_custom_serialization<Car2549>);
#endif

namespace builder_tests {
using namespace std;

bool allchar_test() {
  TEST_START();
  auto get_utf8_codepoints = []() -> std::string {
    std::string result;
    for (char32_t cp = 0; cp <= 0x10FFFF; ++cp) {
      if ((cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
        continue; // Skip surrogate pairs and invalid codepoints
      }
      if (cp < 0x80) {
        result += static_cast<char>(cp);
      } else if (cp < 0x800) {
        result += static_cast<char>(0xC0 | (cp >> 6));
        result += static_cast<char>(0x80 | (cp & 0x3F));
      } else if (cp < 0x10000) {
        result += static_cast<char>(0xE0 | (cp >> 12));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
      } else {
        result += static_cast<char>(0xF0 | (cp >> 18));
        result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
      }
    }
    return result;
  };
  auto allutf8 = get_utf8_codepoints();
  simdjson::builder::string_builder sb;
  sb.start_object();
  sb.append_key_value("input", allutf8);
  sb.end_object();
  std::string_view p;
  ASSERT_TRUE(sb.validate_unicode());
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  simdjson::padded_string output = p;
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(output).get(doc));
  std::string_view recovered;
  ASSERT_SUCCESS(doc["input"].get(recovered));
  ASSERT_EQUAL(recovered, allutf8);
  simdjson::dom::parser domparser;
  simdjson::dom::element elem;
  ASSERT_SUCCESS(domparser.parse(output).get(elem));
  ASSERT_SUCCESS(elem["input"].get(recovered));
  ASSERT_EQUAL(recovered, allutf8);
  TEST_SUCCEED();
}

bool bad_utf8_test() {
  TEST_START();
  std::string bad_utf8 = "\xFF";
  simdjson::builder::string_builder sb;
  sb.start_object();
  sb.append_key_value("input", bad_utf8);
  sb.end_object();
  ASSERT_FALSE(sb.validate_unicode())
  TEST_SUCCEED();
}
#if SIMDJSON_EXCEPTIONS
bool string_convertion_except() {
  TEST_START();
  simdjson::ondemand::parser p;
  simdjson::builder::string_builder sb;
  sb.append('a');
  std::string r(sb);
  ASSERT_EQUAL(r, "a");

  // forcing call to `operator std::string()`
  r = [](std::string x) { return x; }(sb);
  ASSERT_EQUAL(r, "a");

  TEST_SUCCEED();
}
#endif

bool append_char() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append('a');
  ASSERT_EQUAL(sb.size(), 1);
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "a");
  TEST_SUCCEED();
}

bool append_integer() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append(42);
  ASSERT_EQUAL(sb.size(), 2);
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "42");
  TEST_SUCCEED();
}

bool append_float() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append(1.1);
  ASSERT_EQUAL(sb.size(), 3);
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "1.1");
  TEST_SUCCEED();
}

#if SIMDJSON_ENABLE_NAN_INF
bool append_nan() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append(std::numeric_limits<double>::quiet_NaN());
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "NaN");
  TEST_SUCCEED();
}

bool append_positive_infinity() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append(std::numeric_limits<double>::infinity());
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "Infinity");
  TEST_SUCCEED();
}

bool append_negative_infinity() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append(-std::numeric_limits<double>::infinity());
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "-Infinity");
  TEST_SUCCEED();
}

bool append_float_nan_inf() {
  TEST_START();
  {
    simdjson::builder::string_builder sb;
    sb.append(std::numeric_limits<float>::quiet_NaN());
    std::string_view p;
    ASSERT_SUCCESS(sb.view().get(p));
    ASSERT_EQUAL(p, "NaN");
  }
  {
    simdjson::builder::string_builder sb;
    sb.append(std::numeric_limits<float>::infinity());
    std::string_view p;
    ASSERT_SUCCESS(sb.view().get(p));
    ASSERT_EQUAL(p, "Infinity");
  }
  {
    simdjson::builder::string_builder sb;
    sb.append(-std::numeric_limits<float>::infinity());
    std::string_view p;
    ASSERT_SUCCESS(sb.view().get(p));
    ASSERT_EQUAL(p, "-Infinity");
  }
  TEST_SUCCEED();
}

bool nan_inf_in_array() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.start_array();
  sb.append(1.5);
  sb.append_comma();
  sb.append(std::numeric_limits<double>::quiet_NaN());
  sb.append_comma();
  sb.append(std::numeric_limits<double>::infinity());
  sb.append_comma();
  sb.append(-std::numeric_limits<double>::infinity());
  sb.append_comma();
  sb.append(2.5);
  sb.end_array();
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "[1.5,NaN,Infinity,-Infinity,2.5]");
  TEST_SUCCEED();
}

bool nan_inf_in_object() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.start_object();
  sb.append_key_value("a", std::numeric_limits<double>::quiet_NaN());
  sb.append_comma();
  sb.append_key_value("b", std::numeric_limits<double>::infinity());
  sb.append_comma();
  sb.append_key_value("c", -std::numeric_limits<double>::infinity());
  sb.end_object();
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "{\"a\":NaN,\"b\":Infinity,\"c\":-Infinity}");
  TEST_SUCCEED();
}

bool nan_inf_roundtrip() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.start_array();
  sb.append(std::numeric_limits<double>::quiet_NaN());
  sb.append_comma();
  sb.append(std::numeric_limits<double>::infinity());
  sb.append_comma();
  sb.append(-std::numeric_limits<double>::infinity());
  sb.end_array();
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));

  simdjson::padded_string output{p};
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  ASSERT_SUCCESS(parser.parse(output).get(doc));
  simdjson::dom::array arr;
  ASSERT_SUCCESS(doc.get_array().get(arr));

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
#endif // SIMDJSON_ENABLE_NAN_INF

bool append_null() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append_null();
  ASSERT_EQUAL(sb.size(), 4);
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "null");
  TEST_SUCCEED();
}

bool clear() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append('a');
  sb.clear();
  ASSERT_EQUAL(sb.size(), 0);
  TEST_SUCCEED();
}

bool escape_and_append() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.escape_and_append("Hello, \"world\"!");
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "Hello, \\\"world\\\"!");
  TEST_SUCCEED();
}

bool escape_and_append_with_quotes() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.escape_and_append_with_quotes("Hello, \"world\"!");
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "\"Hello, \\\"world\\\"!\"");
  TEST_SUCCEED();
}

// Scalar reference for JSON string-content escaping (matches escape_json_char).
static std::string reference_escape(std::string_view in) {
  static const char *const ctrl[32] = {
      "\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005", "\\u0006",
      "\\u0007", "\\b",     "\\t",     "\\n",     "\\u000b", "\\f",     "\\r",
      "\\u000e", "\\u000f", "\\u0010", "\\u0011", "\\u0012", "\\u0013", "\\u0014",
      "\\u0015", "\\u0016", "\\u0017", "\\u0018", "\\u0019", "\\u001a", "\\u001b",
      "\\u001c", "\\u001d", "\\u001e", "\\u001f"};
  std::string out;
  out.reserve(in.size() * 2);
  for (unsigned char c : in) {
    if (c == '"') {
      out += "\\\"";
    } else if (c == '\\') {
      out += "\\\\";
    } else if (c < 32) {
      out += ctrl[c];
    } else {
      out.push_back(char(c));
    }
  }
  return out;
}

static bool check_escape_matches_reference(std::string_view in) {
  simdjson::builder::string_builder sb;
  sb.escape_and_append(in);
  std::string_view got;
  if (sb.view().get(got)) {
    std::cerr << "FAIL: escape_and_append view() error for len=" << in.size()
              << std::endl;
    return false;
  }
  const std::string expected = reference_escape(in);
  if (got != expected) {
    std::cerr << "FAIL: write_string_escaped mismatch for len=" << in.size()
              << std::endl;
    return false;
  }
  return true;
}

// Exercises every length class of the block-escape path (scalar <4, 4x2, 8x2,
// full 16-byte blocks + last-16 tail), single quotable positions, all control
// characters, dense escapes, and random mixes against a scalar reference.
bool escape_write_string_escaped_exhaustive() {
  TEST_START();

  for (size_t n = 0; n <= 48; n++) {
    ASSERT_TRUE(check_escape_matches_reference(std::string(n, 'a')));
  }

  const char specials[] = {'"',  '\\', '\n', '\t', '\r',
                           '\b', '\f', char(0), char(1), char(31)};
  for (size_t n = 1; n <= 40; n++) {
    for (size_t pos = 0; pos < n; pos++) {
      for (char sp : specials) {
        std::string s(n, 'x');
        s[pos] = sp;
        ASSERT_TRUE(check_escape_matches_reference(s));
      }
    }
  }

  for (int c = 0; c < 32; c++) {
    ASSERT_TRUE(check_escape_matches_reference(std::string(1, char(c))));
  }

  ASSERT_TRUE(check_escape_matches_reference(std::string(32, '"')));
  ASSERT_TRUE(check_escape_matches_reference(std::string(32, '\\')));
  ASSERT_TRUE(check_escape_matches_reference(std::string(17, '\n')));

  // Deterministic LCG so CI is reproducible without depending on rand().
  uint32_t state = 1;
  auto next = [&state]() -> uint32_t {
    state = state * 1664525u + 1013904223u;
    return state;
  };
  for (int t = 0; t < 2000; t++) {
    const size_t n = size_t(next() % 64);
    std::string s(n, '\0');
    for (size_t i = 0; i < n; i++) {
      s[i] = char(next() & 0xff);
    }
    ASSERT_TRUE(check_escape_matches_reference(s));
  }

  // High bytes must pass through unescaped; mix with a quotable mid-string.
  ASSERT_TRUE(check_escape_matches_reference(
      std::string("\xc3\xa9\xe2\x82\xac\xf0\x9f\x98\x80")));
  {
    std::string mixed(20, char(0x80));
    mixed.push_back('"');
    mixed.append(5, char(0xff));
    ASSERT_TRUE(check_escape_matches_reference(mixed));
  }

  TEST_SUCCEED();
}

bool various_integers() {
  TEST_START();
  std::vector<std::pair<int64_t, std::string_view>> test_cases = {
      {0, "0"},
      {1, "1"},
      {-1, "-1"},
      {42, "42"},
      {-42, "-42"},
      {100, "100"},
      {-100, "-100"},
      {999, "999"},
      {-999, "-999"},
      {2147483647, "2147483647"},    // max 32-bit integer
      {-2147483648, "-2147483648"},  // min 32-bit integer
      {4294967296ULL, "4294967296"}, // 2^32
      {-4294967296LL, "-4294967296"},
      {10000000000LL, "10000000000"}, // 10 billion
      {-10000000000LL, "-10000000000"},
      {9223372036854775807LL, "9223372036854775807"}, // max 64-bit integer
      {-9223372036854775807LL - 1,
       "-9223372036854775808"}, // min 64-bit integer
      {1234567890123LL, "1234567890123"},
      {-1234567890123LL, "-1234567890123"},
  };
  for (const auto &[value, expected] : test_cases) {
    simdjson::builder::string_builder sb;
    sb.append(value);
    std::string_view p;
    auto result = sb.view().get(p);
    ASSERT_SUCCESS(result);
    ASSERT_EQUAL(p, expected);
  }
  TEST_SUCCEED();
}

bool various_unsigned_integers() {
  TEST_START();
  std::vector<std::pair<uint64_t, std::string_view>> test_cases = {
      {0, "0"},
      {1, "1"},
      {42, "42"},
      {100, "100"},
      {999, "999"},
      {2147483647, "2147483647"},                     // max 32-bit integer
      {4294967296ULL, "4294967296"},                  // 2^32
      {10000000000LL, "10000000000"},                 // 10 billion
      {9223372036854775807LL, "9223372036854775807"}, // max 64-bit integer
      {1234567890123LL, "1234567890123"},
  };
  for (const auto &[value, expected] : test_cases) {
    simdjson::builder::string_builder sb;
    sb.append(value);
    std::string_view p;
    auto result = sb.view().get(p);
    ASSERT_SUCCESS(result);
    ASSERT_EQUAL(p, expected);
  }
  TEST_SUCCEED();
}

bool append_raw() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append_raw("Test");
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "Test");
  TEST_SUCCEED();
}

bool raw_with_length() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append_raw("Test String", 4);
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "Test");
  TEST_SUCCEED();
}

bool string_convertion() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append('a');
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "a");
  TEST_SUCCEED();
}

bool unicode_validation() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append('a');
  ASSERT_TRUE(sb.validate_unicode());
  TEST_SUCCEED();
}

bool buffer_growth() {
  TEST_START();
  simdjson::builder::string_builder sb;
  for (int i = 0; i < 3; ++i) {
    sb.append('a');
  }
  ASSERT_EQUAL(sb.size(), 3);
  TEST_SUCCEED();
}

void serialize_car_long(const Car &car,
                        simdjson::builder::string_builder &builder) {
  // start of JSON
  builder.append_raw("{");

  // "make"
  builder.escape_and_append_with_quotes("make");
  builder.append_raw(":");
  builder.escape_and_append_with_quotes(car.make);

  // "model"
  builder.append_raw(",");
  builder.escape_and_append_with_quotes("model");
  builder.append_raw(":");
  builder.escape_and_append_with_quotes(car.model);

  // "year"
  builder.append_raw(",");
  builder.escape_and_append_with_quotes("year");
  builder.append_raw(":");
  builder.append(car.year);

  // "tire_pressure"
  builder.append_raw(",");
  builder.escape_and_append_with_quotes("tire_pressure");
  builder.append_raw(":[");

  // vector tire_pressure
  for (size_t i = 0; i < car.tire_pressure.size(); ++i) {
    builder.append(car.tire_pressure[i]);
    if (i < car.tire_pressure.size() - 1) {
      builder.append_raw(",");
    }
  }
  // end of array
  builder.append_raw("]");

  // end of object
  builder.append_raw("}");
}

bool car_test_long() {
  TEST_START();
  simdjson::builder::string_builder sb;
  Car c = {"Toyota", "Corolla", 2017, {30.0, 30.2, 30.513, 30.79}};
  serialize_car_long(c, sb);
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,"
                  "\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
  TEST_SUCCEED();
}

void serialize_car(const Car &car, simdjson::builder::string_builder &builder) {
  // start of JSON
  builder.start_object();

  // "make"
  builder.append_key_value("make", car.make);
  builder.append_comma();

  // "model"
  builder.append_key_value("model", car.model);

  builder.append_comma();

  // "year"
  builder.append_key_value("year", car.year);

  builder.append_comma();

  // "tire_pressure"
  builder.escape_and_append_with_quotes("tire_pressure");
  builder.append_colon();
  builder.start_array();
  // vector tire_pressure
  for (size_t i = 0; i < car.tire_pressure.size(); ++i) {
    builder.append(car.tire_pressure[i]);
    if (i < car.tire_pressure.size() - 1) {
      builder.append_comma();
    }
  }
  // end of array
  builder.end_array();

  // end of object
  builder.end_object();
}

#if SIMDJSON_SUPPORTS_CONCEPTS
void serialize_car_template(const Car &car,
                            simdjson::builder::string_builder &builder) {
  // start of JSON
  builder.start_object();

  // "make"
  builder.append_key_value<"make">(car.make);
  builder.append_comma();

  // "model"
  builder.append_key_value<"model">(car.model);

  builder.append_comma();

  // "year"
  builder.append_key_value<"year">(car.year);

  builder.append_comma();

  // "tire_pressure"
  builder.escape_and_append_with_quotes<"tire_pressure">();
  builder.append_colon();
  builder.start_array();
  // vector tire_pressure
  for (size_t i = 0; i < car.tire_pressure.size(); ++i) {
    builder.append(car.tire_pressure[i]);
    if (i < car.tire_pressure.size() - 1) {
      builder.append_comma();
    }
  }
  // end of array
  builder.end_array();

  // end of object
  builder.end_object();
}
#endif
bool car_test() {
  TEST_START();
  simdjson::builder::string_builder sb;
  Car c = {"Toyota", "Corolla", 2017, {30.0, 30.2, 30.513, 30.79}};
  serialize_car(c, sb);
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,"
                  "\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
  TEST_SUCCEED();
}
#if SIMDJSON_SUPPORTS_CONCEPTS

bool issue2549() {
  TEST_START();
  simdjson::builder::string_builder sb;
  Car2549 c = {"Toyota", "Corolla", 2017, {1.0f, 2.0f, 3.0f}};
  sb.start_object();
  sb.append_key_value("car", c);
  sb.end_object();
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "{\"car\":{\"make\":\"Toyota\",\"model\":\"Corolla\","
                  "\"year\":2017,\"tire_pressure\":[1.0,2.0,3.0]}}");
  TEST_SUCCEED();
}

bool car_test_template() {
  TEST_START();
  simdjson::builder::string_builder sb;
  Car c = {"Toyota", "Corolla", 2017, {30.0, 30.2, 30.513, 30.79}};
  serialize_car_template(c, sb);
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,"
                  "\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
  TEST_SUCCEED();
}
bool serialize_optional() {
  TEST_START();
  simdjson::builder::string_builder sb;
  std::optional<std::string> optional_string = "Hello, World!";
  sb.append(optional_string);
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "\"Hello, World!\"");
  sb.clear();
  std::optional<std::string> optional_string_null = std::nullopt;
  sb.append(optional_string_null);
  result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "null");
  TEST_SUCCEED();
}

// https://github.com/simdjson/simdjson/issues/2827
// Since C++26 (P3168), std::optional is itself a range, so an optional
// used to match both the container and the optional serialization overloads.
// These calls must compile and pick the optional overload.
bool issue2827_optional_serialization() {
  TEST_START();
  std::string json;

  std::optional<std::string> optional_string = "bar";
  ASSERT_SUCCESS(simdjson::to_json(optional_string, json));
  ASSERT_EQUAL(json, "\"bar\"");

  std::optional<std::string> empty_string = std::nullopt;
  ASSERT_SUCCESS(simdjson::to_json(empty_string, json));
  ASSERT_EQUAL(json, "null");

  std::optional<int> optional_int = 3;
  ASSERT_SUCCESS(simdjson::to_json(optional_int, json));
  ASSERT_EQUAL(json, "3");

  std::optional<int> empty_int = std::nullopt;
  ASSERT_SUCCESS(simdjson::to_json(empty_int, json));
  ASSERT_EQUAL(json, "null");

  std::vector<std::optional<int>> vector_of_optionals = {1, std::nullopt, 3};
  ASSERT_SUCCESS(simdjson::to_json(vector_of_optionals, json));
  ASSERT_EQUAL(json, "[1,null,3]");

  std::optional<std::vector<int>> optional_vector = std::vector<int>{1, 2};
  ASSERT_SUCCESS(simdjson::to_json(optional_vector, json));
  ASSERT_EQUAL(json, "[1,2]");

  std::optional<std::vector<int>> empty_vector = std::nullopt;
  ASSERT_SUCCESS(simdjson::to_json(empty_vector, json));
  ASSERT_EQUAL(json, "null");
  TEST_SUCCEED();
}
#endif

#if SIMDJSON_SUPPORTS_RANGES && SIMDJSON_SUPPORTS_CONCEPTS
void serialize_car_simple(const Car &car,
                          simdjson::builder::string_builder &builder) {
  builder.start_object();
  builder.append_key_value("make", car.make);
  builder.append_comma();
  builder.append_key_value("model", car.model);
  builder.append_comma();
  builder.append_key_value("year", car.year);
  builder.append_comma();
  builder.append_key_value("tire_pressure", car.tire_pressure);
  builder.end_object();
}
bool car_test_simple() {
  TEST_START();
  simdjson::builder::string_builder sb;
  Car c = {"Toyota", "Corolla", 2017, {30.0, 30.2, 30.513, 30.79}};
  serialize_car(c, sb);
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,"
                  "\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
  TEST_SUCCEED();
}

bool car_test_simple_complete() {
  TEST_START();
  Car c = {"Toyota", "Corolla", 2017, {30.0, 30.2, 30.513, 30.79}};
  simdjson::builder::string_builder sb;
  sb.start_object();
  sb.append_key_value("make", c.make);
  sb.append_comma();
  sb.append_key_value("model", c.model);
  sb.append_comma();
  sb.append_key_value("year", c.year);
  sb.append_comma();
  sb.append_key_value("tire_pressure", c.tire_pressure);
  sb.end_object();
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,"
                  "\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
  TEST_SUCCEED();
}

bool map_test() {
  TEST_START();
  std::map<std::string, double> c = {{"key1", 1}, {"key2", 1}};
  simdjson::builder::string_builder sb;
  sb.append(c);
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "{\"key1\":1.0,\"key2\":1.0}");
  std::string s;

  ASSERT_SUCCESS(simdjson::to_json(c).get(s));
  ASSERT_EQUAL(s, "{\"key1\":1.0,\"key2\":1.0}");
  TEST_SUCCEED();
}
bool ranges_test() {
  TEST_START();
  struct Foo {
    int a;
    float b;
  };
  std::vector<Foo> c = {{1, 2.0f}, {3, 4.0f}, {5, 6.0f}, {7, 8.0f}};
  simdjson::builder::string_builder sb;
  sb.append(c | std::views::transform(&Foo::b));
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "[2.0,4.0,6.0,8.0]");
  std::string s;
  ASSERT_SUCCESS(simdjson::to_json(c | std::views::transform(&Foo::b)).get(s));
  ASSERT_EQUAL(s, "[2.0,4.0,6.0,8.0]");
  TEST_SUCCEED();
}
bool double_double_test() {
  TEST_START();
  std::vector<std::vector<double>> c = {{1.0, 2.0}, {3.0, 4.0}};
  simdjson::builder::string_builder sb;
  sb.append(c);
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "[[1.0,2.0],[3.0,4.0]]");
  std::string s;
  ASSERT_SUCCESS(simdjson::to_json(c).get(s));
  ASSERT_EQUAL(s, "[[1.0,2.0],[3.0,4.0]]");
  TEST_SUCCEED();
}
bool double_double_test_to_string() {
  TEST_START();
  std::vector<std::vector<double>> c = {{1.0, 2.0}, {3.0, 4.0}};
  simdjson::builder::string_builder sb;
  sb.append(c);
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "[[1.0,2.0],[3.0,4.0]]");
  std::string s;
  simdjson::simdjson_error ec = simdjson::to_json(c, s);
  ASSERT_EQUAL(s, "[[1.0,2.0],[3.0,4.0]]");
  TEST_SUCCEED();
}
#if SIMDJSON_EXCEPTIONS
bool car_test_simple_complete_exceptions() {
  TEST_START();
  Car c = {"Toyota", "Corolla", 2017, {30.0, 30.2, 30.513, 30.79}};
  simdjson::builder::string_builder sb;
  sb.start_object();
  sb.append_key_value("make", c.make);
  sb.append_comma();
  sb.append_key_value("model", c.model);
  sb.append_comma();
  sb.append_key_value("year", c.year);
  sb.append_comma();
  sb.append_key_value("tire_pressure", c.tire_pressure);
  sb.end_object();
  std::string_view p = sb.view();
  ASSERT_EQUAL(p, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,"
                  "\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
  TEST_SUCCEED();
}
#endif // SIMDJSON_EXCEPTIONS
#endif // SIMDJSONS_SUPPORT_RANGES

#if SIMDJSON_EXCEPTIONS
bool car_test_exception() {
  TEST_START();
  simdjson::builder::string_builder sb;
  Car c = {"Toyota", "Corolla", 2017, {30.0, 30.2, 30.513, 30.79}};
  serialize_car_long(c, sb);
  std::string_view p{sb};
  ASSERT_EQUAL(p, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,"
                  "\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
  TEST_SUCCEED();
}
#endif

// Boundary test for the SIMD escaper's capacity handling: dense escape
// patterns (up to 6x expansion) at every offset and length that exercises
// block boundaries, built with initial_capacity 1 so every growth request
// lands on an exact pos+needed boundary. An under-budgeted capacity check
// corrupts memory (caught by sanitizers) or output (caught by the
// byte-for-byte reference comparison below).
bool escape_tight_capacity_boundary() {
  TEST_START();
  auto reference_escape = [](const std::string &in) {
    static const char *ctrl[32] = {
        "\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005",
        "\\u0006", "\\u0007", "\\b",     "\\t",     "\\n",     "\\u000b",
        "\\f",     "\\r",     "\\u000e", "\\u000f", "\\u0010", "\\u0011",
        "\\u0012", "\\u0013", "\\u0014", "\\u0015", "\\u0016", "\\u0017",
        "\\u0018", "\\u0019", "\\u001a", "\\u001b", "\\u001c", "\\u001d",
        "\\u001e", "\\u001f"};
    std::string out = "\"";
    for (unsigned char c : in) {
      if (c == '"') {
        out += "\\\"";
      } else if (c == '\\') {
        out += "\\\\";
      } else if (c < 32) {
        out += ctrl[c];
      } else {
        out += char(c);
      }
    }
    out += '"';
    return out;
  };
  auto check = [&](const std::string &in) -> bool {
    for (size_t initial_capacity : {size_t(1), size_t(1024)}) {
      builder::string_builder sb(initial_capacity);
      sb.escape_and_append_with_quotes(std::string_view(in));
      std::string_view got;
      if (sb.view().get(got)) { return false; }
      if (std::string_view(reference_escape(in)) != got) { return false; }
    }
    return true;
  };
  const char escapes[] = {'\x01', '"', '\\', '\x1f'};
  // Maximum-density expansions (6 bytes per input byte) at every length that
  // exercises the last-chunk repositioning store.
  for (size_t len = 1; len <= 64; len++) {
    ASSERT_TRUE(check(std::string(len, '\x01')));
    ASSERT_TRUE(check(std::string(len, '"')));
  }
  // Dense escapes immediately before the chunk boundary, clean tail after.
  for (size_t lead = 1; lead <= 16; lead++) {
    for (char e : escapes) {
      std::string s(lead, e);
      s += std::string(24, 'a');
      ASSERT_TRUE(check(s));
      std::string t(24, 'a');
      t += std::string(lead, e);
      ASSERT_TRUE(check(t));
    }
  }
  // A single escape at every offset of a two-chunk string.
  for (size_t pos = 0; pos < 33; pos++) {
    for (char e : escapes) {
      std::string s(33, 'b');
      s[pos] = e;
      ASSERT_TRUE(check(s));
    }
  }
  TEST_SUCCEED();
}

bool run() {
  return allchar_test() && bad_utf8_test() && various_integers() &&
         various_unsigned_integers() && car_test_long() && car_test() &&
#if SIMDJSON_EXCEPTIONS
         car_test_exception() && string_convertion_except() &&
#endif
#if SIMDJSON_SUPPORTS_RANGES && SIMDJSON_SUPPORTS_CONCEPTS
         map_test() && ranges_test() && double_double_test() &&
         double_double_test_to_string() && car_test_simple() &&
         car_test_simple_complete() &&
#if SIMDJSON_EXCEPTIONS
         car_test_simple_complete_exceptions() &&
#endif
#endif
#if SIMDJSON_SUPPORTS_CONCEPTS
         issue2549() && car_test_template() && serialize_optional() &&
         issue2827_optional_serialization() &&
#endif
         append_char() && append_integer() && append_float() && append_null() &&
#if SIMDJSON_ENABLE_NAN_INF
         append_nan() && append_positive_infinity() &&
         append_negative_infinity() && append_float_nan_inf() &&
         nan_inf_in_array() && nan_inf_in_object() && nan_inf_roundtrip() &&
#endif
         clear() && escape_and_append() && escape_and_append_with_quotes() &&
         escape_write_string_escaped_exhaustive() &&
         escape_tight_capacity_boundary() && append_raw() &&
         raw_with_length() && string_convertion() && buffer_growth() &&
         unicode_validation() && true;
}

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, builder_tests::run);
}
