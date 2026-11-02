#include "simdjson.h"
#include "test_ondemand.h"

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <stack>
#include <queue>
#include <deque>
#include <unordered_set>
#include <optional>
#include <memory>
#include <limits>
#include <stdexcept>

#if SIMDJSON_SUPPORTS_CONCEPTS
class Array : public std::vector<float> {};
static_assert(simdjson::concepts::appendable_containers<Array>, "Array must be appendable_containers");
static_assert(!simdjson::require_custom_serialization<Array>);
namespace simdjson {
// This tag_invoke MUST be inside simdjson namespace
template <typename simdjson_value>
auto tag_invoke(deserialize_tag, simdjson_value &val, Array& out) {
    simdjson::ondemand::array arr;
    SIMDJSON_TRY(val.get_array().get(arr));
    for (auto v : arr) {
      float temp;
      if (auto const err = v.get<float>().get(temp); err) {
        return err;
      }
      out.push_back(temp);
      out.push_back(temp); // adding it twice
    }
    return error_code::SUCCESS;
}

#if SIMDJSON_EXCEPTIONS
// Deserializes an integer, but throws (after writing to out) if it is negative.
struct throwing_int { int64_t value{0}; };
template <typename simdjson_value>
simdjson::error_code tag_invoke(deserialize_tag, simdjson_value &val, throwing_int& out) {
  SIMDJSON_TRY(val.get_int64().get(out.value));
  if (out.value < 0) { throw std::runtime_error("negative"); }
  return error_code::SUCCESS;
}
#endif // SIMDJSON_EXCEPTIONS
} // namespace simdjson

// require_custom_serialization
#endif // SIMDJSON_SUPPORTS_CONCEPTS

