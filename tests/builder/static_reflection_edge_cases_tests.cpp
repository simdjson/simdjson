#include "simdjson.h"
#include "test_builder.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <limits>

using namespace simdjson;

namespace builder_tests {

  bool test_empty_values() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct EmptyValues {
      std::string empty_string{};
      std::vector<int> empty_vector{};
      std::optional<int> null_optional{};
      std::unique_ptr<int> null_unique_ptr{};
      std::shared_ptr<std::string> null_shared_ptr{};
    };

    EmptyValues test;
    test.empty_string = "";
    // empty_vector is already empty by default
    test.null_optional = std::nullopt;
    test.null_unique_ptr = nullptr;
    test.null_shared_ptr = nullptr;

    std::string json;
    ASSERT_SUCCESS(builder::to_json_string(test).get(json));
    ASSERT_TRUE(json.find("\"empty_string\":\"\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"empty_vector\":[]") != std::string::npos);
    ASSERT_TRUE(json.find("\"null_optional\":null") != std::string::npos);
    ASSERT_TRUE(json.find("\"null_unique_ptr\":null") != std::string::npos);
    ASSERT_TRUE(json.find("\"null_shared_ptr\":null") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    EmptyValues deserialized;
    ASSERT_SUCCESS(doc_result.get<EmptyValues>().get(deserialized));
    ASSERT_EQUAL(deserialized.empty_string, "");
    ASSERT_EQUAL(deserialized.empty_vector.size(), 0);
    ASSERT_FALSE(deserialized.null_optional.has_value());
    ASSERT_TRUE(deserialized.null_unique_ptr == nullptr);
    ASSERT_TRUE(deserialized.null_shared_ptr == nullptr);
#endif
    TEST_SUCCEED();
  }

  bool test_special_characters() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct SpecialChars {
      std::string quotes{};
      std::string backslashes{};
      std::string newlines{};
      std::string unicode{};
      char null_char{};
    };

    SpecialChars test;
    test.quotes = "He said \"Hello\"";
    test.backslashes = "Path\\to\\file";
    test.newlines = "Line1\nLine2\tTabbed";
    test.unicode = "Caf\xc3\xa9 r\xc3\xa9sum\xc3\xa9";
    test.null_char = '\0';

    std::string json;
    ASSERT_SUCCESS(builder::to_json_string(test).get(json));
    // Test that quotes are properly escaped
    ASSERT_TRUE(json.find("\\\"Hello\\\"") != std::string::npos);
    // Test that backslashes are properly escaped
    ASSERT_TRUE(json.find("\\\\to\\\\") != std::string::npos);
    // Test that newlines are properly escaped
    ASSERT_TRUE(json.find("\\n") != std::string::npos);
    ASSERT_TRUE(json.find("\\t") != std::string::npos);

    // Test round-trip (excluding null char which has special handling)
    struct SpecialCharsNoNull {
      std::string quotes{};
      std::string backslashes{};
      std::string newlines{};
      std::string unicode{};
    };

    SpecialCharsNoNull test_no_null;
    test_no_null.quotes = test.quotes;
    test_no_null.backslashes = test.backslashes;
    test_no_null.newlines = test.newlines;
    test_no_null.unicode = test.unicode;

    std::string result_no_null;
    ASSERT_SUCCESS(builder::to_json_string(test_no_null).get(result_no_null));

    ondemand::parser parser;
    ondemand::document doc_result;

    ASSERT_SUCCESS(parser.iterate(pad(result_no_null)).get(doc_result));

    SpecialCharsNoNull deserialized;
    ASSERT_SUCCESS(doc_result.get<SpecialCharsNoNull>().get(deserialized));

