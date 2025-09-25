#include "simdjson.h"
#include "test_builder.h"
#include <string>
#include <string_view>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <optional>
#include <memory>

using namespace simdjson;

namespace builder_tests {

  bool test_primitive_types() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct PrimitiveTypes {
      bool bool_val;
      char char_val;
      int int_val;
      double double_val;
      float float_val;
    };

    PrimitiveTypes test{true, 'X', 42, 3.14159, 2.71f};

    auto result = builder::to_json_string(test);
    ASSERT_SUCCESS(result);

    std::string json = result.value();
    ASSERT_TRUE(json.find("\"bool_val\":true") != std::string::npos);
    ASSERT_TRUE(json.find("\"char_val\":\"X\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"int_val\":42") != std::string::npos);
    ASSERT_TRUE(json.find("\"double_val\":3.14159") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<PrimitiveTypes>();
    ASSERT_SUCCESS(get_result);

    PrimitiveTypes deserialized = std::move(get_result.value());
    ASSERT_EQUAL(deserialized.bool_val, test.bool_val);
    ASSERT_EQUAL(deserialized.char_val, test.char_val);
    ASSERT_EQUAL(deserialized.int_val, test.int_val);
    ASSERT_EQUAL(deserialized.double_val, test.double_val);
#endif
    TEST_SUCCEED();
  }

  bool test_string_types() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct StringTypes {
      std::string string_val;
      std::string_view string_view_val;
    };

    StringTypes test{"hello world", "test_view"};

    auto result = builder::to_json_string(test);
    ASSERT_SUCCESS(result);

    std::string json = result.value();
    ASSERT_TRUE(json.find("\"string_val\":\"hello world\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"string_view_val\":\"test_view\"") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<StringTypes>();
    ASSERT_SUCCESS(get_result);

    StringTypes deserialized = std::move(get_result.value());
    ASSERT_EQUAL(deserialized.string_val, test.string_val);
#endif
    TEST_SUCCEED();
  }

  bool test_optional_types() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct OptionalTypes {
      std::optional<int> opt_int_with_value;
      std::optional<std::string> opt_string_with_value;
      std::optional<int> opt_int_null;
      std::optional<std::string> opt_string_null;
    };

    OptionalTypes test;
    test.opt_int_with_value = 42;
    test.opt_string_with_value = "optional_test";
    test.opt_int_null = std::nullopt;
    test.opt_string_null = std::nullopt;

    auto result = builder::to_json_string(test);
    ASSERT_SUCCESS(result);

    std::string json = result.value();
    ASSERT_TRUE(json.find("\"opt_int_with_value\":42") != std::string::npos);
    ASSERT_TRUE(json.find("\"opt_string_with_value\":\"optional_test\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"opt_int_null\":null") != std::string::npos);
    ASSERT_TRUE(json.find("\"opt_string_null\":null") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<OptionalTypes>();
    ASSERT_SUCCESS(get_result);

    OptionalTypes deserialized = std::move(get_result.value());
    ASSERT_TRUE(deserialized.opt_int_with_value.has_value());
    ASSERT_EQUAL(*deserialized.opt_int_with_value, 42);
    ASSERT_TRUE(deserialized.opt_string_with_value.has_value());
    ASSERT_EQUAL(*deserialized.opt_string_with_value, "optional_test");
    ASSERT_FALSE(deserialized.opt_int_null.has_value());
    ASSERT_FALSE(deserialized.opt_string_null.has_value());
#endif
    TEST_SUCCEED();
  }

  bool test_smart_pointer_types() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct SmartPointerTypes {
      std::unique_ptr<int> unique_int_with_value;
      std::shared_ptr<std::string> shared_string_with_value;
      std::unique_ptr<bool> unique_bool_with_value;
      std::unique_ptr<int> unique_int_null;
      std::shared_ptr<std::string> shared_string_null;
    };

    SmartPointerTypes test;
    test.unique_int_with_value = std::make_unique<int>(123);
    test.shared_string_with_value = std::make_shared<std::string>("shared_test");
    test.unique_bool_with_value = std::make_unique<bool>(true);
    test.unique_int_null = nullptr;
    test.shared_string_null = nullptr;

    auto result = builder::to_json_string(test);
    ASSERT_SUCCESS(result);

