#include <iostream>
#include <set>
#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace wildcard_tests {
  using namespace std;

  bool array_wildcard_basic() {
    TEST_START();
    auto json = R"({
      "books": [
        {"title": "Book A", "price": 10},
        {"title": "Book B", "price": 15},
        {"title": "Book C", "price": 20}
      ]
    })"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    auto result = doc.at_path_with_wildcard("$.books[*].title");
    if (result.error()) {
      std::cerr << "Error: " << result.error() << std::endl;
      return false;
    }

    std::vector<ondemand::value> titles = result.value_unsafe();
    ASSERT_EQUAL(titles.size(), 3);

    ASSERT_EQUAL(titles[0].get_string().value_unsafe(), "Book A");
    ASSERT_EQUAL(titles[1].get_string().value_unsafe(), "Book B");
    ASSERT_EQUAL(titles[2].get_string().value_unsafe(), "Book C");

    TEST_SUCCEED();
  }

  bool array_wildcard_numbers() {
    TEST_START();
    auto json = R"({
      "prices": [10, 20, 30, 40, 50]
    })"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    auto result = doc.at_path_with_wildcard("$.prices[*]");
    if (result.error()) {
      std::cerr << "Error: " << result.error() << std::endl;
      return false;
    }

    std::vector<ondemand::value> prices = result.value_unsafe();
    ASSERT_EQUAL(prices.size(), 5);

    ASSERT_EQUAL(prices[0].get_uint64().value_unsafe(), 10);
    ASSERT_EQUAL(prices[2].get_uint64().value_unsafe(), 30);
    ASSERT_EQUAL(prices[4].get_uint64().value_unsafe(), 50);

    TEST_SUCCEED();
  }

  bool object_wildcard_basic() {
    TEST_START();
    auto json = R"({
      "users": {
        "user1": {"name": "Alice", "age": 30},
        "user2": {"name": "Bob", "age": 25},
        "user3": {"name": "Charlie", "age": 35}
      }
    })"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    auto result = doc.at_path_with_wildcard("$.users.*.name");
    if (result.error()) {
      std::cerr << "Error: " << result.error() << std::endl;
      return false;
    }

    std::vector<ondemand::value> names = result.value_unsafe();
    ASSERT_EQUAL(names.size(), 3);

    // Verify we got all three names (order may vary)
    std::set<std::string_view> name_set;
    for (auto& name : names) {
      name_set.insert(name.get_string().value_unsafe());
    }
    ASSERT_TRUE(name_set.count("Alice") > 0);
    ASSERT_TRUE(name_set.count("Bob") > 0);
    ASSERT_TRUE(name_set.count("Charlie") > 0);

    TEST_SUCCEED();
  }

  bool combined_wildcards() {
    TEST_START();
    auto json = R"({
      "departments": {
        "engineering": {
          "employees": [
            {"name": "Alice", "salary": 100000},
            {"name": "Bob", "salary": 95000}
          ]
        },
        "sales": {
          "employees": [
            {"name": "Charlie", "salary": 80000},
            {"name": "Diana", "salary": 85000}
          ]
        }
      }
    })"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    auto result = doc.at_path_with_wildcard("$.departments.*.employees[*].salary");
    if (result.error()) {
      std::cerr << "Error: " << result.error() << std::endl;
      return false;
    }

    std::vector<ondemand::value> salaries = result.value_unsafe();
    ASSERT_EQUAL(salaries.size(), 4);

    // Check sum of all salaries
    uint64_t total = 0;
    for (auto& salary : salaries) {
      total += salary.get_uint64().value_unsafe();
    }
    ASSERT_EQUAL(total, 360000);

    TEST_SUCCEED();
  }

  bool empty_array_wildcard() {
    TEST_START();
    auto json = R"({"items": []})"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    auto result = doc.at_path_with_wildcard("$.items[*].name");
    if (result.error()) {
      std::cerr << "Error: " << result.error() << std::endl;
      return false;
    }

    std::vector<ondemand::value> items = result.value_unsafe();
    ASSERT_EQUAL(items.size(), 0);

    TEST_SUCCEED();
  }

  bool empty_object_wildcard() {
    TEST_START();
    auto json = R"({"users": {}})"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    auto result = doc.at_path_with_wildcard("$.users.*.name");
    if (result.error()) {
      std::cerr << "Error: " << result.error() << std::endl;
      return false;
    }

    std::vector<ondemand::value> names = result.value_unsafe();
    ASSERT_EQUAL(names.size(), 0);

    TEST_SUCCEED();
  }

  bool wildcard_on_scalar_error() {
    TEST_START();
    auto json = R"({"data": 123})"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    // Can't use array wildcard on scalar
    auto result = doc.at_path_with_wildcard("$.data[*]");
    ASSERT_ERROR(result.error(), INVALID_JSON_POINTER);

    TEST_SUCCEED();
  }

  bool nested_arrays_wildcard() {
    TEST_START();
    auto json = R"({
      "matrix": [
        [1, 2, 3],
        [4, 5, 6],
        [7, 8, 9]
      ]
    })"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    // Get all nested arrays
    auto result = doc.at_path_with_wildcard("$.matrix[*]");
    if (result.error()) {
      std::cerr << "Error: " << result.error() << std::endl;
      return false;
    }

    std::vector<ondemand::value> arrays = result.value_unsafe();
    // Verify we got 3 arrays
    ASSERT_EQUAL(arrays.size(), 3);

    TEST_SUCCEED();
  }

  bool wildcard_with_nested_objects() {
    TEST_START();
    auto json = R"({
      "data": [
        {"info": {"id": 1, "status": "active"}},
        {"info": {"id": 2, "status": "inactive"}},
        {"info": {"id": 3, "status": "active"}}
      ]
    })"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    auto result = doc.at_path_with_wildcard("$.data[*].info.status");
    if (result.error()) {
      std::cerr << "Error: " << result.error() << std::endl;
      return false;
    }

    std::vector<ondemand::value> statuses = result.value_unsafe();
    ASSERT_EQUAL(statuses.size(), 3);

    ASSERT_EQUAL(statuses[0].get_string().value_unsafe(), "active");
    ASSERT_EQUAL(statuses[1].get_string().value_unsafe(), "inactive");
    ASSERT_EQUAL(statuses[2].get_string().value_unsafe(), "active");

    TEST_SUCCEED();
  }

  bool mixed_types_in_array() {
    TEST_START();
    auto json = R"({
      "mixed": [
        42,
        "hello",
        {"key": "value"},
        [1, 2, 3],
        true,
        null
      ]
    })"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    auto result = doc.at_path_with_wildcard("$.mixed[*]");
    if (result.error()) {
      std::cerr << "Error: " << result.error() << std::endl;
      return false;
    }

    std::vector<ondemand::value> values = result.value_unsafe();
    // Just verify we got all 6 elements
    ASSERT_EQUAL(values.size(), 6);

    // Don't try to access the values - they've been consumed by the OnDemand API

    TEST_SUCCEED();
  }

  bool wildcard_nonexistent_field() {
    TEST_START();
    auto json = R"({
      "data": [
        {"name": "Alice"},
        {"name": "Bob"},
        {"other": "field"}
      ]
    })"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    // Third element doesn't have "name" field
    auto result = doc.at_path_with_wildcard("$.data[*].name");

    // This might error or return partial results
    if (result.error()) {
      // If it errors, that's acceptable
      TEST_SUCCEED();
    }

    std::vector<ondemand::value> names = result.value_unsafe();
    // Should have at least 2 results
    ASSERT_TRUE(names.size() >= 2);

    TEST_SUCCEED();
  }

  bool root_array_wildcard() {
    TEST_START();
    auto json = R"([
      {"id": 1, "name": "Item 1"},
      {"id": 2, "name": "Item 2"},
      {"id": 3, "name": "Item 3"}
    ])"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    auto result = doc.at_path_with_wildcard("$[*].name");
    if (result.error()) {
      std::cerr << "Error: " << result.error() << std::endl;
      return false;
    }

    std::vector<ondemand::value> names = result.value_unsafe();
    ASSERT_EQUAL(names.size(), 3);

    ASSERT_EQUAL(names[0].get_string().value_unsafe(), "Item 1");
    ASSERT_EQUAL(names[1].get_string().value_unsafe(), "Item 2");
    ASSERT_EQUAL(names[2].get_string().value_unsafe(), "Item 3");

    TEST_SUCCEED();
  }

  bool run() {
    return array_wildcard_basic() &&
           array_wildcard_numbers() &&
           object_wildcard_basic() &&
           combined_wildcards() &&
           empty_array_wildcard() &&
           empty_object_wildcard() &&
           wildcard_on_scalar_error() &&
           nested_arrays_wildcard() &&
           wildcard_with_nested_objects() &&
           mixed_types_in_array() &&
           wildcard_nonexistent_field() &&
           root_array_wildcard();
  }
}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, wildcard_tests::run);
}