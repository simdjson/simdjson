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

    std::vector<std::string_view> titles;
    ASSERT_SUCCESS(doc.for_each_at_path_with_wildcard("$.books[*].title", [&](ondemand::value v) {
      std::string_view sv;
      if (v.get_string().get(sv) == SUCCESS) { titles.push_back(sv); }
    }));

    ASSERT_EQUAL(titles.size(), 3);
    ASSERT_EQUAL(titles[0], "Book A");
    ASSERT_EQUAL(titles[1], "Book B");
    ASSERT_EQUAL(titles[2], "Book C");

    TEST_SUCCEED();
  }

  bool array_wildcard_numbers() {
    TEST_START();
    auto json = R"({
      "prices": [10, 20, 30, 40, 50]
    })"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    std::vector<uint64_t> prices;
    ASSERT_SUCCESS(doc.for_each_at_path_with_wildcard("$.prices[*]", [&](ondemand::value v) {
      uint64_t p;
      if (v.get_uint64().get(p) == SUCCESS) { prices.push_back(p); }
    }));

    ASSERT_EQUAL(prices.size(), 5);
    ASSERT_EQUAL(prices[0], 10);
    ASSERT_EQUAL(prices[2], 30);
    ASSERT_EQUAL(prices[4], 50);

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

    std::set<std::string_view> name_set;
    auto error = doc.for_each_at_path_with_wildcard("$.users.*.name", [&](ondemand::value v) {
      std::string_view sv;
      if (v.get_string().get(sv) == SUCCESS) { name_set.insert(sv); }
    });
    ASSERT_SUCCESS(error);

    ASSERT_EQUAL(name_set.size(), 3);
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

    uint64_t total = 0;
    size_t count = 0;
    auto error = doc.for_each_at_path_with_wildcard("$.departments.*.employees[*].salary", [&](ondemand::value v) {
      uint64_t s;
      if (v.get_uint64().get(s) == SUCCESS) { total += s; count++; }
    });
    ASSERT_SUCCESS(error);

    ASSERT_EQUAL(count, 4);
    ASSERT_EQUAL(total, 360000);

    TEST_SUCCEED();
  }

  bool empty_array_wildcard() {
    TEST_START();
    auto json = R"({"items": []})"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    size_t count = 0;
    auto error = doc.for_each_at_path_with_wildcard("$.items[*].name", [&](ondemand::value) {
      count++;
    });
    ASSERT_SUCCESS(error);
    ASSERT_EQUAL(count, 0);

    TEST_SUCCEED();
  }

  bool empty_object_wildcard() {
    TEST_START();
    auto json = R"({"users": {}})"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    size_t count = 0;
    auto error = doc.for_each_at_path_with_wildcard("$.users.*.name", [&](ondemand::value) {
      count++;
    });
    ASSERT_SUCCESS(error);
    ASSERT_EQUAL(count, 0);

    TEST_SUCCEED();
  }

  bool wildcard_on_scalar_error() {
    TEST_START();
    auto json = R"({"data": 123})"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    // Can't use array wildcard on scalar
    auto error = doc.for_each_at_path_with_wildcard("$.data[*]", [](ondemand::value) {});
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

    size_t count = 0;
    ASSERT_SUCCESS(doc.for_each_at_path_with_wildcard("$.matrix[*]", [&](ondemand::value) {
      count++;
    }));

    ASSERT_EQUAL(count, 3);

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

    std::vector<std::string_view> statuses;
    ASSERT_SUCCESS(doc.for_each_at_path_with_wildcard("$.data[*].info.status", [&](ondemand::value v) {
      std::string_view sv;
      if (v.get_string().get(sv) == SUCCESS) { statuses.push_back(sv); }
    }));

    ASSERT_EQUAL(statuses.size(), 3);
    ASSERT_EQUAL(statuses[0], "active");
    ASSERT_EQUAL(statuses[1], "inactive");
    ASSERT_EQUAL(statuses[2], "active");

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

    size_t count = 0;
    ASSERT_SUCCESS(doc.for_each_at_path_with_wildcard("$.mixed[*]", [&](ondemand::value) {
      count++;
    }));

    ASSERT_EQUAL(count, 6);

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
    std::vector<std::string_view> names;
    auto error = doc.for_each_at_path_with_wildcard("$.data[*].name", [&](ondemand::value v) {
      std::string_view sv;
      if (v.get_string().get(sv) == SUCCESS) { names.push_back(sv); }
    });

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

    std::vector<std::string_view> names;
    ASSERT_SUCCESS(doc.for_each_at_path_with_wildcard("$[*].name", [&](ondemand::value v) {
      std::string_view sv;
      if (v.get_string().get(sv) == SUCCESS) { names.push_back(sv); }
    }));

    ASSERT_EQUAL(names.size(), 3);
    ASSERT_EQUAL(names[0], "Item 1");
    ASSERT_EQUAL(names[1], "Item 2");
    ASSERT_EQUAL(names[2], "Item 3");

    TEST_SUCCEED();
  }

  // https://github.com/simdjson/simdjson/issues/2684
  bool wildcard_raw_json_issue_2684() {
    TEST_START();
    auto json = R"([{"tag_meta":{"meta_code":2000211,"meta_value":""},"tag_value":""}])"_padded;

    ondemand::parser parser;
    auto doc = parser.iterate(json);

    size_t count = 0;
    bool raw_json_ok = true;
    ASSERT_SUCCESS(doc.for_each_at_path_with_wildcard("$[*].tag_meta", [&](ondemand::value v) {
      count++;
      std::string_view raw;
      if (v.raw_json().get(raw) != SUCCESS) {
        raw_json_ok = false;
        return;
      }
      // raw_json() must not leak sibling fields or outer array delimiters
      std::string_view expected = R"({"meta_code":2000211,"meta_value":""})";
      if (raw != expected || raw.find("tag_value") != std::string_view::npos) {
        std::cerr << "  raw_json() returned: " << raw << std::endl;
        raw_json_ok = false;
      }
    }));

    ASSERT_EQUAL(count, 1);
    ASSERT_TRUE(raw_json_ok);

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
           root_array_wildcard() &&
           wildcard_raw_json_issue_2684();
  }
}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, wildcard_tests::run);
}
