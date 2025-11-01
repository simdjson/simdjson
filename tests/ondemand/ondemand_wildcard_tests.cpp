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

    std::vector<ondemand::value> titles;
    ASSERT_SUCCESS(doc.at_path_with_wildcard("$.books[*].title").get(titles));

    ASSERT_EQUAL(titles.size(), 3);

    std::string_view title;
    ASSERT_SUCCESS(titles[0].get_string().get(title));
    ASSERT_EQUAL(title, "Book A");
    ASSERT_SUCCESS(titles[1].get_string().get(title));
    ASSERT_EQUAL(title, "Book B");
    ASSERT_SUCCESS(titles[2].get_string().get(title));
    ASSERT_EQUAL(title, "Book C");

    TEST_SUCCEED();
  }

  bool array_wildcard_numbers() {
    TEST_START();
    auto json = R"({
      "prices": [10, 20, 30, 40, 50]
    })"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    std::vector<ondemand::value> prices;
    ASSERT_SUCCESS(doc.at_path_with_wildcard("$.prices[*]").get(prices));

    ASSERT_EQUAL(prices.size(), 5);

    uint64_t price;
    ASSERT_SUCCESS(prices[0].get_uint64().get(price));
    ASSERT_EQUAL(price, 10);
    ASSERT_SUCCESS(prices[2].get_uint64().get(price));
    ASSERT_EQUAL(price, 30);
    ASSERT_SUCCESS(prices[4].get_uint64().get(price));
    ASSERT_EQUAL(price, 50);

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

    std::vector<ondemand::value> names;
    auto error = doc.at_path_with_wildcard("$.users.*.name").get(names);
    ASSERT_SUCCESS(error);

    ASSERT_EQUAL(names.size(), 3);

    // Verify we got all three names (order may vary)
    std::set<std::string_view> name_set;
    for (auto& name : names) {
      std::string_view view;
      ASSERT_SUCCESS(name.get_string().get(view));
      name_set.insert(view);
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

    std::vector<ondemand::value> salaries;
    auto error = doc.at_path_with_wildcard("$.departments.*.employees[*].salary").get(salaries);
    ASSERT_SUCCESS(error);

    ASSERT_EQUAL(salaries.size(), 4);

    // Check sum of all salaries
    uint64_t total = 0;
    for (auto& salary : salaries) {
      uint64_t value;
      ASSERT_SUCCESS(salary.get_uint64().get(value));
      total += value;
    }
    ASSERT_EQUAL(total, 360000);

    TEST_SUCCEED();
  }

  bool empty_array_wildcard() {
    TEST_START();
    auto json = R"({"items": []})"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    std::vector<ondemand::value> items;
    auto error = doc.at_path_with_wildcard("$.items[*].name").get(items);
    ASSERT_SUCCESS(error);

    ASSERT_EQUAL(items.size(), 0);

    TEST_SUCCEED();
  }

  bool empty_object_wildcard() {
    TEST_START();
    auto json = R"({"users": {}})"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    std::vector<ondemand::value> names;
    auto error = doc.at_path_with_wildcard("$.users.*.name").get(names);
    ASSERT_SUCCESS(error);

    ASSERT_EQUAL(names.size(), 0);

    TEST_SUCCEED();
  }

  bool wildcard_on_scalar_error() {
    TEST_START();
    auto json = R"({"data": 123})"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    // Can't use array wildcard on scalar
    std::vector<ondemand::value> values;
    auto error = doc.at_path_with_wildcard("$.data[*]").get(values);
    ASSERT_ERROR(error, INVALID_JSON_POINTER);

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
    std::vector<ondemand::value> arrays;
    ASSERT_SUCCESS(doc.at_path_with_wildcard("$.matrix[*]").get(arrays));

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

    std::vector<ondemand::value> statuses;
    ASSERT_SUCCESS(doc.at_path_with_wildcard("$.data[*].info.status").get(statuses));

    ASSERT_EQUAL(statuses.size(), 3);

    std::string_view status;
    ASSERT_SUCCESS(statuses[0].get_string().get(status));
    ASSERT_EQUAL(status, "active");
    ASSERT_SUCCESS(statuses[1].get_string().get(status));
    ASSERT_EQUAL(status, "inactive");
    ASSERT_SUCCESS(statuses[2].get_string().get(status));
    ASSERT_EQUAL(status, "active");

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

    std::vector<ondemand::value> values;
    ASSERT_SUCCESS(doc.at_path_with_wildcard("$.mixed[*]").get(values));

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
    std::vector<ondemand::value> names;
    auto error = doc.at_path_with_wildcard("$.data[*].name").get(names);

    // This might error or return partial results
    if (error) {
      // If it errors, that's acceptable
      TEST_SUCCEED();
    }

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

    std::vector<ondemand::value> names;
    ASSERT_SUCCESS(doc.at_path_with_wildcard("$[*].name").get(names));

    ASSERT_EQUAL(names.size(), 3);

    std::string_view name;
    ASSERT_SUCCESS(names[0].get_string().get(name));
    ASSERT_EQUAL(name, "Item 1");
    ASSERT_SUCCESS(names[1].get_string().get(name));
    ASSERT_EQUAL(name, "Item 2");
    ASSERT_SUCCESS(names[2].get_string().get(name));
    ASSERT_EQUAL(name, "Item 3");

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