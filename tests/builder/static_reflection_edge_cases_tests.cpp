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
      std::string empty_string;
      std::vector<int> empty_vector;
      std::optional<int> null_optional;
      std::unique_ptr<int> null_unique_ptr;
      std::shared_ptr<std::string> null_shared_ptr;
    };

    EmptyValues test;
    test.empty_string = "";
    // empty_vector is already empty by default
    test.null_optional = std::nullopt;
    test.null_unique_ptr = nullptr;
    test.null_shared_ptr = nullptr;

    auto result = builder::to_json_string(test);
    ASSERT_SUCCESS(result);

    std::string json = result.value();
    ASSERT_TRUE(json.find("\"empty_string\":\"\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"empty_vector\":[]") != std::string::npos);
    ASSERT_TRUE(json.find("\"null_optional\":null") != std::string::npos);
    ASSERT_TRUE(json.find("\"null_unique_ptr\":null") != std::string::npos);
    ASSERT_TRUE(json.find("\"null_shared_ptr\":null") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<EmptyValues>();
    ASSERT_SUCCESS(get_result);

    EmptyValues deserialized = std::move(get_result.value());
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
      std::string quotes;
      std::string backslashes;
      std::string newlines;
      std::string unicode;
      char null_char;
    };

    SpecialChars test;
    test.quotes = "He said \"Hello\"";
    test.backslashes = "Path\\to\\file";
    test.newlines = "Line1\nLine2\tTabbed";
    test.unicode = "Café résumé";
    test.null_char = '\0';

    auto result = builder::to_json_string(test);
    ASSERT_SUCCESS(result);

    std::string json = result.value();
    // Test that quotes are properly escaped
    ASSERT_TRUE(json.find("\\\"Hello\\\"") != std::string::npos);
    // Test that backslashes are properly escaped
    ASSERT_TRUE(json.find("\\\\to\\\\") != std::string::npos);
    // Test that newlines are properly escaped
    ASSERT_TRUE(json.find("\\n") != std::string::npos);
    ASSERT_TRUE(json.find("\\t") != std::string::npos);

    // Test round-trip (excluding null char which has special handling)
    struct SpecialCharsNoNull {
      std::string quotes;
      std::string backslashes;
      std::string newlines;
      std::string unicode;
    };

    SpecialCharsNoNull test_no_null;
    test_no_null.quotes = test.quotes;
    test_no_null.backslashes = test.backslashes;
    test_no_null.newlines = test.newlines;
    test_no_null.unicode = test.unicode;

    auto result_no_null = builder::to_json_string(test_no_null);
    ASSERT_SUCCESS(result_no_null);

    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(result_no_null.value()));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<SpecialCharsNoNull>();
    ASSERT_SUCCESS(get_result);

    SpecialCharsNoNull deserialized = std::move(get_result.value());
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

    auto result = builder::to_json_string(test);
    ASSERT_SUCCESS(result);

    std::string json = result.value();
    ASSERT_TRUE(json.find("\"true_val\":true") != std::string::npos);
    ASSERT_TRUE(json.find("\"false_val\":false") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<NumericLimits>();
    ASSERT_SUCCESS(get_result);

    NumericLimits deserialized = std::move(get_result.value());
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
      int value;
      std::string name;
    };

    struct Outer {
      Inner inner_obj;
      std::vector<Inner> inner_vector;
      std::optional<Inner> optional_inner;
      std::unique_ptr<Inner> unique_inner;
    };

    Outer test;
    test.inner_obj = {42, "inner"};
    test.inner_vector = {{1, "first"}, {2, "second"}};
    test.optional_inner = Inner{99, "optional"};
    test.unique_inner = std::make_unique<Inner>(Inner{123, "unique"});

    auto result = builder::to_json_string(test);
    ASSERT_SUCCESS(result);

    std::string json = result.value();
    ASSERT_TRUE(json.find("\"inner_obj\":{") != std::string::npos);
    ASSERT_TRUE(json.find("\"inner_vector\":[") != std::string::npos);
    ASSERT_TRUE(json.find("\"optional_inner\":{") != std::string::npos);
    ASSERT_TRUE(json.find("\"unique_inner\":{") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<Outer>();
    ASSERT_SUCCESS(get_result);

    Outer deserialized = std::move(get_result.value());
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


  bool run() {
    return test_empty_values() &&
           test_special_characters() &&
           test_numeric_limits() &&
           test_nested_structures();
  }

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, builder_tests::run);
}