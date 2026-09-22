#include "simdjson.h"
#include "test_ondemand.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

using namespace simdjson;

// The C++20 char8_t variants of our string accessors are only available when the
// compiler provides char8_t. Under C++17 (and earlier), this whole file is a no-op.
//
// Note: we keep this source file pure ASCII. Non-ASCII characters are spelled out
// with \u escapes, both in the JSON inputs and in the expected u8 literals.
namespace u8string_tests {
#if SIMDJSON_SUPPORTS_CHAR8_T

  // { "make": "Toyota", "key": "caf\u00e9", "tags": ["a", "b"] }
  const padded_string BASIC_JSON =
      "{ \"make\": \"Toyota\", \"key\": \"caf\\u00e9\", \"tags\": [\"a\", \"b\"] }"_padded;

  bool value_get_u8string() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(BASIC_JSON).get(doc) );
    std::u8string_view view;
    ASSERT_SUCCESS( doc["make"].get_u8string().get(view) );
    ASSERT_TRUE( view == u8"Toyota" );
    ASSERT_EQUAL( view.size(), 6 );
    TEST_SUCCEED();
  }

  // "caf\u00e9" unescapes to five UTF-8 bytes: "caf" followed by U+00E9.
  bool value_get_u8string_unicode() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(BASIC_JSON).get(doc) );
    std::u8string_view view;
    ASSERT_SUCCESS( doc["key"].get_u8string().get(view) );
    ASSERT_TRUE( view == u8"caf\u00e9" );
    ASSERT_EQUAL( view.size(), 5 );
    TEST_SUCCEED();
  }

  // The u8 accessors are a pure reinterpretation: they must hand back exactly the
  // bytes that get_string() hands back.
  bool value_get_u8string_same_bytes() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(BASIC_JSON).get(doc) );
    std::string_view narrow;
    ASSERT_SUCCESS( doc["key"].get_string().get(narrow) );
    doc.rewind();
    std::u8string_view wide;
    ASSERT_SUCCESS( doc["key"].get_u8string().get(wide) );
    ASSERT_EQUAL( wide.size(), narrow.size() );
    ASSERT_TRUE( std::equal(narrow.begin(), narrow.end(), wide.begin(),
                            [](char a, char8_t b) { return static_cast<unsigned char>(a) == b; }) );
    TEST_SUCCEED();
  }

  bool value_get_u8string_incorrect_type() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"({ "n": 1 })"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(json).get(doc) );
    ASSERT_ERROR( doc["n"].get_u8string(), INCORRECT_TYPE );
    TEST_SUCCEED();
  }

  // The allow_replacement flag is forwarded, just like it is by get_string().
  bool value_get_u8string_replacement() {
    TEST_START();
    ondemand::parser parser;
    auto json = "{ \"deviceId\": \"431924697b\\udff0L\\u0001Y\" }"_padded;
    {
      ondemand::document doc;
      ASSERT_SUCCESS( parser.iterate(json).get(doc) );
      ASSERT_ERROR( doc["deviceId"].get_u8string(), STRING_ERROR );
    }
    {
      ondemand::document doc;
      ASSERT_SUCCESS( parser.iterate(json).get(doc) );
      std::u8string_view view;
      ASSERT_SUCCESS( doc["deviceId"].get_u8string(true).get(view) );
      // The unpaired surrogate becomes the replacement character U+FFFD.
      ASSERT_TRUE( view.find(u8"\ufffd") != std::u8string_view::npos );
    }
    TEST_SUCCEED();
  }

  bool document_get_u8string() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"("a document that is just a string")"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(json).get(doc) );
    std::u8string_view view;
    ASSERT_SUCCESS( doc.get_u8string().get(view) );
    ASSERT_TRUE( view == u8"a document that is just a string" );
    TEST_SUCCEED();
  }

  // simdjson_result<document> forwards get_u8string(), error included.
  bool document_result_get_u8string() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"("hello")"_padded;
    std::u8string_view view;
    ASSERT_SUCCESS( parser.iterate(json).get_u8string().get(view) );
    ASSERT_TRUE( view == u8"hello" );
    auto empty = ""_padded;
    ASSERT_ERROR( parser.iterate(empty).get_u8string(), EMPTY );
    TEST_SUCCEED();
  }

  // iterate_many hands out document_reference instances.
  bool document_reference_get_u8string() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"("one" "two" "three")"_padded;
    ondemand::document_stream stream;
    ASSERT_SUCCESS( parser.iterate_many(json).get(stream) );
    const std::u8string_view expected[3] = { u8"one", u8"two", u8"three" };
    size_t count = 0;
    for (auto doc : stream) {
      ASSERT_TRUE( count < 3 );
      std::u8string_view view;
      ASSERT_SUCCESS( doc.get_u8string().get(view) );
      ASSERT_TRUE( view == expected[count] );
      count++;
    }
    ASSERT_EQUAL( count, 3 );
    TEST_SUCCEED();
  }

  bool field_u8key() {
    TEST_START();
    ondemand::parser parser;
    auto json = "{ \"caf\\u00e9\": \"value\" }"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(json).get(doc) );
    ondemand::object object;
    ASSERT_SUCCESS( doc.get_object().get(object) );
    size_t count = 0;
    for (auto field : object) {
      // escaped_u8key() hands back the raw, still escaped key.
      std::u8string_view escaped;
      ASSERT_SUCCESS( field.escaped_u8key().get(escaped) );
      ASSERT_TRUE( escaped == u8"caf\\u00e9" );
      // unescaped_u8key() hands back the unescaped key.
      std::u8string_view unescaped;
      ASSERT_SUCCESS( field.unescaped_u8key().get(unescaped) );
      ASSERT_TRUE( unescaped == u8"caf\u00e9" );
      count++;
    }
    ASSERT_EQUAL( count, 1 );
    TEST_SUCCEED();
  }

  // The templated receiver overloads must accept a std::u8string.
  bool receiver_u8string() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(BASIC_JSON).get(doc) );
    ondemand::object object;
    ASSERT_SUCCESS( doc.get_object().get(object) );
    std::u8string value;
    ASSERT_SUCCESS( object["make"].get_string(value) );
    ASSERT_TRUE( value == u8"Toyota" );
    doc.rewind();
    ASSERT_SUCCESS( doc.get_object().get(object) );
    for (auto field : object) {
      std::u8string key;
      ASSERT_SUCCESS( field.unescaped_key(key) );
      ASSERT_TRUE( key == u8"make" );
      break;
    }
    TEST_SUCCEED();
  }

  bool document_receiver_u8string() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"("root")"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(json).get(doc) );
    std::u8string value;
    ASSERT_SUCCESS( doc.get_string(value) );
    ASSERT_TRUE( value == u8"root" );
    doc.rewind();
    std::u8string_view view;
    ASSERT_SUCCESS( doc.get_string(view) );
    ASSERT_TRUE( view == u8"root" );
    TEST_SUCCEED();
  }

  bool get_u8string_view_template() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(BASIC_JSON).get(doc) );
    std::u8string_view view;
    ASSERT_SUCCESS( doc["make"].get<std::u8string_view>().get(view) );
    ASSERT_TRUE( view == u8"Toyota" );
    // and on the document itself
    auto json = R"("root")"_padded;
    ondemand::document doc2;
    ASSERT_SUCCESS( parser.iterate(json).get(doc2) );
    std::u8string_view root;
    ASSERT_SUCCESS( doc2.get<std::u8string_view>().get(root) );
    ASSERT_TRUE( root == u8"root" );
    TEST_SUCCEED();
  }