namespace stl_types {
#if SIMDJSON_SUPPORTS_CONCEPTS
// Smart pointers to built-in types go through the generic smart-pointer
// tag_invoke: same range checks as get<T>(), null resets the pointer, and a
// failed parse leaves the pointer untouched.
template <template <typename...> class Ptr, typename T>
bool smart_pointer_case(std::string_view json, simdjson::error_code expected, T expected_value = T{}) {
  simdjson::ondemand::parser parser;
  // As a document.
  {
    simdjson::padded_string padded(json);
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    Ptr<T> out;
    ASSERT_EQUAL(doc.get(out), expected);
    if (expected == simdjson::SUCCESS) {
      ASSERT_TRUE(out != nullptr);
      ASSERT_TRUE(*out == expected_value);
    } else {
      ASSERT_TRUE(out == nullptr);
    }
  }
  // As an object field (ondemand::value).
  {
    simdjson::padded_string padded(std::string(R"({"a":)") + std::string(json) + "}");
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    Ptr<T> out;
    ASSERT_EQUAL(doc["a"].get(out), expected);
    if (expected == simdjson::SUCCESS) {
      ASSERT_TRUE(out != nullptr);
      ASSERT_TRUE(*out == expected_value);
    } else {
      ASSERT_TRUE(out == nullptr);
    }
  }
  // null resets a non-null pointer.
  {
    simdjson::padded_string padded(std::string_view(R"({"a":null})"));
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    Ptr<T> out(new T{});
    ASSERT_SUCCESS(doc["a"].get(out));
    ASSERT_TRUE(out == nullptr);
  }
  return true;
}

template <template <typename...> class Ptr>
bool smart_pointer_builtin_types() {
  using simdjson::SUCCESS;
  using simdjson::INCORRECT_TYPE;
  using simdjson::NUMBER_OUT_OF_RANGE;
  return smart_pointer_case<Ptr, bool>("true", SUCCESS, true) &&
         smart_pointer_case<Ptr, bool>("1", INCORRECT_TYPE) &&
         smart_pointer_case<Ptr, int>("-2147483648", SUCCESS, (std::numeric_limits<int>::min)()) &&
         smart_pointer_case<Ptr, int>("2147483647", SUCCESS, (std::numeric_limits<int>::max)()) &&
         smart_pointer_case<Ptr, int>("2147483648", NUMBER_OUT_OF_RANGE) &&
         smart_pointer_case<Ptr, int>("-2147483649", NUMBER_OUT_OF_RANGE) &&
         smart_pointer_case<Ptr, int>("4294967297", NUMBER_OUT_OF_RANGE) &&
         smart_pointer_case<Ptr, int>("\"1\"", INCORRECT_TYPE) &&
         smart_pointer_case<Ptr, int64_t>("-9223372036854775808", SUCCESS, (std::numeric_limits<int64_t>::min)()) &&
         smart_pointer_case<Ptr, int64_t>("9223372036854775808", INCORRECT_TYPE) &&
         smart_pointer_case<Ptr, uint64_t>("18446744073709551615", SUCCESS, (std::numeric_limits<uint64_t>::max)()) &&
         smart_pointer_case<Ptr, uint64_t>("-1", INCORRECT_TYPE) &&
         smart_pointer_case<Ptr, double>("1.5", SUCCESS, 1.5) &&
         smart_pointer_case<Ptr, double>("true", INCORRECT_TYPE) &&
         smart_pointer_case<Ptr, std::string>("\"h\\u00e9llo\"", SUCCESS, std::string("h\xc3\xa9llo")) &&
         smart_pointer_case<Ptr, std::string>("12", INCORRECT_TYPE) &&
         smart_pointer_case<Ptr, std::string_view>("\"hello\"", SUCCESS, std::string_view("hello")) &&
         smart_pointer_case<Ptr, std::string_view>("12", INCORRECT_TYPE);
}

bool smart_pointers_to_builtin_types() {
  TEST_START();
  ASSERT_TRUE(smart_pointer_builtin_types<std::unique_ptr>());
  ASSERT_TRUE(smart_pointer_builtin_types<std::shared_ptr>());
  TEST_SUCCEED();
}

// Optionals of built-in types go through the generic optional tag_invoke:
// same range checks as get<T>(), null resets the optional, and a failed parse
// leaves the optional untouched.
template <typename T>
bool optional_case(std::string_view json, simdjson::error_code expected, T expected_value = T{}) {
  simdjson::ondemand::parser parser;
  // As a document.
  {
    simdjson::padded_string padded(json);
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    std::optional<T> out;
    ASSERT_EQUAL(doc.get(out), expected);
    if (expected == simdjson::SUCCESS) {
      ASSERT_TRUE(out.has_value());
      ASSERT_TRUE(*out == expected_value);
    } else {
      ASSERT_FALSE(out.has_value());
    }
  }
  // As an object field (ondemand::value).
  {
    simdjson::padded_string padded(std::string(R"({"a":)") + std::string(json) + "}");
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    std::optional<T> out;
    ASSERT_EQUAL(doc["a"].get(out), expected);
    if (expected == simdjson::SUCCESS) {
      ASSERT_TRUE(out.has_value());
      ASSERT_TRUE(*out == expected_value);
    } else {
      ASSERT_FALSE(out.has_value());
    }
  }
  // null resets a non-null optional.
  {
    simdjson::padded_string padded(std::string_view(R"({"a":null})"));
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    std::optional<T> out(expected_value);
    ASSERT_SUCCESS(doc["a"].get(out));
    ASSERT_FALSE(out.has_value());
  }
  // An error leaves an existing value untouched.
  if (expected != simdjson::SUCCESS) {
    simdjson::padded_string padded(std::string(R"({"a":)") + std::string(json) + "}");
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    std::optional<T> out(expected_value);
    ASSERT_EQUAL(doc["a"].get(out), expected);
    ASSERT_TRUE(out.has_value());
    ASSERT_TRUE(*out == expected_value);
  }
  return true;
}

bool optionals_to_builtin_types() {
  TEST_START();
  using simdjson::SUCCESS;
  using simdjson::INCORRECT_TYPE;
  using simdjson::NUMBER_OUT_OF_RANGE;
  ASSERT_TRUE(optional_case<bool>("true", SUCCESS, true));
  ASSERT_TRUE(optional_case<bool>("1", INCORRECT_TYPE));
  ASSERT_TRUE(optional_case<int>("-2147483648", SUCCESS, (std::numeric_limits<int>::min)()));
  ASSERT_TRUE(optional_case<int>("2147483647", SUCCESS, (std::numeric_limits<int>::max)()));
  ASSERT_TRUE(optional_case<int>("2147483648", NUMBER_OUT_OF_RANGE));
  ASSERT_TRUE(optional_case<int>("-2147483649", NUMBER_OUT_OF_RANGE));
  ASSERT_TRUE(optional_case<int>("4294967297", NUMBER_OUT_OF_RANGE));
  ASSERT_TRUE(optional_case<int>("\"1\"", INCORRECT_TYPE));
  ASSERT_TRUE(optional_case<int64_t>("-9223372036854775808", SUCCESS, (std::numeric_limits<int64_t>::min)()));
  ASSERT_TRUE(optional_case<int64_t>("9223372036854775808", INCORRECT_TYPE));
  ASSERT_TRUE(optional_case<uint64_t>("18446744073709551615", SUCCESS, (std::numeric_limits<uint64_t>::max)()));
  ASSERT_TRUE(optional_case<uint64_t>("-1", INCORRECT_TYPE));
  ASSERT_TRUE(optional_case<double>("1.5", SUCCESS, 1.5));
  ASSERT_TRUE(optional_case<double>("true", INCORRECT_TYPE));
  ASSERT_TRUE(optional_case<std::string>("\"h\\u00e9llo\"", SUCCESS, std::string("h\xc3\xa9llo")));
  ASSERT_TRUE(optional_case<std::string>("12", INCORRECT_TYPE));
  ASSERT_TRUE(optional_case<std::string_view>("\"hello\"", SUCCESS, std::string_view("hello")));
  ASSERT_TRUE(optional_case<std::string_view>("12", INCORRECT_TYPE));
  TEST_SUCCEED();
}

// An optional holding a container: a successful parse replaces the container
// (a plain container is appended to), a failed parse leaves it untouched.
bool optional_containers() {
  TEST_START();
  simdjson::ondemand::parser parser;
  {
    auto json = R"([3])"_padded;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::optional<std::vector<int>> out(std::vector<int>{1, 2});
    ASSERT_SUCCESS(doc.get(out));
    ASSERT_TRUE(out.has_value());
    ASSERT_TRUE(*out == std::vector<int>{3});
  }
  {
    auto json = R"([1,2,"x"])"_padded;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::optional<std::vector<int>> out;
    ASSERT_EQUAL(doc.get(out), simdjson::INCORRECT_TYPE);
    ASSERT_FALSE(out.has_value());
  }
  {
    auto json = R"([1,2,"x"])"_padded;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::optional<std::vector<int>> out(std::vector<int>{7});
    ASSERT_EQUAL(doc.get(out), simdjson::INCORRECT_TYPE);
    ASSERT_TRUE(out.has_value());
    ASSERT_TRUE(*out == std::vector<int>{7});
  }
  {
    auto json = R"({"a":[4,5]})"_padded;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::optional<std::vector<int>> out;
    ASSERT_SUCCESS(doc["a"].get(out));
    ASSERT_TRUE(out.has_value());
    ASSERT_TRUE((*out == std::vector<int>{4, 5}));
  }
  TEST_SUCCEED();
}

#if SIMDJSON_EXCEPTIONS
// An exception thrown by a user tag_invoke also leaves the optional untouched.
bool optional_exception_untouched() {
  TEST_START();
  simdjson::ondemand::parser parser;
  for (bool engaged : {false, true}) {
    auto json = R"({"a":-1})"_padded;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::optional<simdjson::throwing_int> out;
    if (engaged) { out.emplace(simdjson::throwing_int{42}); }
    bool thrown = false;
    try {
      (void)doc["a"].get(out);
    } catch (const std::runtime_error &) {
      thrown = true;
    }
    ASSERT_TRUE(thrown);
    ASSERT_EQUAL(out.has_value(), engaged);
    if (engaged) { ASSERT_EQUAL(out->value, 42); }
  }
  {
    auto json = R"({"a":5})"_padded;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::optional<simdjson::throwing_int> out;
    ASSERT_SUCCESS(doc["a"].get(out));
    ASSERT_TRUE(out.has_value());
    ASSERT_EQUAL(out->value, 5);
  }
  TEST_SUCCEED();
}
#endif // SIMDJSON_EXCEPTIONS
#endif // SIMDJSON_SUPPORTS_CONCEPTS

#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS

bool basic_general_madness() {
  TEST_START();
  simdjson::padded_string json =
      R"( [ { "codes": [1,2,3,4,18,19] },
            { "codes": [1,3,4,111,1111] } ])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  std::vector<uint16_t> codes;
  for (auto val : doc) {
    simdjson::ondemand::object obj;
    SIMDJSON_TRY(val.get_object().get(obj));
    obj["codes"].get(codes); // append to it
  }

  if (codes.size() != 11) {
    return false;
  }
  if (codes[5] != 19 || codes[7] != 3 || codes[9] != 111) {
    return false;
  }

  TEST_SUCCEED();
}

