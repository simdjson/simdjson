#include "simdjson.h"
#include "test_ondemand.h"
#include <string>

#if SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_STATIC_REFLECTION

using namespace simdjson;

namespace compile_time_json_pointer_tests {

// Test structures
struct User {
  std::string name;
  int age;
  std::string email;
};

struct Car {
  std::string make;
  std::string model;
  int64_t year;
  std::vector<double> tire_pressure;
};

const padded_string TEST_USER_JSON = R"(
{
  "name": "John Doe",
  "age": 30,
  "email": "john@example.com"
}
)"_padded;

const padded_string TEST_CAR_JSON = R"(
{
  "make": "Toyota",
  "model": "Camry",
  "year": 2018,
  "tire_pressure": [40.1, 39.9, 37.7, 40.4]
}
)"_padded;

const padded_string TEST_NESTED_JSON = R"(
{
  "users": [
    {
      "name": "Alice",
      "age": 25,
      "email": "alice@example.com"
    },
    {
      "name": "Bob",
      "age": 35,
      "email": "bob@example.com"
    }
  ],
  "metadata": {
    "count": 2,
    "version": "1.0"
  }
}
)"_padded;

const padded_string TEST_ARRAY_JSON = R"(
[
  {"make": "Toyota", "model": "Camry", "year": 2018, "tire_pressure": [40.1, 39.9, 37.7, 40.4]},
  {"make": "Kia", "model": "Soul", "year": 2012, "tire_pressure": [30.1, 31.0, 28.6, 28.7]},
  {"make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [29.8, 30.0, 30.2, 30.5]}
]
)"_padded;

// Test 1: Simple field access
bool test_simple_field() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_USER_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"/name">(doc);
  ASSERT_SUCCESS(result.error());

  std::string_view name;
  ASSERT_SUCCESS(result.get_string().get(name));
  ASSERT_EQUAL(name, "John Doe");

  TEST_SUCCEED();
}

// Test 2: Integer field access
bool test_integer_field() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_USER_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"/age">(doc);
  ASSERT_SUCCESS(result.error());

  int64_t age;
  ASSERT_SUCCESS(result.get_int64().get(age));
  ASSERT_EQUAL(age, 30);

  TEST_SUCCEED();
}

// Test 3: Array index access
bool test_array_index() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_CAR_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"/tire_pressure/1">(doc);
  ASSERT_SUCCESS(result.error());

  double pressure;
  ASSERT_SUCCESS(result.get_double().get(pressure));
  ASSERT_EQUAL(pressure, 39.9);

  TEST_SUCCEED();
}

// Test 4: Nested field access
bool test_nested_field() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_NESTED_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"/metadata/version">(doc);
  ASSERT_SUCCESS(result.error());

  std::string_view version;
  ASSERT_SUCCESS(result.get_string().get(version));
  ASSERT_EQUAL(version, "1.0");

  TEST_SUCCEED();
}

// Test 5: Array of objects with nested path
bool test_array_object_nested() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_NESTED_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"/users/0/name">(doc);
  ASSERT_SUCCESS(result.error());

  std::string_view name;
  ASSERT_SUCCESS(result.get_string().get(name));
  ASSERT_EQUAL(name, "Alice");

  TEST_SUCCEED();
}

// Test 6: Root array access
bool test_root_array() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_ARRAY_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"/1/make">(doc);
  ASSERT_SUCCESS(result.error());

  std::string_view make;
  ASSERT_SUCCESS(result.get_string().get(make));
  ASSERT_EQUAL(make, "Kia");

  TEST_SUCCEED();
}

// Test 7: Deep nested array
bool test_deep_nested_array() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_ARRAY_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"/0/tire_pressure/2">(doc);
  ASSERT_SUCCESS(result.error());

  double pressure;
  ASSERT_SUCCESS(result.get_double().get(pressure));
  ASSERT_EQUAL(pressure, 37.7);

  TEST_SUCCEED();
}

// Test 8: Root pointer (empty or "/")
bool test_root_pointer() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_USER_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"">(doc);
  ASSERT_SUCCESS(result.error());

  auto obj = result.get_object();
  ASSERT_SUCCESS(obj.error());

  TEST_SUCCEED();
}

