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
#include <span>
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

bool append_double_formatting() {
  TEST_START();
  struct expectation {
    double value;
    const char *text;
  };
  const expectation cases[] = {
      {0.0, "0.0"},
      {-0.0, "-0.0"},
      {1.0, "1.0"},
      {-1.5, "-1.5"},
      {0.1, "0.1"},
      {0.3, "0.3"},
      {0.5, "0.5"},
      {4.35, "4.35"},
      {40.1, "40.1"},
      {100.0, "100.0"},
      {-123.456, "-123.456"},
      {1.0 / 3.0, "0.3333333333333333"},
      {123456789.0, "123456789.0"},
      {1e14, "100000000000000.0"},
      {123456789012345.6, "123456789012345.6"},
      {999999999999999.9, "999999999999999.9"},
      {1e15, "1e+15"},
      {1.5e15, "1.5e+15"},
      {9007199254740993.0, "9.007199254740992e+15"},
      {1e16, "1e+16"},
      {1e21, "1e+21"},
      {1e23, "1e+23"},
      {0.001, "0.001"},
      {0.0001, "0.0001"},
      {0.00012345, "0.00012345"},
      {1e-5, "1e-05"},
      {1.5e-7, "1.5e-07"},
      {1e100, "1e+100"},
      {1e-100, "1e-100"},
      {1.2345678901234567e-300, "1.2345678901234568e-300"},
      {2.2250738585072014e-308, "2.2250738585072014e-308"},
      {2.225073858507201e-308, "2.225073858507201e-308"},
      {5e-324, "5e-324"},
      {1.7976931348623157e308, "1.7976931348623157e+308"},
  };
  for (const expectation &c : cases) {
    simdjson::builder::string_builder sb;
    sb.append(c.value);
    std::string_view p;
    ASSERT_SUCCESS(sb.view().get(p));
    ASSERT_EQUAL(p, c.text);
  }
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
#else
// Without SIMDJSON_ENABLE_NAN_INF, NaN and Infinity are written as null.
bool nan_inf_as_null() {
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
  sb.append(std::numeric_limits<float>::quiet_NaN());
  sb.append_comma();
  sb.append(-std::numeric_limits<float>::infinity());
  sb.end_array();
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "[1.5,null,null,null,null,null]");
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  ASSERT_SUCCESS(parser.parse(p).get(doc));
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

// Every digit count, at the edges where it changes, appended one after the
// other to the same builder.
bool integer_digit_boundaries() {
  TEST_START();
  std::vector<uint64_t> values = {0, (std::numeric_limits<uint64_t>::max)(),
                                  (std::numeric_limits<uint64_t>::max)() - 1};
  uint64_t power = 1;
  for (int digits = 1; digits <= 20; digits++) {
    values.push_back(power - 1);
    values.push_back(power);
    values.push_back(power + 1);
    if (digits < 20) {
      power *= 10;
    }
  }
  for (int bits = 0; bits < 64; bits++) {
    values.push_back((uint64_t(1) << bits) - 1);
    values.push_back(uint64_t(1) << bits);
  }
  simdjson::builder::string_builder unsigned_sb;
  simdjson::builder::string_builder signed_sb;
  std::string unsigned_expected;
  std::string signed_expected;
  for (uint64_t value : values) {
    unsigned_sb.append(value);
    unsigned_sb.append_comma();
    unsigned_expected.append(std::to_string(value));
    unsigned_expected.push_back(',');
    if (value <= uint64_t((std::numeric_limits<int64_t>::max)())) {
      signed_sb.append(int64_t(value));
      signed_sb.append_comma();
      signed_sb.append(-int64_t(value));
      signed_sb.append_comma();
      signed_expected.append(std::to_string(int64_t(value)));
      signed_expected.push_back(',');
      signed_expected.append(std::to_string(-int64_t(value)));
      signed_expected.push_back(',');
    }
  }
  signed_sb.append((std::numeric_limits<int64_t>::min)());
  signed_expected.append(std::to_string((std::numeric_limits<int64_t>::min)()));
  std::string_view p;
  ASSERT_SUCCESS(unsigned_sb.view().get(p));
  ASSERT_EQUAL(p, unsigned_expected);
  ASSERT_SUCCESS(signed_sb.view().get(p));
  ASSERT_EQUAL(p, signed_expected);
  TEST_SUCCEED();
}

bool move_to_string() {
  TEST_START();
  simdjson::builder::string_builder sb(4); // small, so that we grow
  std::string expected;
  for (size_t i = 0; i < 1000; i++) {
    sb.append(uint64_t(i));
    sb.append_comma();
    expected += std::to_string(i) + ",";
  }
  std::string out = "previous content";
  ASSERT_SUCCESS(sb.move_to(out));
  ASSERT_EQUAL(out, expected);
  ASSERT_EQUAL(sb.size(), 0);
  // The builder remains usable.
  sb.append_raw("Test");
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "Test");
  ASSERT_SUCCESS(sb.move_to(out));
  ASSERT_EQUAL(out, "Test");
  TEST_SUCCEED();
}

