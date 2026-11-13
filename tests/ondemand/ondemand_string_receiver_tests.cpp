#include "simdjson.h"
#include "test_ondemand.h"

#include <string>
#include <vector>

using namespace simdjson;

// Tests for the overloads that unescape into a std::string: get_string(std::string&),
// unescaped_key(std::string&) and get<std::string>(). The string_view overloads serve as the
// reference: for every test string, both must return the same content or the same error.
namespace string_receiver_tests {

  // Text without repeating patterns, so that a copy from a wrong offset is noticed.
  std::string sample_text(size_t length) {
    const std::string alphabet = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string text;
    text.reserve(length);
    for (size_t i = 0; i < length; i++) {
      text += alphabet[i % alphabet.size()];
    }
    return text;
  }

  // JSON strings (with their quotes) around the sizes that matter: std::string keeps up to 15
  // (libstdc++, MSVC) or 22 (libc++) characters inline, and the kernels scan 32 or 64 bytes at once.
  std::vector<std::string> test_strings() {
    std::vector<std::string> contents;

    // No escapes, every length from 0 to 80.
    for (size_t length = 0; length <= 80; length++) {
      contents.push_back(sample_text(length));
    }

    // An escape sequence at the start, in the middle or at the end.
    const std::vector<std::string> escapes = { "\\n", "\\\"", "\\\\", "\\/", "\\u00e9", "\\ud83d\\ude00" };
    const std::vector<size_t> lengths = { 0, 1, 14, 15, 16, 21, 22, 23, 31, 32, 33, 63, 64, 100 };
    for (const std::string &escape : escapes) {
      for (size_t length : lengths) {
        contents.push_back(escape + sample_text(length));
        contents.push_back(sample_text(length / 2) + escape + sample_text(length - length / 2));
        contents.push_back(sample_text(length) + escape);
      }
    }

    // Multi-byte UTF-8 characters around the end of the inline buffer.
    for (size_t length = 12; length <= 24; length++) {
      contents.push_back(sample_text(length) + "\xc3\xa9");         // U+00E9, 2 bytes
      contents.push_back(sample_text(length) + "\xf0\x9f\x98\x80"); // U+1F600, 4 bytes
    }

    // Invalid escapes, and lone surrogates (only valid when replacement is allowed).
    contents.push_back("\\q");
    contents.push_back("abc\\x");
    contents.push_back("\\ud83d");
    contents.push_back("abc\\ude00def");
    contents.push_back(sample_text(30) + "\\ud83d");

    std::vector<std::string> strings;
    for (const std::string &content : contents) {
      strings.push_back("\"" + content + "\"");
    }
    return strings;
  }

  bool same_result(error_code error, const std::string &receiver, error_code expected_error, std::string_view expected) {
    ASSERT_ERROR(error, expected_error);
    if (!expected_error) {
      ASSERT_EQUAL(receiver, std::string(expected));
    }
    return true;
  }

  // Reads json_string as the first value of an array.
  bool check_array_value(const std::string &json_string, bool allow_replacement) {
    const padded_string json("[" + json_string + "]");
    ondemand::parser view_parser;
    ondemand::parser receiver_parser;
    ondemand::document view_doc;
    ondemand::document receiver_doc;
    ASSERT_SUCCESS(view_parser.iterate(json).get(view_doc));
    ASSERT_SUCCESS(receiver_parser.iterate(json).get(receiver_doc));

    std::string_view expected;
    const error_code expected_error = view_doc.at(0).get_string(allow_replacement).get(expected);
    std::string receiver = "previous content";
    const error_code error = receiver_doc.at(0).get_string(receiver, allow_replacement);
    return same_result(error, receiver, expected_error, expected);
  }

  // Reads json_string as the key of an object.
  bool check_object_key(const std::string &json_string, bool allow_replacement) {
    const padded_string json("{" + json_string + ":0}");
    ondemand::parser view_parser;
    ondemand::parser receiver_parser;
    ondemand::document view_doc;
    ondemand::document receiver_doc;
    ASSERT_SUCCESS(view_parser.iterate(json).get(view_doc));
    ASSERT_SUCCESS(receiver_parser.iterate(json).get(receiver_doc));

    ondemand::object view_object;
    ondemand::object receiver_object;
    ASSERT_SUCCESS(view_doc.get_object().get(view_object));
    ASSERT_SUCCESS(receiver_doc.get_object().get(receiver_object));
    ondemand::object_iterator view_it;
    ondemand::object_iterator receiver_it;
    ASSERT_SUCCESS(view_object.begin().get(view_it));
    ASSERT_SUCCESS(receiver_object.begin().get(receiver_it));
    ondemand::field view_field;
    ondemand::field receiver_field;
    ASSERT_SUCCESS((*view_it).get(view_field));
    ASSERT_SUCCESS((*receiver_it).get(receiver_field));

    std::string_view expected;
    const error_code expected_error = view_field.unescaped_key(allow_replacement).get(expected);
    std::string receiver = "previous content";
    const error_code error = receiver_field.unescaped_key(receiver, allow_replacement);
    return same_result(error, receiver, expected_error, expected);
  }