bool basic_general_madness_vector() {
  TEST_START();
  simdjson::padded_string json =
      R"( [ { "codes": [1.2, 3.4, 5.6, 7.8, 9.0] },
            { "codes": [2.2, 4.4, 6.6, 8.8, 10.0] } ])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  std::vector<float> codes;
  for (auto val : doc) {
    simdjson::ondemand::object obj;
    SIMDJSON_TRY(val.get_object().get(obj));
    obj["codes"].get(codes); // append to it
  }

  if (codes.size() != 10) {
    return false;
  }
  if (codes[5] != 2.2f || codes[7] != 6.6f || codes[9] != 10.0f) {
    return false;
  }

  TEST_SUCCEED();
}

bool basic_general_madness_list() {
  TEST_START();
  simdjson::padded_string json =
      R"( [ { "codes": [1.2, 3.4, 5.6, 7.8, 9.0] },
            { "codes": [2.2, 4.4, 6.6, 8.8, 10.0] } ])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  std::list<float> codes;
  for (auto val : doc) {
    simdjson::ondemand::object obj;
    SIMDJSON_TRY(val.get_object().get(obj));
    obj["codes"].get(codes); // append to it
  }

  if (codes.size() != 10) {
    return false;
  }
  if (codes.back() != 10.0f || codes.front() != 1.2f) {
    return false;
  }

  TEST_SUCCEED();
}