bool move_to_small_result_does_not_keep_buffer() {
  TEST_START();
  // A small result is copied out, so that it does not keep the whole buffer.
  simdjson::builder::string_builder sb;
  sb.append_raw("[1,2]");
  std::string out;
  ASSERT_SUCCESS(sb.move_to(out));
  ASSERT_EQUAL(out, "[1,2]");
  ASSERT_TRUE(out.capacity() <
              simdjson::builder::string_builder::DEFAULT_INITIAL_CAPACITY);
  TEST_SUCCEED();
}

bool move_to_large_result_is_not_copied() {
  TEST_START();
  // A result that fills most of the buffer is handed over without a copy.
  simdjson::builder::string_builder sb;
  const std::string expected(
      simdjson::builder::string_builder::DEFAULT_INITIAL_CAPACITY * 3 / 4, 'x');
  sb.append_raw(expected);
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  const char *data = p.data();
  std::string out;
  ASSERT_SUCCESS(sb.move_to(out));
  ASSERT_EQUAL(out, expected);
  ASSERT_TRUE(out.data() == data);
  TEST_SUCCEED();
}

bool move_to_small_result_keeps_builder_buffer() {
  TEST_START();
  // After copying out a small result, the builder reuses its buffer.
  simdjson::builder::string_builder sb;
  sb.append_raw("[1,2]");
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  const char *data = p.data();
  std::string out;
  ASSERT_SUCCESS(sb.move_to(out));
  ASSERT_EQUAL(out, "[1,2]");
  ASSERT_EQUAL(sb.size(), 0);
  sb.append_raw("[3]");
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "[3]");
  ASSERT_TRUE(p.data() == data);
  ASSERT_SUCCESS(sb.move_to(out));
  ASSERT_EQUAL(out, "[3]");
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

// An empty std::string_view may have a null data() pointer.
bool empty_inputs() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append_raw(std::string_view{});
  sb.append_raw(nullptr, 0);
  sb.escape_and_append(std::string_view{});
  sb.escape_and_append_with_quotes(std::string_view{});
  sb.append_raw("x");
  sb.append_raw(std::string_view{});
  sb.escape_and_append(std::string_view{});
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "\"\"x");
  char out[16] = {'a', 'b', 'c', 'd'};
  ASSERT_EQUAL(simdjson::internal::escape_scalar::write_string_escaped(
                   std::string_view{}, out),
               0);
  ASSERT_EQUAL(simdjson::internal::escape_best::write_string_escaped(
                   std::string_view{}, out),
               0);
  ASSERT_EQUAL(std::string_view(out, 4), "abcd");
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
bool escape_and_append_with_quotes_template() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.escape_and_append_with_quotes<"">();
  sb.escape_and_append_with_quotes<"plain">();
  sb.escape_and_append_with_quotes<"a\"b\\c\n\x01">();
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "\"\"\"plain\"\"a\\\"b\\\\c\\n\\u0001\"");
  TEST_SUCCEED();
}
bool append_key_value_template() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append_key_value<"key", "value">();
  sb.append_key_value<"bool", true>();
  sb.append_key_value<"int", 5>();
  sb.append_key_value<"nullptr", nullptr>();
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "\"key\":\"value\"\"bool\":true\"int\":5\"nullptr\":null");
  TEST_SUCCEED();
}
bool append_key_value_template_padding() {
  TEST_START();
  simdjson::builder::string_builder sb(1);
  sb.append_key_value<"short", "x">();
  sb.append_key_value<"tire_pressure">(2); // "tire_pressure": is 16 bytes
  sb.append_key_value<"a_key_longer_than_16_bytes">(1);
  sb.escape_and_append_with_quotes<"k">();
  sb.append_key_value<"n", nullptr>();
  std::string_view p;
  auto result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "\"short\":\"x\"\"tire_pressure\":2\"a_key_longer_than_16_bytes\":1"
                  "\"k\"\"n\":null");
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
  sb.clear();
  std::optional<int> optional_int = 3;
  sb.append(optional_int);
  result = sb.view().get(p);
  ASSERT_SUCCESS(result);
  ASSERT_EQUAL(p, "3");
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
template <typename R>
std::string append_to_json(const R &range) {
  simdjson::builder::string_builder sb;
  sb.append(range);
  std::string_view p;
  if (sb.view().get(p)) {
    return "error";
  }
  return std::string(p);
}

