#include <iostream>
#include "simdjson.h"
#include "test_macros.h"
#include "test_main.h"

#if SIMDJSON_SUPPORTS_RANGES

#include <algorithm>
#include <ranges>
#include <string>
#include <vector>

using namespace simdjson;

namespace ondemand_ranges_tests {

#if SIMDJSON_EXCEPTIONS

bool array_get_range_basic() {
  TEST_START();
  auto json = R"([10, 20, 30])"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  auto arr = doc.get_array();
  auto range = ondemand::get_range(arr);

  std::vector<int64_t> values;
  for (auto elem : range) {
    values.push_back(int64_t(elem));
  }
  ASSERT_EQUAL(values.size(), size_t(3));
  ASSERT_EQUAL(values[0], int64_t(10));
  ASSERT_EQUAL(values[1], int64_t(20));
  ASSERT_EQUAL(values[2], int64_t(30));
  TEST_SUCCEED();
}

bool array_range_with_transform() {
  TEST_START();
  auto json = R"([1, 2, 3, 4, 5])"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  auto arr = doc.get_array();

  auto doubled = ondemand::get_range(arr)
    | std::views::transform([](auto v) -> int64_t { return int64_t(v) * 2; });

  std::vector<int64_t> values;
  for (auto val : doubled) {
    values.push_back(val);
  }
  ASSERT_EQUAL(values.size(), size_t(5));
  ASSERT_EQUAL(values[0], int64_t(2));
  ASSERT_EQUAL(values[1], int64_t(4));
  ASSERT_EQUAL(values[2], int64_t(6));
  ASSERT_EQUAL(values[3], int64_t(8));
  ASSERT_EQUAL(values[4], int64_t(10));
  TEST_SUCCEED();
}

bool array_range_strings() {
  TEST_START();
  auto json = R"(["alpha", "beta", "gamma"])"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  auto arr = doc.get_array();

  auto to_string = [](auto v) -> std::string {
    return std::string(std::string_view(v));
  };
  auto strings = ondemand::get_range(arr) | std::views::transform(to_string);

  std::vector<std::string> values;
  for (auto s : strings) {
    values.push_back(s);
  }
  ASSERT_EQUAL(values.size(), size_t(3));
  ASSERT_TRUE(values[0] == "alpha");
  ASSERT_TRUE(values[1] == "beta");
  ASSERT_TRUE(values[2] == "gamma");
  TEST_SUCCEED();
}

bool array_range_empty() {
  TEST_START();
  auto json = R"([])"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  auto arr = doc.get_array();
  auto range = ondemand::get_range(arr);

  int count = 0;
  for (simdjson_unused auto elem : range) {
    count++;
  }
  ASSERT_EQUAL(count, 0);
  TEST_SUCCEED();
}

bool array_range_nested() {
  TEST_START();
  auto json = R"([{"name": "Alice", "age": 30}, {"name": "Bob", "age": 25}])"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  auto arr = doc.get_array();

  auto get_name = [](auto v) -> std::string {
    return std::string(std::string_view(v["name"]));
  };
  auto names = ondemand::get_range(arr) | std::views::transform(get_name);

  std::vector<std::string> values;
  for (auto name : names) {
    values.push_back(name);
  }
  ASSERT_EQUAL(values.size(), size_t(2));
  ASSERT_TRUE(values[0] == "Alice");
  ASSERT_TRUE(values[1] == "Bob");
  TEST_SUCCEED();
}

bool object_get_range_basic() {
  TEST_START();
  auto json = R"({"a": 1, "b": 2, "c": 3})"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  auto obj = doc.get_object();
  auto range = ondemand::get_key_value_range(obj);

  std::vector<std::string> keys;
  std::vector<int64_t> vals;
  for (auto field_result : range) {
    keys.push_back(std::string(std::string_view(field_result.escaped_key())));
    vals.push_back(int64_t(field_result.value()));
  }
  ASSERT_EQUAL(keys.size(), size_t(3));
  ASSERT_TRUE(keys[0] == "a");
  ASSERT_TRUE(keys[1] == "b");
  ASSERT_TRUE(keys[2] == "c");
  ASSERT_EQUAL(vals[0], int64_t(1));
  ASSERT_EQUAL(vals[1], int64_t(2));
  ASSERT_EQUAL(vals[2], int64_t(3));
  TEST_SUCCEED();
}

bool object_range_with_transform() {
  TEST_START();
  auto json = R"({"x": 10, "y": 20, "z": 30})"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  auto obj = doc.get_object();

  auto get_key = [](auto field_result) -> std::string {
    return std::string(std::string_view(field_result.escaped_key()));
  };
  auto keys = ondemand::get_key_value_range(obj) | std::views::transform(get_key);

  std::vector<std::string> values;
  for (auto k : keys) {
    values.push_back(k);
  }
  ASSERT_EQUAL(values.size(), size_t(3));
  ASSERT_TRUE(values[0] == "x");
  ASSERT_TRUE(values[1] == "y");
  ASSERT_TRUE(values[2] == "z");
  TEST_SUCCEED();
}

bool object_range_empty() {
  TEST_START();
  auto json = R"({})"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  auto obj = doc.get_object();
  auto range = ondemand::get_key_value_range(obj);

  int count = 0;
  for (simdjson_unused auto elem : range) {
    count++;
  }
  ASSERT_EQUAL(count, 0);
  TEST_SUCCEED();
}

bool get_range_from_result() {
  TEST_START();
  auto json = R"([100, 200, 300])"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);