bool basic_general_madness_Array() {
  TEST_START();
  simdjson::padded_string json =
      R"( [ { "codes": [1.2, 3.4, 5.6, 7.8, 9.0] },
            { "codes": [2.2, 4.4, 6.6, 8.8, 10.0] } ])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  Array codes;
  for (auto val : doc) {
    simdjson::ondemand::object obj;
    SIMDJSON_TRY(val.get_object().get(obj));
    obj["codes"].get(codes); // append to it
  }
  ASSERT_EQUAL(codes.size(), 20);
  TEST_SUCCEED();
}


template <template <typename...> typename Container, typename ValT = float>
bool basic_general_madness_container() {
  TYPED_TEST_START(Container<ValT>);

  simdjson::padded_string json =
      R"( [ { "codes": [1.0, 3.4, 5.6, 7.8, 9.0] },
            { "codes": [2.2, 4.4, 6.6, 8.8, 10.0] } ])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  Container<ValT> codes;
  for (auto val : doc) {
    simdjson::ondemand::object obj;
    SIMDJSON_TRY(val.get_object().get(obj));
    obj["codes"].get(codes); // append to it
  }

  if (codes.size() != 10) {
    return false;
  }


  if constexpr (requires {codes.front();}) {
    if constexpr (requires {*codes.front();}) {
      if (*codes.front() != 1.0) {
        return false;
      }
    } else {
      if (codes.front() != 1.0) {
        return false;
      }
    }
  }
  if constexpr (requires {codes.back();} ) {
    if constexpr (requires {*codes.back();}) {
      if (*codes.back() != 10.0) {
        return false;
      }
    } else {
      if (codes.back() != 10.0) {
        return false;
      }
    }
  }

  TEST_SUCCEED();
}