// The same values through the generic range path, which takes a range that is
// not contiguous.
template <typename T>
std::string generic_to_json(const std::vector<T> &v) {
  return append_to_json(v | std::views::transform([](T x) { return x; }));
}

bool integer_array_test() {
  TEST_START();
  ASSERT_EQUAL(append_to_json(std::vector<int>{}), "[]");
  ASSERT_EQUAL(append_to_json(std::vector<int>{7}), "[7]");
  ASSERT_EQUAL(append_to_json(std::vector<int64_t>{0, -1, 9, 10, -99, 100}),
               "[0,-1,9,10,-99,100]");
  std::vector<int64_t> i64{(std::numeric_limits<int64_t>::min)(),
                           (std::numeric_limits<int64_t>::max)()};
  ASSERT_EQUAL(append_to_json(i64),
               "[-9223372036854775808,9223372036854775807]");
  ASSERT_EQUAL(append_to_json(std::vector<uint64_t>{
                   0, (std::numeric_limits<uint64_t>::max)()}),
               "[0,18446744073709551615]");
  ASSERT_EQUAL(append_to_json(std::vector<int16_t>{-32768, 32767}),
               "[-32768,32767]");
  ASSERT_EQUAL(append_to_json(std::vector<uint16_t>{0, 65535}), "[0,65535]");
  ASSERT_EQUAL(append_to_json(std::vector<int32_t>{
                   (std::numeric_limits<int32_t>::min)(),
                   (std::numeric_limits<int32_t>::max)()}),
               "[-2147483648,2147483647]");
  ASSERT_EQUAL(append_to_json(std::array<unsigned, 3>{1, 22, 333}),
               "[1,22,333]");
  ASSERT_EQUAL(append_to_json(std::span<const int64_t>(i64).subspan(1)),
               "[9223372036854775807]");
  // A contiguous range without a size takes the generic path.
  std::vector<int> head{1, 2, 3, 9, 1};
  ASSERT_EQUAL(append_to_json(head | std::views::take_while(
                                         [](int x) { return x < 5; })),
               "[1,2,3]");
  TEST_SUCCEED();
}

bool integer_array_matches_generic_path() {
  TEST_START();
  uint64_t state = 12345;
  auto next = [&state]() {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    return state;
  };
  // The large sizes span several reserved blocks.
  for (size_t n : {size_t(1), size_t(2), size_t(1000), size_t(120000),
                   size_t(300000)}) {
    std::vector<int64_t> i64(n);
    for (auto &x : i64) {
      uint64_t r = next();
      x = int64_t(r) >> (r % 64);
    }
    ASSERT_EQUAL(append_to_json(i64), generic_to_json(i64));
    std::vector<uint32_t> u32(n);
    for (auto &x : u32) {
      uint64_t r = next();
      x = uint32_t(r >> (r % 64));
    }
    ASSERT_EQUAL(append_to_json(u32), generic_to_json(u32));
    std::vector<int16_t> i16(n);
    for (auto &x : i16) {
      x = int16_t(next());
    }
    ASSERT_EQUAL(append_to_json(i16), generic_to_json(i16));
  }
  TEST_SUCCEED();
}