  // Reads json_string as the whole document.
  bool check_root_string(const std::string &json_string, bool allow_replacement) {
    const padded_string json(json_string);
    ondemand::parser view_parser;
    ondemand::parser receiver_parser;
    ondemand::document view_doc;
    ondemand::document receiver_doc;
    ASSERT_SUCCESS(view_parser.iterate(json).get(view_doc));
    ASSERT_SUCCESS(receiver_parser.iterate(json).get(receiver_doc));

    std::string_view expected;
    const error_code expected_error = view_doc.get_string(allow_replacement).get(expected);
    std::string receiver = "previous content";
    const error_code error = receiver_doc.get_string(receiver, allow_replacement);
    return same_result(error, receiver, expected_error, expected);
  }

#if SIMDJSON_SUPPORTS_CONCEPTS
  // Reads json_string with get<std::string>(), which never allows replacement.
  bool check_get_std_string(const std::string &json_string) {
    const padded_string json("[" + json_string + "]");
    ondemand::parser view_parser;
    ondemand::parser receiver_parser;
    ondemand::document view_doc;
    ondemand::document receiver_doc;
    ASSERT_SUCCESS(view_parser.iterate(json).get(view_doc));
    ASSERT_SUCCESS(receiver_parser.iterate(json).get(receiver_doc));

    std::string_view expected;
    const error_code expected_error = view_doc.at(0).get_string().get(expected);
    std::string receiver = "previous content";
    const error_code error = receiver_doc.at(0).get<std::string>().get(receiver);
    return same_result(error, receiver, expected_error, expected);
  }
#endif

  bool same_as_string_view() {
    TEST_START();
    for (const std::string &json_string : test_strings()) {
      for (bool allow_replacement : { false, true }) {
        bool ok = check_array_value(json_string, allow_replacement) &&
                  check_object_key(json_string, allow_replacement) &&
                  check_root_string(json_string, allow_replacement);
#if SIMDJSON_SUPPORTS_CONCEPTS
        if (!allow_replacement) {
          ok = ok && check_get_std_string(json_string);
        }
#endif
        if (!ok) {
          std::cerr << "Failed for " << json_string << " with allow_replacement = " << allow_replacement << std::endl;
          return false;
        }
      }
    }
    TEST_SUCCEED();
  }

  bool reused_receiver() {
    TEST_START();
    // Each string must fully replace the previous one, whether it is longer or shorter.
    const padded_string json("[\"" + std::string(100, 'x') + "\", \"short\", \"" + sample_text(40) + "\", \"with \\\"escapes\\\"\", \"\"]");
    const std::vector<std::string> expected = { std::string(100, 'x'), "short", sample_text(40), "with \"escapes\"", "" };

    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::string receiver;
    size_t index = 0;
    for (auto element : doc) {
      ASSERT_TRUE(index < expected.size());
      ASSERT_SUCCESS(element.get_string(receiver));
      ASSERT_EQUAL(receiver, expected[index]);
      index++;
    }
    ASSERT_EQUAL(index, expected.size());
    TEST_SUCCEED();
  }

  bool reserved_capacity_is_kept() {
    TEST_START();
    const padded_string json("[\"" + sample_text(40) + "\", \"short\", \"with\\nan escape and more than 22 characters\"]");
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));

    std::string receiver;
    receiver.reserve(1000);
    const size_t capacity = receiver.capacity();
    for (auto element : doc) {
      ASSERT_SUCCESS(element.get_string(receiver));
      ASSERT_EQUAL(receiver.capacity(), capacity);
    }
    TEST_SUCCEED();
  }

  bool earlier_string_views_stay_valid() {
    TEST_START();
    // A string_view from get_string() stays valid until the parser parses another document (see
    // doc/basics.md), so reading later strings into a std::string must not overwrite it.
    const padded_string json = R"(["first\nview", "with \"escape\"", "plain", "x"])"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));

    std::string_view first;
    std::string receiver;
    bool is_first = true;
    for (auto element : doc) {
      if (is_first) {
        ASSERT_SUCCESS(element.get_string().get(first));
        is_first = false;
      } else {
        ASSERT_SUCCESS(element.get_string(receiver));
      }
    }
    ASSERT_EQUAL(first, "first\nview");
    TEST_SUCCEED();
  }

  bool run() {
    return same_as_string_view() &&
           reused_receiver() &&
           reserved_capacity_is_kept() &&
           earlier_string_views_stay_valid();
  }

} // namespace string_receiver_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, string_receiver_tests::run);
}