    ASSERT_EQUAL(deserialized.quotes, test.quotes);
    ASSERT_EQUAL(deserialized.backslashes, test.backslashes);
    ASSERT_EQUAL(deserialized.newlines, test.newlines);
    ASSERT_EQUAL(deserialized.unicode, test.unicode);
#endif
    TEST_SUCCEED();
  }

  bool test_numeric_limits() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct NumericLimits {
      int max_int;
      int min_int;
      double max_double;
      double min_double;
      bool true_val;
      bool false_val;
    };

    NumericLimits test;
    test.max_int = std::numeric_limits<int>::max();
    test.min_int = std::numeric_limits<int>::min();
    test.max_double = 1e100;  // Large but safe double value
    test.min_double = -1e100; // Large negative but safe double value
    test.true_val = true;
    test.false_val = false;

    std::string json;
    ASSERT_SUCCESS(builder::to_json_string(test).get(json));
    ASSERT_TRUE(json.find("\"true_val\":true") != std::string::npos);
    ASSERT_TRUE(json.find("\"false_val\":false") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    ondemand::document doc_result;

    ASSERT_SUCCESS(parser.iterate(pad(json)).get(doc_result));
    NumericLimits deserialized;
    ASSERT_SUCCESS(doc_result.get<NumericLimits>().get(deserialized));
    ASSERT_EQUAL(deserialized.max_int, test.max_int);
    ASSERT_EQUAL(deserialized.min_int, test.min_int);
    ASSERT_EQUAL(deserialized.true_val, true);
    ASSERT_EQUAL(deserialized.false_val, false);
#endif
    TEST_SUCCEED();
  }

  bool test_nested_structures() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct Inner {
      int value{};
      std::string name{};
    };

    struct Outer {
      Inner inner_obj{};
      std::vector<Inner> inner_vector{};
      std::optional<Inner> optional_inner{};
      std::unique_ptr<Inner> unique_inner{};
    };

    Outer test;
    test.inner_obj = {42, "inner"};
    test.inner_vector = {{1, "first"}, {2, "second"}};
    test.optional_inner = Inner{99, "optional"};
    test.unique_inner = std::make_unique<Inner>(Inner{123, "unique"});

    std::string json;
    ASSERT_SUCCESS(builder::to_json_string(test).get(json));
    ASSERT_TRUE(json.find("\"inner_obj\":{") != std::string::npos);
    ASSERT_TRUE(json.find("\"inner_vector\":[") != std::string::npos);
    ASSERT_TRUE(json.find("\"optional_inner\":{") != std::string::npos);
    ASSERT_TRUE(json.find("\"unique_inner\":{") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    Outer deserialized;
    ASSERT_SUCCESS(doc_result.get<Outer>().get(deserialized));
    ASSERT_EQUAL(deserialized.inner_obj.value, 42);
    ASSERT_EQUAL(deserialized.inner_obj.name, "inner");
    ASSERT_EQUAL(deserialized.inner_vector.size(), 2);
    ASSERT_EQUAL(deserialized.inner_vector[0].value, 1);
    ASSERT_EQUAL(deserialized.inner_vector[1].name, "second");
    ASSERT_TRUE(deserialized.optional_inner.has_value());
    ASSERT_EQUAL(deserialized.optional_inner->value, 99);
    ASSERT_TRUE(deserialized.unique_inner != nullptr);
    ASSERT_EQUAL(deserialized.unique_inner->value, 123);
#endif
    TEST_SUCCEED();
  }


  // https://github.com/simdjson/simdjson/issues/2827
  // Since C++26, std::optional is a range, so it used to match both the
  // container and the optional serialization overloads.
  bool test_issue2827_optional() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct Foo {
      std::optional<std::string> maybe{};
    };

    Foo foo{"bar"};
    std::string json;
    ASSERT_SUCCESS(simdjson::to_json(foo).get(json));
    ASSERT_EQUAL(json, "{\"maybe\":\"bar\"}");

    Foo empty;
    ASSERT_SUCCESS(simdjson::to_json(empty).get(json));
    ASSERT_EQUAL(json, "{\"maybe\":null}");

    // Optionals used directly, not as a struct member.
    std::optional<std::string> optional_string = "bar";
    ASSERT_SUCCESS(simdjson::to_json(optional_string).get(json));
    ASSERT_EQUAL(json, "\"bar\"");

    std::optional<std::string> empty_string = std::nullopt;
    ASSERT_SUCCESS(simdjson::to_json(empty_string).get(json));
    ASSERT_EQUAL(json, "null");

    std::optional<Foo> optional_struct = Foo{"x"};
    ASSERT_SUCCESS(simdjson::to_json(optional_struct).get(json));
    ASSERT_EQUAL(json, "{\"maybe\":\"x\"}");

    std::vector<std::optional<int>> vector_of_optionals = {1, std::nullopt, 3};
    ASSERT_SUCCESS(simdjson::to_json(vector_of_optionals).get(json));
    ASSERT_EQUAL(json, "[1,null,3]");

    std::optional<std::vector<int>> optional_vector = std::vector<int>{1, 2};
    ASSERT_SUCCESS(simdjson::to_json(optional_vector).get(json));
    ASSERT_EQUAL(json, "[1,2]");

    // Round-trip the struct.
    ondemand::parser parser;
    std::string document = "{\"maybe\":\"bar\"}";
    auto doc_result = parser.iterate(pad(document));
    ASSERT_SUCCESS(doc_result);
    Foo deserialized;
    ASSERT_SUCCESS(doc_result.get<Foo>().get(deserialized));
    ASSERT_TRUE(deserialized.maybe.has_value());
    ASSERT_EQUAL(*deserialized.maybe, "bar");
#endif
    TEST_SUCCEED();
  }

  bool run() {
    return test_empty_values() &&
           test_special_characters() &&
           test_numeric_limits() &&
           test_nested_structures() &&
           test_issue2827_optional();
  }

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, builder_tests::run);
}