bool integer_array_after_other_content() {
  TEST_START();
  simdjson::builder::string_builder sb(1);
  sb.start_array();
  sb.append(std::vector<int>{1, -2});
  sb.append_comma();
  sb.append(std::vector<uint64_t>{});
  sb.append_comma();
  sb.append(std::vector<short>{3});
  sb.end_array();
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "[[1,-2],[],[3]]");
  TEST_SUCCEED();
}

// The integer writer may write past the last digit of a short value. A
// builder that starts with one byte reserves exactly the bound, so the last
// element ends near the end of the buffer.
bool integer_array_short_last_element() {
  TEST_START();
  auto exact_to_json = [](const auto &range) {
    simdjson::builder::string_builder sb(1);
    sb.append(range);
    std::string_view p;
    return sb.view().get(p) ? std::string("error") : std::string(p);
  };
  for (size_t n = 1; n <= 40; n++) {
    std::vector<uint16_t> u16(n, 65535);
    std::vector<int16_t> i16(n, -32768);
    std::vector<uint32_t> u32(n, (std::numeric_limits<uint32_t>::max)());
    std::vector<int64_t> i64(n, (std::numeric_limits<int64_t>::min)());
    std::vector<uint64_t> u64(n, (std::numeric_limits<uint64_t>::max)());
    for (int last : {0, 7, 42, 999, 9999}) {
      u16.back() = uint16_t(last);
      i16.back() = int16_t(-last);
      u32.back() = uint32_t(last);
      i64.back() = -last;
      u64.back() = uint64_t(last);
      ASSERT_EQUAL(exact_to_json(u16), generic_to_json(u16));
      ASSERT_EQUAL(exact_to_json(i16), generic_to_json(i16));
      ASSERT_EQUAL(exact_to_json(u32), generic_to_json(u32));
      ASSERT_EQUAL(exact_to_json(i64), generic_to_json(i64));
      ASSERT_EQUAL(exact_to_json(u64), generic_to_json(u64));
    }
  }
  TEST_SUCCEED();
}

bool integer_array_bounded_reservation() {
  TEST_START();
  // Two bytes per element are written, while the bound is 21 bytes per element.
  std::vector<int64_t> zeros(1000000, 0);
  simdjson::builder::string_builder sb;
  sb.append(zeros);
  ASSERT_EQUAL(sb.size(), 2 * zeros.size() + 1);
  ASSERT_TRUE(sb.unsafe_capacity() <= 2 * (sb.size() + (size_t(1) << 20)));
  TEST_SUCCEED();
}

// Reports a size larger than its storage. Only the first block is read: the
// reservation for the rest overflows size_t and must fail the builder.
struct oversized_uint16_range {
  const uint16_t *first;
  size_t reported;
  const uint16_t *data() const { return first; }
  const uint16_t *begin() const { return first; }
  const uint16_t *end() const { return first; }
  size_t size() const { return reported; }
};

bool integer_array_reservation_overflow() {
  TEST_START();
  constexpr size_t block = (size_t(1) << 20) / 6;
  std::vector<uint16_t> zeros(block, 0);
  // Three bytes per element on average after the first block of zeros.
  oversized_uint16_range r{zeros.data(), block + (std::numeric_limits<size_t>::max)() / 3};
  simdjson::builder::string_builder sb;
  sb.append(r);
  std::string_view v;
  ASSERT_ERROR(sb.view().get(v), simdjson::OUT_OF_CAPACITY);
  TEST_SUCCEED();
}

bool contiguous_integer_arrays() {
  TEST_START();
  simdjson::builder::string_builder sb(1);
  std::vector<int64_t> v = {0, -1, (std::numeric_limits<int64_t>::min)(),
                            (std::numeric_limits<int64_t>::max)()};
  sb.append(v);
  std::array<uint16_t, 2> a = {7, 65535};
  sb.append(a);
  std::vector<int> empty;
  sb.append(empty);
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "[0,-1,-9223372036854775808,9223372036854775807][7,65535][]");
  TEST_SUCCEED();
}

