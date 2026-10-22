#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace key_string_tests {
#if SIMDJSON_EXCEPTIONS
  bool parser_key_value() {
    TEST_START();
    ondemand::parser parser;
    const padded_string json = R"({ "1": "1", "2": "2", "3": "3", "abc": "abc", "\u0075": "\u0075" })"_padded;
    auto doc = parser.iterate(json);
    for(auto field : doc.get_object())  {
      std::string_view keyv = field.unescaped_key();
      std::string_view valuev = field.value();
      if(keyv != valuev) { return false; }
    }
    return true;
  }

  bool parser_escaped_key() {
    TEST_START();
    ondemand::parser parser;
    const padded_string json = "{ \"1\": \"1\", \"2\"   : \"2\", \"3\" \t : \"3\", \"abc\"\n\t\n: \"abc\", \"\\u0075\": \"\\\\u0075\" }"_padded;
    auto doc = parser.iterate(json);
    for(auto field : doc.get_object())  {
      std::string_view keyv = field.escaped_key();
      std::string_view valuev = field.value();
      if(keyv != valuev) { return false; }
    }
    return true;
  }

#endif // SIMDJSON_EXCEPTIONS
  // escaped_key() and key_raw_json_token() must not depend on whether the
  // value has been consumed.
  bool key_after_value_consumed() {
    TEST_START();
    ondemand::parser parser;
    const padded_string json = R"({"name":"Alice","tags":["x","y"],"age" :42,"obj":{"a":1}})"_padded;
    const char *expected_keys[] = {"name", "tags", "age", "obj"};
    const char *expected_tokens[] = {"\"name\"", "\"tags\"", "\"age\" ", "\"obj\""};
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::object object;
    ASSERT_SUCCESS(doc.get_object().get(object));
    size_t i = 0;
    for (auto field_result : object) {
      ondemand::field field;
      ASSERT_SUCCESS(std::move(field_result).get(field));
      ASSERT_TRUE(i < 4);
      // Consume the value before looking at the key.
      if (i == 0) {
        std::string_view name;
        ASSERT_SUCCESS(field.value().get_string().get(name));
        ASSERT_EQUAL(name, "Alice");
      } else if (i == 2) {
        int64_t age;
        ASSERT_SUCCESS(field.value().get_int64().get(age));
        ASSERT_EQUAL(age, 42);
      } else {
        std::string_view raw_json;
        ASSERT_SUCCESS(field.value().raw_json().get(raw_json));
      }
      ASSERT_EQUAL(field.escaped_key(), expected_keys[i]);
      ASSERT_EQUAL(field.key_raw_json_token(), expected_tokens[i]);
      i++;
    }
    ASSERT_EQUAL(i, 4);
    TEST_SUCCEED();
  }

  bool run() {
    return
#if SIMDJSON_EXCEPTIONS
      parser_key_value() &&
      parser_escaped_key() &&
#endif // SIMDJSON_EXCEPTIONS
      key_after_value_consumed() &&
      true;
  }

} // namespace key_string_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, key_string_tests::run);
}