    std::string json = result.value();
    ASSERT_TRUE(json.find("\"unique_int_with_value\":123") != std::string::npos);
    ASSERT_TRUE(json.find("\"shared_string_with_value\":\"shared_test\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"unique_bool_with_value\":true") != std::string::npos);
    ASSERT_TRUE(json.find("\"unique_int_null\":null") != std::string::npos);
    ASSERT_TRUE(json.find("\"shared_string_null\":null") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<SmartPointerTypes>();
    ASSERT_SUCCESS(get_result);

    SmartPointerTypes deserialized = std::move(get_result.value());
    ASSERT_TRUE(deserialized.unique_int_with_value != nullptr);
    ASSERT_EQUAL(*deserialized.unique_int_with_value, 123);
    ASSERT_TRUE(deserialized.shared_string_with_value != nullptr);
    ASSERT_EQUAL(*deserialized.shared_string_with_value, "shared_test");
    ASSERT_TRUE(deserialized.unique_bool_with_value != nullptr);
    ASSERT_EQUAL(*deserialized.unique_bool_with_value, true);
    ASSERT_TRUE(deserialized.unique_int_null == nullptr);
    ASSERT_TRUE(deserialized.shared_string_null == nullptr);
#endif
    TEST_SUCCEED();
  }

  bool test_container_types() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    // Test basic container types
    struct ContainerTypes {
      std::vector<int> int_vector;
      std::set<std::string> string_set;
      std::map<std::string, int> string_map;
    };

    ContainerTypes test;
    test.int_vector = {1, 2, 3, 4, 5};
    test.string_set = {"apple", "banana", "cherry"};
    test.string_map = {{"key1", 10}, {"key2", 20}, {"key3", 30}};

    auto result = builder::to_json_string(test);
    ASSERT_SUCCESS(result);

    std::string json = result.value();
    ASSERT_TRUE(json.find("\"int_vector\":[1,2,3,4,5]") != std::string::npos);
    ASSERT_TRUE(json.find("\"string_set\":[") != std::string::npos);
    ASSERT_TRUE(json.find("\"string_map\":{") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<ContainerTypes>();
    ASSERT_SUCCESS(get_result);

    ContainerTypes deserialized = std::move(get_result.value());
    ASSERT_EQUAL(deserialized.int_vector.size(), 5);
    ASSERT_EQUAL(deserialized.string_set.size(), 3);
    ASSERT_EQUAL(deserialized.string_map.size(), 3);
    ASSERT_EQUAL(deserialized.int_vector[0], 1);
    ASSERT_EQUAL(deserialized.int_vector[4], 5);

    // Test std::list with iterator-based serialization
    struct ListContainer {
      std::list<int> int_list;
      std::list<std::string> string_list;
    };

    ListContainer list_test;
    list_test.int_list = {10, 20, 30, 40, 50};
    list_test.string_list = {"first", "second", "third"};

    auto list_result = builder::to_json_string(list_test);
    ASSERT_SUCCESS(list_result);

    std::string list_json = list_result.value();
    // Check that list serialization produces correct JSON array format
    ASSERT_TRUE(list_json.find("\"int_list\":[10,20,30,40,50]") != std::string::npos);
    ASSERT_TRUE(list_json.find("\"string_list\":[\"first\",\"second\",\"third\"]") != std::string::npos);

    // Test list round-trip
    auto list_doc_result = parser.iterate(pad(list_json));
    ASSERT_SUCCESS(list_doc_result);

    auto list_get_result = list_doc_result.value().get<ListContainer>();
    ASSERT_SUCCESS(list_get_result);

    ListContainer list_deserialized = std::move(list_get_result.value());
    ASSERT_EQUAL(list_deserialized.int_list.size(), 5);
    ASSERT_EQUAL(list_deserialized.string_list.size(), 3);

    // Check list values
    auto it = list_deserialized.int_list.begin();
    ASSERT_EQUAL(*it, 10);
    std::advance(it, 4);
    ASSERT_EQUAL(*it, 50);

    auto str_it = list_deserialized.string_list.begin();
    ASSERT_EQUAL(*str_it, "first");
    std::advance(str_it, 2);
    ASSERT_EQUAL(*str_it, "third");
#endif
    TEST_SUCCEED();
  }


  bool run() {
    return test_primitive_types() &&
           test_string_types() &&
           test_optional_types() &&
           test_smart_pointer_types() &&
           test_container_types();
  }

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, builder_tests::run);
}