bool contiguous_string_arrays() {
  TEST_START();
  simdjson::builder::string_builder sb;
  std::vector<std::string> v = {"a\"b", "\n", "", std::string(1, '\x01'), "\\"};
  sb.append(v);
  std::vector<std::string_view> sv = {"x", "y"};
  sb.append(sv);
  std::array<std::string, 1> one = {"z"};
  sb.append(one);
  std::vector<std::string> empty;
  sb.append(empty);
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "[\"a\\\"b\",\"\\n\",\"\",\"\\u0001\",\"\\\\\"][\"x\",\"y\"][\"z\"][]");
  TEST_SUCCEED();
}

// Element sizes whose reservation plus the current position overflows
// size_t. Only the sizes are read before the reservation fails.
bool contiguous_string_arrays_reservation_overflow() {
  TEST_START();
  constexpr size_t max = (std::numeric_limits<size_t>::max)();
  // Largest unescaped size whose 1/16 slack still fits in size_t. The
  // reservation is then within 17 bytes of max, below the prefix length.
  size_t bytes = max - max / 17 - 64;
  while ((bytes + 1) <= max - (bytes + 1) / 16) {
    bytes++;
  }
  const size_t count = 6;
  const size_t payload = bytes - 2 - 3 * count;
  static const char storage = 'a';
  std::vector<std::string_view> v;
  for (size_t i = 0; i < count; i++) {
    v.emplace_back(&storage, payload / count + (i == 0 ? payload % count : 0));
  }
  simdjson::builder::string_builder sb;
  sb.append_raw(std::string(32, ' '));
  sb.append(v);
  std::string_view p;
  ASSERT_ERROR(sb.view().get(p), simdjson::OUT_OF_CAPACITY);
  TEST_SUCCEED();
}

