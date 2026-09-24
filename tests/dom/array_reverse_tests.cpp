#include "simdjson.h"
#include "test_macros.h"
#include "test_main.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <limits>
#include <locale>
#include <random>
#include <sstream>
#include <vector>

using namespace simdjson;

namespace array_reverse_tests {

bool check_array(const dom::array array) {
  const std::vector<dom::element> expected(array.begin(), array.end());
  auto it = array.rbegin();
  const auto end = array.rend();
  for (auto forward = expected.rbegin(); forward != expected.rend(); ++forward) {
    ASSERT_TRUE(it != end);
    ASSERT_TRUE(*it == *forward);
    auto copy = it;
    ASSERT_TRUE(copy == it);
    auto previous = it++;
    ASSERT_TRUE(previous == copy);
    ASSERT_TRUE(*copy == *forward);
    ++copy;
    ASSERT_TRUE(copy == it);
  }
  ASSERT_TRUE(it == end);
  ASSERT_EQUAL(std::distance(array.rbegin(), array.rend()), expected.size());
  ASSERT_TRUE(std::equal(array.rbegin(), array.rend(), expected.rbegin()));
  for (auto value : expected) {
    if (value.is_array()) {
      dom::array child;
      ASSERT_SUCCESS(value.get_array().get(child));
      ASSERT_TRUE(check_array(child));
    }
  }
  return true;
}

bool check_json(dom::parser &parser, const std::string &json) {
  dom::array array;
  const padded_string padded(json);
  ASSERT_SUCCESS(parser.parse(padded).get_array().get(array));
  return check_array(array);
}

bool mixed_values() {
  TEST_START();
  dom::parser parser;
  for (const char *json : {
    "[]", "[null]", "[true]", "[false]", "[1]", "[1.5]", "[\"\"]",
    "[[]]", "[{}]", "[[[],{}],{\"x\":[1,2]}]",
    "[null,true,false,\"hello\",1,-1,1.5,-0.0,18446744073709551615,"
    "-9223372036854775808,[],{},[2,3],{\"a\":4}]"
  }) {
    ASSERT_TRUE(check_json(parser, json));
  }

  // Starting a reverse traversal inside a document must respect that array's
  // boundaries, even when neighboring values have different tape widths.
  const auto json = R"({"before":1,"arrays":[[],[1],[[2],3]],"after":4})"_padded;
  dom::array arrays;
  ASSERT_SUCCESS(parser.parse(json)["arrays"].get_array().get(arrays));
  ASSERT_TRUE(check_array(arrays));
  for (auto value : arrays) {
    dom::array child;
    ASSERT_SUCCESS(value.get_array().get(child));
    ASSERT_TRUE(check_array(child));
  }

  parser.number_as_string(true);
  ASSERT_TRUE(check_json(parser, "[99999999999999999999,0,-99999999999999999999]"));
  TEST_SUCCEED();
}

std::vector<std::string> numeric_payloads() {
  std::vector<std::string> values{
    "0", "-1", "-0.0", "1.5", "18446744073709551615", "-9223372036854775808"
  };
  // Raw number bits can look like any tape tag, including an exact numeric
  // header or a closing container tag with a plausible backwards link.
  for (char tag : {'r', '[', ']', '{', '}', '"', 'l', 'u', 'd', 't', 'f', 'n', 'Z'}) {
    for (uint64_t payload : {uint64_t(0), uint64_t(1), uint64_t(7)}) {
      const uint64_t bits = (uint64_t(tag) << 56) | payload;
      values.push_back(std::to_string(bits));
      double number;
      std::memcpy(&number, &bits, sizeof(number));
      std::ostringstream out;
      out.imbue(std::locale::classic());
      out << std::setprecision(std::numeric_limits<double>::max_digits10) << number;
      values.push_back(out.str());
    }
  }
  return values;
}

bool ambiguous_numbers() {
  TEST_START();
  dom::parser parser;
  const auto values = numeric_payloads();
  // Payloads imitate mutually matching container tags, with varying marker
  // runs before and between them. Matching links alone do not prove a tag.
  for (char tag : {'[', '{'}) {
    const char close = tag == '[' ? ']' : '}';
    std::string prefix;
    for (size_t p = 0; p < 8; ++p) {
      std::string gap;
      for (size_t g = 0; g < 8; ++g) {
        const auto fake_open = std::to_string((uint64_t(tag) << 56) | (2 * p + 2 * g + 6));
        const auto fake_close = std::to_string((uint64_t(close) << 56) | (2 * p + 3));
        ASSERT_TRUE(check_json(parser, "[" + prefix + fake_open + ',' + gap + fake_close + "]"));
        gap += std::to_string(uint64_t('l') << 56) + ',';
      }
      prefix += std::to_string(uint64_t('l') << 56) + ',';
    }
  }
  for (const auto &first : values) {
    for (const auto &second : values) {
      ASSERT_TRUE(check_json(parser, "[" + first + "," + second + ",null,[],{},\"x\"]"));
      ASSERT_TRUE(check_json(parser, "[null,[],{},\"x\"," + first + "," + second + "]"));
    }
  }

  // Exercise both odd and even marker runs. A traversal must not rescan the
  // entire run when advancing through each of its numbers.
  std::string run = "[";
  for (size_t i = 0; i < 4096; ++i) {
    if (i) { run += ','; }
    run += std::to_string(uint64_t("lud"[i % 3]) << 56);
  }
  for (const char *tail : { "]", ",null]", ",1]", ",[]]", ",[1,2]]" }) {
    ASSERT_TRUE(check_json(parser, run + tail));
    ASSERT_TRUE(check_json(parser, "[" + run + tail + ",null]"));
  }
  std::string prefix;
  for (size_t i = 0; i < 8; ++i) {
    ASSERT_TRUE(check_json(parser, "[" + prefix + run + "]]"));
    prefix += std::to_string(uint64_t('l') << 56) + ',';
  }
  // A forged opening tag may be a payload inside an earlier nested array.
  // The candidate boundary still needs validation in that case.
  for (char tag : {'[', '{'}) {
    const char close = tag == '[' ? ']' : '}';
    const auto fake_open = std::to_string((uint64_t(tag) << 56) | (2 * 4096 + 8));
    const auto fake_close = std::to_string((uint64_t(close) << 56) | (2 * 4096 + 4));
    ASSERT_TRUE(check_json(parser, "[" + run + ',' + fake_open + "]," + fake_close + "]"));
  }
  TEST_SUCCEED();
}

bool randomized_values() {
  TEST_START();
  dom::parser parser;
  auto values = numeric_payloads();
  for (const char *value : {"null", "true", "false", "\"a\\u0000b\"", "[]", "{}",
      "[1,[2,3],null]", "{\"a\":[4,5],\"b\":{}}"}) {
    values.emplace_back(value);
  }
  std::mt19937 random(1369);
  for (size_t trial = 0; trial < 2000; ++trial) {
    std::string json = "[";
    const size_t count = random() % 64;
    for (size_t i = 0; i < count; ++i) {
      if (i) { json += ','; }
      json += values[random() % values.size()];
    }
    json += ']';
    ASSERT_TRUE(check_json(parser, json));
  }
  TEST_SUCCEED();
}

bool result_wrappers() {
  TEST_START();
#if SIMDJSON_EXCEPTIONS
  dom::parser parser;
  const auto json = "[1,2,3]"_padded;
  auto result = parser.parse(json).get_array();
  ASSERT_SUCCESS(result.error());
  int64_t expected = 3;
  for (auto it = result.rbegin(); it != result.rend(); ++it) {
    int64_t value;
    ASSERT_SUCCESS((*it).get_int64().get(value));
    ASSERT_EQUAL(value, expected--);
  }
  ASSERT_EQUAL(expected, 0);
  const simdjson_result<dom::array> failed(NO_SUCH_FIELD);
  bool begin_threw = false, end_threw = false;
  try { (void)failed.rbegin(); }
  catch (const simdjson_error &error) {
    ASSERT_EQUAL(error.error(), NO_SUCH_FIELD);
    begin_threw = true;
  }
  try { (void)failed.rend(); }
  catch (const simdjson_error &error) {
    ASSERT_EQUAL(error.error(), NO_SUCH_FIELD);
    end_threw = true;
  }
  ASSERT_TRUE(begin_threw && end_threw);
#endif
  TEST_SUCCEED();
}

bool run() {
  const dom::array::reverse_iterator empty1, empty2;
  ASSERT_TRUE(empty1 == empty2);
  return mixed_values() && ambiguous_numbers() && randomized_values() && result_wrappers();
}

} // namespace array_reverse_tests

#if SIMDJSON_SUPPORTS_RANGES
static_assert(std::forward_iterator<dom::array::reverse_iterator>);
#endif

int main(int argc, char *argv[]) {
  return test_main(argc, argv, array_reverse_tests::run);
}