#if SIMDJSON_SUPPORTS_DESERIALIZATION
  bool get_u8string_template() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(BASIC_JSON).get(doc) );
    std::u8string value;
    ASSERT_SUCCESS( doc["make"].get<std::u8string>().get(value) );
    ASSERT_TRUE( value == u8"Toyota" );
    TEST_SUCCEED();
  }

  bool container_of_u8string() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(BASIC_JSON).get(doc) );
    std::vector<std::u8string> tags;
    ASSERT_SUCCESS( doc["tags"].get<std::vector<std::u8string>>().get(tags) );
    ASSERT_EQUAL( tags.size(), 2 );
    ASSERT_TRUE( tags[0] == u8"a" );
    ASSERT_TRUE( tags[1] == u8"b" );
    doc.rewind();
    std::vector<std::u8string_view> views;
    ASSERT_SUCCESS( doc["tags"].get<std::vector<std::u8string_view>>().get(views) );
    ASSERT_EQUAL( views.size(), 2 );
    ASSERT_TRUE( views[0] == u8"a" );
    ASSERT_TRUE( views[1] == u8"b" );
    TEST_SUCCEED();
  }

  bool optional_u8string() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"({ "a": "x", "b": null })"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(json).get(doc) );
    std::optional<std::u8string> a;
    ASSERT_SUCCESS( doc["a"].get<std::optional<std::u8string>>().get(a) );
    ASSERT_TRUE( a.has_value() );
    ASSERT_TRUE( *a == u8"x" );
    doc.rewind();
    std::optional<std::u8string> b;
    ASSERT_SUCCESS( doc["b"].get<std::optional<std::u8string>>().get(b) );
    ASSERT_TRUE( !b.has_value() );
    TEST_SUCCEED();
  }

  // Adding the char8_t overloads must not disturb the char-based ones.
  bool narrow_types_still_work() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(BASIC_JSON).get(doc) );
    std::string value;
    ASSERT_SUCCESS( doc["make"].get<std::string>().get(value) );
    ASSERT_EQUAL( value, "Toyota" );
    doc.rewind();
    std::vector<std::string> tags;
    ASSERT_SUCCESS( doc["tags"].get<std::vector<std::string>>().get(tags) );
    ASSERT_EQUAL( tags.size(), 2 );
    ASSERT_EQUAL( tags[0], "a" );
    doc.rewind();
    std::string receiver;
    ASSERT_SUCCESS( doc["make"].get_string(receiver) );
    ASSERT_EQUAL( receiver, "Toyota" );
    TEST_SUCCEED();
  }