#endif // SIMDJSON_EXCEPTIONS

#if SIMDJSON_SUPPORTS_CONCEPTS
bool map_deserialization() {
  TEST_START();

  simdjson::padded_string json = R"({
    "settings": {
      "timeout": 30,
      "retries": 3,
      "escaped\nkey": 42
    },
    "nested": {
      "inner": {
        "alpha": 100,
        "beta": 200
      }
    }
  })"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);

  // 1. Deserializing from ondemand::value into std::map
  std::map<std::string, int> settings_map;
  ASSERT_SUCCESS(doc["settings"].get(settings_map));
  ASSERT_EQUAL(settings_map.size(), 3);
  ASSERT_EQUAL(settings_map["timeout"], 30);
  ASSERT_EQUAL(settings_map["retries"], 3);
  ASSERT_EQUAL(settings_map["escaped\nkey"], 42);

  // 2. Deserializing into std::unordered_map from ondemand::value
  std::unordered_map<std::string, int> un_map;
  simdjson::ondemand::document doc2 = parser.iterate(json);
  ASSERT_SUCCESS(doc2["settings"].get(un_map));
  ASSERT_EQUAL(un_map.size(), 3);
  ASSERT_EQUAL(un_map["timeout"], 30);
  ASSERT_EQUAL(un_map["retries"], 3);
  ASSERT_EQUAL(un_map["escaped\nkey"], 42);

  // 3. Deserializing from document root into std::map (overwrites pre-existing content via clear)
  simdjson::padded_string root_map_json = R"({"a": 1, "b": 2, "c": 3})"_padded;
  simdjson::ondemand::document doc3 = parser.iterate(root_map_json);
  std::map<std::string, int> root_map;
  root_map["existing"] = 999;
  ASSERT_SUCCESS(doc3.get(root_map));
  ASSERT_EQUAL(root_map.size(), 3);
  ASSERT_TRUE(root_map.find("existing") == root_map.end());
  ASSERT_EQUAL(root_map["a"], 1);
  ASSERT_EQUAL(root_map["b"], 2);
  ASSERT_EQUAL(root_map["c"], 3);

  // 4. Deserializing from ondemand::object directly
  simdjson::ondemand::document doc4 = parser.iterate(root_map_json);
  simdjson::ondemand::object root_obj;
  ASSERT_SUCCESS(doc4.get_object().get(root_obj));
  std::map<std::string, int> obj_map;
  ASSERT_SUCCESS(simdjson::deserialize(root_obj, obj_map));
  ASSERT_EQUAL(obj_map.size(), 3);
  ASSERT_EQUAL(obj_map["a"], 1);

  // 5. Deserializing into map when value is not an object returns INCORRECT_TYPE
  simdjson::padded_string not_object_json = R"([1, 2, 3])"_padded;
  simdjson::ondemand::document doc5 = parser.iterate(not_object_json);
  std::map<std::string, int> preserved_map;
  preserved_map["kept"] = 123;
  ASSERT_ERROR(doc5.get(preserved_map), simdjson::INCORRECT_TYPE);
  ASSERT_EQUAL(preserved_map.size(), 1);
  ASSERT_EQUAL(preserved_map["kept"], 123);

  // 6. Deserializing when a field value fails type conversion
  simdjson::padded_string bad_field_json = R"({"a": 1, "b": "not_an_int"})"_padded;
  simdjson::ondemand::document doc6 = parser.iterate(bad_field_json);
  std::map<std::string, int> err_map;
  ASSERT_ERROR(doc6.get(err_map), simdjson::INCORRECT_TYPE);

  // 7. Non-trivial mapped type (nested container: std::map<std::string, std::vector<int>>)
  simdjson::padded_string nested_json = R"({"odds": [1, 3, 5], "evens": [2, 4, 6]})"_padded;
  simdjson::ondemand::document doc7 = parser.iterate(nested_json);
  std::map<std::string, std::vector<int>> vec_map;
  ASSERT_SUCCESS(doc7.get(vec_map));
  ASSERT_EQUAL(vec_map.size(), 2);
  ASSERT_EQUAL(vec_map["odds"].size(), 3);
  ASSERT_EQUAL(vec_map["odds"][0], 1);
  ASSERT_EQUAL(vec_map["evens"].size(), 3);
  ASSERT_EQUAL(vec_map["evens"][2], 6);

  TEST_SUCCEED();
}
#endif // SIMDJSON_SUPPORTS_CONCEPTS

