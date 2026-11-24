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
    const std::string before(61, 'a');
    const std::string after(256, 'z');
    ASSERT_TRUE(check_string(padding, before + "\\\"" + after, before + "\"" + after));
    ASSERT_TRUE(check_string(padding, before + "\\\\" + after, before + "\\" + after));
    ASSERT_TRUE(check_string(padding, before + "\\u005B" + after, before + "[" + after));
    ASSERT_TRUE(check_string(padding, before + "\xc2\xa2" + after, before + "\xc2\xa2" + after));
    // An error before plain ASCII blocks must remain visible after the fast path.
    ASSERT_TRUE(check_string(padding, before + "\xc2" + after, "", simdjson::UTF8_ERROR));
    ASSERT_TRUE(check_string(padding, before + "\x01" + after, "", simdjson::UNESCAPED_CHARS));
  }
  TEST_SUCCEED();
}

int main() {
  return plain_string_blocks() ? EXIT_SUCCESS : EXIT_FAILURE;
}