#endif // SIMDJSON_SUPPORTS_DESERIALIZATION

#if SIMDJSON_EXCEPTIONS
  bool exceptions_u8string() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(BASIC_JSON).get(doc) );
    std::u8string_view view = doc["make"].get_u8string();
    ASSERT_TRUE( view == u8"Toyota" );
    TEST_SUCCEED();
  }
#endif // SIMDJSON_EXCEPTIONS

#endif // SIMDJSON_SUPPORTS_CHAR8_T

  bool run() {
    return
#if SIMDJSON_SUPPORTS_CHAR8_T
      value_get_u8string() &&
      value_get_u8string_unicode() &&
      value_get_u8string_same_bytes() &&
      value_get_u8string_incorrect_type() &&
      value_get_u8string_replacement() &&
      document_get_u8string() &&
      document_result_get_u8string() &&
      document_reference_get_u8string() &&
      field_u8key() &&
      receiver_u8string() &&
      document_receiver_u8string() &&
      get_u8string_view_template() &&
#if SIMDJSON_SUPPORTS_DESERIALIZATION
      get_u8string_template() &&
      container_of_u8string() &&
      optional_u8string() &&
      narrow_types_still_work() &&
#endif // SIMDJSON_SUPPORTS_DESERIALIZATION
#if SIMDJSON_EXCEPTIONS
      exceptions_u8string() &&
#endif // SIMDJSON_EXCEPTIONS
#endif // SIMDJSON_SUPPORTS_CHAR8_T
      true;
  }

} // namespace u8string_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, u8string_tests::run);
}