// Escapes beyond the reserved size, around the 512-byte side-buffer limit,
// starting from every small initial capacity.
bool contiguous_string_arrays_growth() {
  TEST_START();
  std::vector<std::string> v;
  for (size_t len : {size_t(0), size_t(1), size_t(100), size_t(511), size_t(512),
                     size_t(513), size_t(1000)}) {
    v.push_back(std::string(len, 'a'));
    v.push_back(std::string(len, '"'));
    v.push_back(std::string(len, '\x1f'));
  }
  std::string prefix(37, ' ');
  simdjson::builder::string_builder ref;
  ref.append_raw(prefix);
  ref.start_array();
  for (size_t i = 0; i < v.size(); i++) {
    if (i) {
      ref.append_comma();
    }
    ref.escape_and_append_with_quotes(v[i]);
  }
  ref.end_array();
  std::string_view expected;
  ASSERT_SUCCESS(ref.view().get(expected));
  for (size_t cap = 1; cap <= expected.size() + 1; cap += (cap < 4200 ? 1 : 997)) {
    simdjson::builder::string_builder sb(cap);
    sb.append_raw(prefix);
    sb.append(v);
    std::string_view p;
    ASSERT_SUCCESS(sb.view().get(p));
    ASSERT_EQUAL(p, expected);
  }
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
// A char in a container or an optional is a one-character string, as in
// append_key_value(), not a raw character.
bool char_values_test() {
  TEST_START();
  std::vector<char> c = {'a', '"', '\n'};
  simdjson::builder::string_builder sb;
  sb.append(c);
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "[\"a\",\"\\\"\",\"\\n\"]");
  std::string s;
  ASSERT_SUCCESS(simdjson::to_json(std::vector<std::optional<char>>{'\\', std::nullopt}).get(s));
  ASSERT_EQUAL(s, "[\"\\\\\",null]");
  ASSERT_SUCCESS(simdjson::to_json('"').get(s));
  ASSERT_EQUAL(s, "\"\\\"\"");
  // The output reads back as the same characters.
  ASSERT_SUCCESS(simdjson::to_json(c).get(s));
  simdjson::ondemand::parser parser;
  simdjson::padded_string json(s);
  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  std::vector<char> back;
  ASSERT_SUCCESS(doc.get(back));
  ASSERT_TRUE(back == c);
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

template <class R>
bool to_json_array_matches_builder(const R &values) {
  simdjson::builder::string_builder sb;
  sb.append(values);
  std::string_view expected;
  ASSERT_SUCCESS(sb.view().get(expected));
  std::string fresh;
  ASSERT_SUCCESS(simdjson::to_json(values).get(fresh));
  ASSERT_EQUAL(fresh, expected);
#if !SIMDJSON_STATIC_REFLECTION
  // A kept result must not hold the worst-case allocation. With static
  // reflection, to_json writes into a string sized by the worst case.
  ASSERT_TRUE(fresh.capacity() <= 2 * fresh.size() + 32);
#endif
  std::string reused(4096, 'x');
  ASSERT_SUCCESS(simdjson::to_json(values, reused));
  ASSERT_EQUAL(reused, expected);
  return true;
}

template <class T>
bool to_json_array_of(size_t n) {
  std::vector<T> values(n);
  for (size_t i = 0; i < n; i++) {
    switch (i % 4) {
    case 0: values[i] = (std::numeric_limits<T>::max)(); break;
    case 1: values[i] = std::numeric_limits<T>::lowest(); break;
    case 2: values[i] = T(i % 10); break;
    default: values[i] = T(T(i) / T(3)); break;
    }
  }
  return to_json_array_matches_builder(values);
}

template <class T>
bool to_json_array_sizes() {
  // Sizes below and above the stack-buffer path.
  for (size_t n : {0, 1, 2, 3, 7, 40, 100, 1000, 10000}) {
    if (!to_json_array_of<T>(n)) {
      return false;
    }
  }
  return true;
}

bool to_json_contiguous_number_arrays() {
  TEST_START();
  ASSERT_TRUE(to_json_array_sizes<int16_t>());
  ASSERT_TRUE(to_json_array_sizes<uint16_t>());
  ASSERT_TRUE(to_json_array_sizes<int32_t>());
  ASSERT_TRUE(to_json_array_sizes<uint32_t>());
  ASSERT_TRUE(to_json_array_sizes<int64_t>());
  ASSERT_TRUE(to_json_array_sizes<uint64_t>());
  ASSERT_TRUE(to_json_array_sizes<float>());
  ASSERT_TRUE(to_json_array_sizes<double>());
  std::array<int, 3> a{1, -2, 3};
  ASSERT_TRUE(to_json_array_matches_builder(a));
  std::vector<double> doubles{-2.2250738585072014e-308, 1.7976931348623157e308,
                              0.00012345678901234567, -0.0, 0.1};
  ASSERT_TRUE(to_json_array_matches_builder(doubles));
  ASSERT_TRUE(to_json_array_matches_builder(std::vector<int64_t>(5000, 7)));
#if !SIMDJSON_STATIC_REFLECTION
  // Wider floating types would be narrowed to double; append rejects them.
  static_assert(sizeof(long double) <= sizeof(uint64_t) ||
                !simdjson::builder_to_json_detail::contiguous_float_array<std::vector<long double>>);
  static_assert(simdjson::builder_to_json_detail::contiguous_float_array<std::vector<double>>);
#endif
#if SIMDJSON_ENABLE_NAN_INF
  std::string json;
  ASSERT_SUCCESS(simdjson::to_json(std::vector<double>{
      std::numeric_limits<double>::quiet_NaN(),
      std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity()}).get(json));
  ASSERT_EQUAL(json, "[NaN,Infinity,-Infinity]");
#endif
  TEST_SUCCEED();
}
#endif // SIMDJSONS_SUPPORT_RANGES

#if SIMDJSON_EXCEPTIONS
bool rvalue_conversion_moves_large_result() {
  TEST_START();
  simdjson::builder::string_builder sb;
  const std::string expected(
      simdjson::builder::string_builder::DEFAULT_INITIAL_CAPACITY * 3 / 4, 'x');
  sb.append_raw(expected);
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  const char *data = p.data();
  std::string out = std::move(sb);
  ASSERT_EQUAL(out, expected);
  ASSERT_TRUE(out.data() == data);
  ASSERT_EQUAL(sb.size(), 0);
  sb.append_raw("[3]");
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "[3]");
  TEST_SUCCEED();
}