// Test 9: First array element
bool test_first_array_element() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_CAR_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"/tire_pressure/0">(doc);
  ASSERT_SUCCESS(result.error());

  double pressure;
  ASSERT_SUCCESS(result.get_double().get(pressure));
  ASSERT_EQUAL(pressure, 40.1);

  TEST_SUCCEED();
}

// Test 10: Multiple indices in path
bool test_multiple_indices() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_NESTED_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"/users/1/age">(doc);
  ASSERT_SUCCESS(result.error());

  int64_t age;
  ASSERT_SUCCESS(result.get_int64().get(age));
  ASSERT_EQUAL(age, 35);

  TEST_SUCCEED();
}

// Test 11: Compare compile-time vs runtime pointer
bool test_compile_vs_runtime() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_USER_JSON).get(doc));

  // Compile-time version
  auto compile_result = ondemand::json_path::at_pointer_compiled<"/name">(doc);
  ASSERT_SUCCESS(compile_result.error());
  std::string_view compile_name;
  ASSERT_SUCCESS(compile_result.get_string().get(compile_name));

  // Runtime version for comparison
  ondemand::parser parser2;
  ondemand::document doc2;
  ASSERT_SUCCESS(parser2.iterate(TEST_USER_JSON).get(doc2));
  auto runtime_result = doc2.at_pointer("/name");
  ASSERT_SUCCESS(runtime_result.error());
  std::string_view runtime_name;
  ASSERT_SUCCESS(runtime_result.get_string().get(runtime_name));

  // Should produce same result
  ASSERT_EQUAL(compile_name, runtime_name);

  TEST_SUCCEED();
}

// Test 12: Nested integer field
bool test_nested_integer() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_NESTED_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"/metadata/count">(doc);
  ASSERT_SUCCESS(result.error());

  int64_t count;
  ASSERT_SUCCESS(result.get_int64().get(count));
  ASSERT_EQUAL(count, 2);

  TEST_SUCCEED();
}

// Test 13: Last array element
bool test_last_array_element() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_CAR_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"/tire_pressure/3">(doc);
  ASSERT_SUCCESS(result.error());

  double pressure;
  ASSERT_SUCCESS(result.get_double().get(pressure));
  ASSERT_EQUAL(pressure, 40.4);

  TEST_SUCCEED();
}

// Test 14: Access second user's email
bool test_second_user_email() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_NESTED_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"/users/1/email">(doc);
  ASSERT_SUCCESS(result.error());

  std::string_view email;
  ASSERT_SUCCESS(result.get_string().get(email));
  ASSERT_EQUAL(email, "bob@example.com");

  TEST_SUCCEED();
}

// Test 15: Root array first element field
bool test_root_array_first_field() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_ARRAY_JSON).get(doc));

  auto result = ondemand::json_path::at_pointer_compiled<"/0/model">(doc);
  ASSERT_SUCCESS(result.error());

  std::string_view model;
  ASSERT_SUCCESS(result.get_string().get(model));
  ASSERT_EQUAL(model, "Camry");

  TEST_SUCCEED();
}

} // namespace compile_time_json_pointer_tests

#endif // SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_STATIC_REFLECTION

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
#if SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_STATIC_REFLECTION
  std::cout << "Running compile-time JSON Pointer tests" << std::endl;

  if (!compile_time_json_pointer_tests::test_simple_field()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_integer_field()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_array_index()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_nested_field()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_array_object_nested()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_root_array()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_deep_nested_array()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_root_pointer()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_first_array_element()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_multiple_indices()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_compile_vs_runtime()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_nested_integer()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_last_array_element()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_second_user_email()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_root_array_first_field()) { return EXIT_FAILURE; }

  std::cout << "All compile-time JSON Pointer tests passed!" << std::endl;
  return EXIT_SUCCESS;
#else
  std::cout << "Compile-time JSON Pointer tests require C++26 reflection support" << std::endl;
  return EXIT_SUCCESS;
#endif
}