  // get_range with simdjson_result<array> - unwraps automatically
  auto range = ondemand::get_range(doc.get_array());

  std::vector<int64_t> values;
  for (auto elem : range) {
    values.push_back(int64_t(elem));
  }
  ASSERT_EQUAL(values.size(), size_t(3));
  ASSERT_EQUAL(values[0], int64_t(100));
  ASSERT_EQUAL(values[1], int64_t(200));
  ASSERT_EQUAL(values[2], int64_t(300));
  TEST_SUCCEED();
}

bool object_range_key_iteration() {
  TEST_START();
  auto json = R"({"name": "Alice", "age": 30, "city": "New York"})"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  auto obj = doc.get_object();

  // Test the specific pattern: iterating over field_result.key()
  std::vector<std::string> keys;
  for (auto field_result : ondemand::get_key_value_range(obj)) {
    keys.push_back(std::string(field_result.escaped_key().value()));
  }

  ASSERT_EQUAL(keys.size(), size_t(3));
  bool has_name = false, has_age = false, has_city = false;
  for (const auto& key : keys) {
    if (key == "name") has_name = true;
    else if (key == "age") has_age = true;
    else if (key == "city") has_city = true;
  }
  ASSERT_TRUE(has_name);
  ASSERT_TRUE(has_age);
  ASSERT_TRUE(has_city);
  TEST_SUCCEED();
}

// Verify that the types satisfy the expected C++20 concepts.
bool concept_checks() {
  TEST_START();
  static_assert(std::input_iterator<ondemand::array_range_iterator>);
  static_assert(std::input_iterator<ondemand::object_range_iterator>);
  static_assert(std::ranges::input_range<ondemand::array_range>);
  static_assert(std::ranges::input_range<ondemand::object_range>);
  static_assert(std::ranges::view<ondemand::array_range>);
  static_assert(std::ranges::view<ondemand::object_range>);
  TEST_SUCCEED();
}

#endif // SIMDJSON_EXCEPTIONS

// These tests work without exceptions.
bool array_range_noexcept_basic() {
  TEST_START();
  auto json = R"([1, 2, 3])"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  ondemand::array arr;
  ASSERT_SUCCESS(doc.get_array().get(arr));
  auto range = ondemand::get_range(arr);
  ASSERT_SUCCESS(range.error());

  int count = 0;
  for (auto elem : range) {
    int64_t val;
    ASSERT_SUCCESS(elem.get_int64().get(val));
    count++;
  }
  ASSERT_EQUAL(count, 3);
  TEST_SUCCEED();
}

bool object_range_noexcept_basic() {
  TEST_START();
  auto json = R"({"a": 1, "b": 2})"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  ondemand::object obj;
  ASSERT_SUCCESS(doc.get_object().get(obj));
  auto range = ondemand::get_key_value_range(obj);
  ASSERT_SUCCESS(range.error());

  int count = 0;
  for (auto field_result : range) {
    simdjson_unused ondemand::field f;
    ASSERT_SUCCESS(std::move(field_result).get(f));
    count++;
  }
  ASSERT_EQUAL(count, 2);
  TEST_SUCCEED();
}

bool run() {
  return
    array_range_noexcept_basic() &&
    object_range_noexcept_basic() &&
#if SIMDJSON_EXCEPTIONS
    concept_checks() &&
    array_get_range_basic() &&
    array_range_with_transform() &&
    array_range_strings() &&
    array_range_empty() &&
    array_range_nested() &&
    object_get_range_basic() &&
    object_range_with_transform() &&
    object_range_empty() &&
    get_range_from_result() &&
    object_range_key_iteration() &&
#endif // SIMDJSON_EXCEPTIONS
    true;
}

} // namespace ondemand_ranges_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, ondemand_ranges_tests::run);
}

#else // !SIMDJSON_SUPPORTS_RANGES

int main() {
  std::cout << "Ranges tests require C++20 ranges support, skipping." << std::endl;
  return 0;
}

#endif // SIMDJSON_SUPPORTS_RANGES
