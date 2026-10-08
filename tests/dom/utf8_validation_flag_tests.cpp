#include "simdjson.h"
#include "test_macros.h"
#include "test_main.h"
#include <string>

using namespace simdjson;

// SIMDJSON_UTF8VALIDATION (CMake option SIMDJSON_SKIPUTF8VALIDATION) must have
// the same effect on every implementation, fallback included.
namespace utf8_validation_flag_tests {

struct string_case {
  const char *json;     // document
  const char *expected; // value of "a" when UTF-8 is not validated
};

// None of these strings is valid UTF-8.
const string_case invalid_cases[] = {
  {"{\"a\":\"\xe0\xa4\"}", "\xe0\xa4"}, // truncated 3-byte sequence
  {"{\"a\":\"\xff\"}", "\xff"},         // byte that never occurs in UTF-8
  {"{\"a\":\"\xe0\\\"\"}", "\xe0\""},   // invalid byte, then an escaped quote
};

bool invalid_utf8_in_string() {
  TEST_START();
  for (auto impl : get_available_implementations()) {
    if (!impl->supported_by_runtime_system()) { continue; }
    std::cout << " - " << impl->name() << std::endl;
    get_active_implementation() = impl;
    for (const auto &c : invalid_cases) {
      padded_string json(std::string(c.json));
      dom::parser parser;
      dom::element doc;
      auto error = parser.parse(json).get(doc);
#if SIMDJSON_UTF8VALIDATION
      ASSERT_ERROR(error, UTF8_ERROR);
#else
      ASSERT_SUCCESS(error);
      std::string_view value;
      ASSERT_SUCCESS(doc["a"].get(value));
      ASSERT_EQUAL(std::string(value), std::string(c.expected));
#endif
    }
  }
  TEST_SUCCEED();
}

bool valid_utf8_in_string() {
  TEST_START();
  const std::string expected = "\xe0\xa4\xb9"; // U+0939
  for (auto impl : get_available_implementations()) {
    if (!impl->supported_by_runtime_system()) { continue; }
    std::cout << " - " << impl->name() << std::endl;
    get_active_implementation() = impl;
    padded_string json(std::string("{\"a\":\"") + expected + "\"}");
    dom::parser parser;
    dom::element doc;
    ASSERT_SUCCESS(parser.parse(json).get(doc));
    std::string_view value;
    ASSERT_SUCCESS(doc["a"].get(value));
    ASSERT_EQUAL(std::string(value), expected);
  }
  TEST_SUCCEED();
}

bool run() {
  std::cout << "SIMDJSON_UTF8VALIDATION=" << SIMDJSON_UTF8VALIDATION << std::endl;
  return invalid_utf8_in_string() && valid_utf8_in_string();
}

} // namespace utf8_validation_flag_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, utf8_validation_flag_tests::run);
}