bool rvalue_conversion_copies_small_result() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append_raw("[1,2]");
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  const char *data = p.data();
  std::string out = std::move(sb);
  ASSERT_EQUAL(out, "[1,2]");
  ASSERT_EQUAL(sb.size(), 0);
  sb.append_raw("[3]");
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "[3]");
  ASSERT_TRUE(p.data() == data);
  TEST_SUCCEED();
}

bool lvalue_conversion_copies() {
  TEST_START();
  simdjson::builder::string_builder sb;
  sb.append_raw("[1,2]");
  std::string out = sb;
  ASSERT_EQUAL(out, "[1,2]");
  ASSERT_EQUAL(sb.size(), 5);
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  ASSERT_EQUAL(p, "[1,2]");
  TEST_SUCCEED();
}

std::string build_and_return(size_t count) {
  simdjson::builder::string_builder sb;
  for (size_t i = 0; i < count; i++) {
    sb.append(uint64_t(i));
    sb.append_comma();
  }
  return sb;
}

bool return_builder_as_string() {
  TEST_START();
  std::string expected;
  for (size_t i = 0; i < 1000; i++) {
    expected += std::to_string(i) + ",";
  }
  ASSERT_EQUAL(build_and_return(1000), expected);
  ASSERT_EQUAL(build_and_return(0), "");
  TEST_SUCCEED();
}

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
bool run() {
  return allchar_test() && bad_utf8_test() && various_integers() &&
         various_unsigned_integers() && integer_digit_boundaries() &&
         car_test_long() && car_test() &&
#if SIMDJSON_EXCEPTIONS
         car_test_exception() && string_convertion_except() &&
         rvalue_conversion_moves_large_result() &&
         rvalue_conversion_copies_small_result() && lvalue_conversion_copies() &&
         return_builder_as_string() &&
#endif
#if SIMDJSON_SUPPORTS_RANGES && SIMDJSON_SUPPORTS_CONCEPTS
         map_test() && ranges_test() && integer_array_test() &&
         integer_array_matches_generic_path() &&
         integer_array_after_other_content() &&
         integer_array_short_last_element() &&
         integer_array_bounded_reservation() &&
         integer_array_reservation_overflow() && contiguous_integer_arrays() &&
         contiguous_string_arrays() && contiguous_string_arrays_growth() &&
         contiguous_string_arrays_reservation_overflow() &&
         double_double_test() &&
         double_double_test_to_string() && car_test_simple() &&
         car_test_simple_complete() && char_values_test() && to_json_contiguous_number_arrays() &&
#if SIMDJSON_EXCEPTIONS
         car_test_simple_complete_exceptions() &&
#endif
#endif
#if SIMDJSON_SUPPORTS_CONCEPTS
         issue2549() && car_test_template() &&
         escape_and_append_with_quotes_template() && append_key_value_template() &&
         append_key_value_template_padding() &&
         serialize_optional() && issue2827_optional_serialization() &&
#endif
         append_char() && append_integer() && append_float() &&
         append_double_formatting() && append_null() &&
#if SIMDJSON_ENABLE_NAN_INF
         append_nan() && append_positive_infinity() &&
         append_negative_infinity() && append_float_nan_inf() &&
         nan_inf_in_array() && nan_inf_in_object() && nan_inf_roundtrip() &&
#else
         nan_inf_as_null() &&
#endif
         clear() && escape_and_append() && escape_and_append_with_quotes() &&
         escape_write_string_escaped_exhaustive() && move_to_string() &&
         move_to_small_result_does_not_keep_buffer() &&
         move_to_large_result_is_not_copied() &&
         move_to_small_result_keeps_builder_buffer() && append_raw() &&
         raw_with_length() && empty_inputs() && string_convertion() && buffer_growth() &&
         unicode_validation() && true;
}

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, builder_tests::run);
}
