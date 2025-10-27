#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

#if SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_STATIC_REFLECTION

// Test compile-time accessors without struct type validation
// These tests verify that the compile-time path parsing works even without
// providing a struct type for validation
namespace compile_time_no_validation_tests {

const padded_string TEST_JSON = R"({
  "name": "Alice",
  "age": 30,
  "address": {
    "city": "Boston",
    "zip": 12345
  },
  "scores": [95, 87, 92]
})"_padded;

// ============================================================================
// JSON Pointer without validation
// ============================================================================

bool test_pointer_simple_field() {
  TEST_START();
  ondemand::parser parser;
  auto doc = parser.iterate(TEST_JSON);

  std::string_view name;
  auto result = ondemand::json_path::at_pointer_compiled<"/name">(doc);
  ASSERT_SUCCESS(result.get(name));
  ASSERT_EQUAL(name, "Alice");

  TEST_SUCCEED();
}

bool test_pointer_integer_field() {
  TEST_START();
  ondemand::parser parser;
  auto doc = parser.iterate(TEST_JSON);

  int64_t age;
  auto result = ondemand::json_path::at_pointer_compiled<"/age">(doc);
  ASSERT_SUCCESS(result.get(age));
  ASSERT_EQUAL(age, 30);

  TEST_SUCCEED();
}

bool test_pointer_nested_field() {
  TEST_START();
  ondemand::parser parser;
  auto doc = parser.iterate(TEST_JSON);

  std::string_view city;
  auto result = ondemand::json_path::at_pointer_compiled<"/address/city">(doc);
  ASSERT_SUCCESS(result.get(city));
  ASSERT_EQUAL(city, "Boston");

  TEST_SUCCEED();
}

bool test_pointer_nested_integer() {
  TEST_START();
  ondemand::parser parser;
  auto doc = parser.iterate(TEST_JSON);

  int64_t zip;
  auto result = ondemand::json_path::at_pointer_compiled<"/address/zip">(doc);
  ASSERT_SUCCESS(result.get(zip));
  ASSERT_EQUAL(zip, 12345);

  TEST_SUCCEED();
}

bool test_pointer_array_access() {
  TEST_START();
  ondemand::parser parser;
  auto doc = parser.iterate(TEST_JSON);

  int64_t score;
  auto result = ondemand::json_path::at_pointer_compiled<"/scores/1">(doc);
  ASSERT_SUCCESS(result.get(score));
  ASSERT_EQUAL(score, 87);

  TEST_SUCCEED();
}

// ============================================================================
// JSON Path without validation
// ============================================================================

bool test_path_simple_field() {
  TEST_START();
  ondemand::parser parser;
  auto doc = parser.iterate(TEST_JSON);

  std::string_view name;
  auto result = ondemand::json_path::at_path_compiled<".name">(doc);
  ASSERT_SUCCESS(result.get(name));
  ASSERT_EQUAL(name, "Alice");

  TEST_SUCCEED();
}

bool test_path_nested_dot_notation() {
  TEST_START();
  ondemand::parser parser;
  auto doc = parser.iterate(TEST_JSON);

  std::string_view city;
  auto result = ondemand::json_path::at_path_compiled<".address.city">(doc);
  ASSERT_SUCCESS(result.get(city));
  ASSERT_EQUAL(city, "Boston");

  TEST_SUCCEED();
}

bool test_path_array_access() {
  TEST_START();
  ondemand::parser parser;
  auto doc = parser.iterate(TEST_JSON);

  int64_t score;
  auto result = ondemand::json_path::at_path_compiled<".scores[1]">(doc);
  ASSERT_SUCCESS(result.get(score));
  ASSERT_EQUAL(score, 87);

  TEST_SUCCEED();
}

bool test_path_bracket_notation() {
  TEST_START();
  ondemand::parser parser;
  auto doc = parser.iterate(TEST_JSON);

  int64_t age;
  auto result = ondemand::json_path::at_path_compiled<"[\"age\"]">(doc);
  ASSERT_SUCCESS(result.get(age));
  ASSERT_EQUAL(age, 30);

  TEST_SUCCEED();
}

bool test_path_mixed_notation() {
  TEST_START();
  ondemand::parser parser;
  auto doc = parser.iterate(TEST_JSON);

  int64_t zip;
  auto result = ondemand::json_path::at_path_compiled<".address[\"zip\"]">(doc);
  ASSERT_SUCCESS(result.get(zip));
  ASSERT_EQUAL(zip, 12345);

  TEST_SUCCEED();
}

bool test_path_all_bracket_notation() {
  TEST_START();
  ondemand::parser parser;
  auto doc = parser.iterate(TEST_JSON);

  std::string_view city;
  auto result = ondemand::json_path::at_path_compiled<"[\"address\"][\"city\"]">(doc);
  ASSERT_SUCCESS(result.get(city));
  ASSERT_EQUAL(city, "Boston");

  TEST_SUCCEED();
}

bool test_path_with_root_prefix() {
  TEST_START();
  ondemand::parser parser;
  auto doc = parser.iterate(TEST_JSON);

  std::string_view name;
  auto result = ondemand::json_path::at_path_compiled<"$.name">(doc);
  ASSERT_SUCCESS(result.get(name));
  ASSERT_EQUAL(name, "Alice");

  TEST_SUCCEED();
}

} // namespace compile_time_no_validation_tests

#endif // SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_STATIC_REFLECTION

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
#if SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_STATIC_REFLECTION
  std::cout << "Running compile-time accessor tests WITHOUT struct validation" << std::endl;

  // JSON Pointer tests
  if (!compile_time_no_validation_tests::test_pointer_simple_field()) { return EXIT_FAILURE; }
  if (!compile_time_no_validation_tests::test_pointer_integer_field()) { return EXIT_FAILURE; }
  if (!compile_time_no_validation_tests::test_pointer_nested_field()) { return EXIT_FAILURE; }
  if (!compile_time_no_validation_tests::test_pointer_nested_integer()) { return EXIT_FAILURE; }
  if (!compile_time_no_validation_tests::test_pointer_array_access()) { return EXIT_FAILURE; }

  // JSON Path tests
  if (!compile_time_no_validation_tests::test_path_simple_field()) { return EXIT_FAILURE; }
  if (!compile_time_no_validation_tests::test_path_nested_dot_notation()) { return EXIT_FAILURE; }
  if (!compile_time_no_validation_tests::test_path_array_access()) { return EXIT_FAILURE; }
  if (!compile_time_no_validation_tests::test_path_bracket_notation()) { return EXIT_FAILURE; }
  if (!compile_time_no_validation_tests::test_path_mixed_notation()) { return EXIT_FAILURE; }
  if (!compile_time_no_validation_tests::test_path_all_bracket_notation()) { return EXIT_FAILURE; }
  if (!compile_time_no_validation_tests::test_path_with_root_prefix()) { return EXIT_FAILURE; }

  std::cout << "All compile-time accessor tests WITHOUT validation passed!" << std::endl;
  return EXIT_SUCCESS;
#else
  std::cout << "Compile-time accessor tests require C++26 reflection support" << std::endl;
  return EXIT_SUCCESS;
#endif
}
