#include "simdjson.h"
#include "test_macros.h"

#include <string>
#include <string_view>

bool check_string(size_t padding, const std::string &raw,
                  const std::string &expected, simdjson::error_code error = simdjson::SUCCESS) {
  simdjson::dom::parser parser;
  simdjson::padded_string input(std::string(padding, ' ') + "[\"" + raw + "\",17]");
  simdjson::dom::element document;
  ASSERT_ERROR(parser.parse(input).get(document), error);
  if (error) { return true; }
  std::string_view value;
  ASSERT_SUCCESS(document.at(0).get_string().get(value));
  ASSERT_EQUAL(value, std::string_view(expected));
  uint64_t following;
  ASSERT_SUCCESS(document.at(1).get_uint64().get(following));
  ASSERT_EQUAL(following, uint64_t(17));
  return true;
}

bool plain_string_blocks() {
  TEST_START();
  // Move string and escape boundaries through every position in a 64-byte block.
  for (size_t padding = 0; padding < 64; ++padding) {
    for (char content : std::string("x []{}:,")) {
      const std::string text(256, content);
      ASSERT_TRUE(check_string(padding, text, text));
    }
    const std::string after(256, 'z');
    // Exercise fallback both before and after consecutive skipped blocks.
    for (size_t prefix_length : {size_t(61), size_t(317)}) {
      const std::string before(prefix_length, 'a');
      ASSERT_TRUE(check_string(padding, before + "\\\"" + after, before + "\"" + after));
      ASSERT_TRUE(check_string(padding, before + "\\\\" + after, before + "\\" + after));
      ASSERT_TRUE(check_string(padding, before + "\\u005B" + after, before + "[" + after));
      ASSERT_TRUE(check_string(padding, before + "\xc2\xa2" + after, before + "\xc2\xa2" + after));
      ASSERT_TRUE(check_string(padding, before + "\xe2\x82\xac" + after, before + "\xe2\x82\xac" + after));
      ASSERT_TRUE(check_string(padding, before + "\xf0\x9f\x98\x80" + after, before + "\xf0\x9f\x98\x80" + after));
      ASSERT_TRUE(check_string(padding, before + std::string(64, '\\') + after,
                               before + std::string(32, '\\') + after));
      ASSERT_TRUE(check_string(padding, before + std::string(65, '\\') + "\"" + after,
                               before + std::string(32, '\\') + "\"" + after));
      // Errors must survive subsequent plain ASCII blocks.
      ASSERT_TRUE(check_string(padding, before + "\xc2" + after, "", simdjson::UTF8_ERROR));
      ASSERT_TRUE(check_string(padding, before + "\xe2\x82" + after, "", simdjson::UTF8_ERROR));
      ASSERT_TRUE(check_string(padding, before + "\x01" + after, "", simdjson::UNESCAPED_CHARS));
    }
    simdjson::dom::parser parser;
    simdjson::padded_string unclosed(std::string(padding, ' ') + "[\"" + after);
    simdjson::dom::element document;
    ASSERT_ERROR(parser.parse(unclosed).get(document), simdjson::UNCLOSED_STRING);
  }
  TEST_SUCCEED();
}

int main() {
  return plain_string_blocks() ? EXIT_SUCCESS : EXIT_FAILURE;
}