bool run() {
  return
#if SIMDJSON_SUPPORTS_CONCEPTS
      smart_pointers_to_builtin_types() &&
      map_deserialization() &&
      optionals_to_builtin_types() &&
      optional_containers() &&
#if SIMDJSON_EXCEPTIONS
      optional_exception_untouched() &&
#endif // SIMDJSON_EXCEPTIONS
#endif // SIMDJSON_SUPPORTS_CONCEPTS
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
      basic_general_madness() &&
      basic_general_madness_vector() &&
      basic_general_madness_list() &&
      basic_general_madness_Array() &&
      basic_general_madness_container<std::vector>() &&
      basic_general_madness_container<std::list>() &&
      basic_general_madness_container<std::set>() &&
      basic_general_madness_container<std::stack>() &&
      basic_general_madness_container<std::queue>() &&
      basic_general_madness_container<std::deque>() &&
      basic_general_madness_container<std::priority_queue>() &&
      basic_general_madness_container<std::unordered_set>() &&
      basic_general_madness_container<std::multiset>() &&
      basic_general_madness_container<std::unordered_multiset>() &&
        // double:
      basic_general_madness_container<std::vector, double>() &&
      basic_general_madness_container<std::list, double>() &&
      basic_general_madness_container<std::set, double>() &&
      basic_general_madness_container<std::stack, double>() &&
      basic_general_madness_container<std::queue, double>() &&
      basic_general_madness_container<std::deque, double>() &&
      basic_general_madness_container<std::priority_queue, double>() &&
      basic_general_madness_container<std::unordered_set, double>() &&
      basic_general_madness_container<std::multiset, double>() &&
      basic_general_madness_container<std::unordered_multiset, double>() &&
        // unique_ptr<double>:
      basic_general_madness_container<std::vector, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::list, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::set, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::stack, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::deque, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::queue, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::priority_queue, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::unordered_set, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::multiset, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::unordered_multiset, std::unique_ptr<double>>() &&
        // shared_ptr<double>:
      basic_general_madness_container<std::vector, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::list, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::set, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::stack, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::deque, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::queue, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::priority_queue, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::unordered_set, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::multiset, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::unordered_multiset, std::shared_ptr<double>>() &&
        // optional<double>:
      basic_general_madness_container<std::vector, std::optional<double>>() &&
      basic_general_madness_container<std::list, std::optional<double>>() &&
      basic_general_madness_container<std::set, std::optional<double>>() &&
      basic_general_madness_container<std::stack, std::optional<double>>() &&
      basic_general_madness_container<std::deque, std::optional<double>>() &&
      basic_general_madness_container<std::queue, std::optional<double>>() &&
      basic_general_madness_container<std::priority_queue, std::optional<double>>() &&
      basic_general_madness_container<std::unordered_set, std::optional<double>>() &&
      basic_general_madness_container<std::multiset, std::optional<double>>() &&
      basic_general_madness_container<std::unordered_multiset, std::optional<double>>() &&
#endif // SIMDJSON_EXCEPTIONS
      true;
}

} // namespace stl_types

int main(int argc, char *argv[]) {
  return test_main(argc, argv, stl_types::run);
}
