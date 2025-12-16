#include "simdjson.h"
#include "test_ondemand.h"
#include <string>

#if SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_STATIC_REFLECTION

using namespace simdjson;

namespace compile_time_json_path_tests {

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

// Nested struct types for testing deep nesting
struct Location {
  double latitude;
  double longitude;
};

struct Address {
  std::string street;
  std::string city;
  int zip;
  Location location;  // Nested 2 levels deep
};

struct Person {
  std::string name;
  int age;
  Address address;  // Nested struct
  std::vector<std::string> emails;  // Array field
};

const padded_string TEST_USER_JSON = R"(
{
  "name": "John Doe",
  "age": 30,
  "email": "john@example.com"
}
)"_padded;

const padded_string TEST_PERSON_JSON = R"(
{
  "name": "Jane Smith",
  "age": 28,
  "address": {
    "street": "123 Main St",
    "city": "Springfield",
    "zip": 12345,
    "location": {
      "latitude": 42.1234,
      "longitude": -71.5678
    }
  },
  "emails": ["jane@example.com", "jane.smith@work.com"]
}
)"_padded;

// ============================================================================
// Tests for JSON Path Syntax (dot notation and brackets)
// ============================================================================

// Test 1: Nested struct with JSON Path syntax (2 levels: Person -> Address -> city)
bool test_nested_struct_path() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_PERSON_JSON).get(doc));

  std::string city;
  using accessor = ondemand::json_path::path_accessor<Person, ".address.city">;
  ASSERT_SUCCESS(accessor::extract_field(doc, city));
  ASSERT_EQUAL(city, "Springfield");

  TEST_SUCCEED();
}

// Test 2: Deep nested with bracket notation (3 levels)
bool test_nested_struct_bracket_notation() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_PERSON_JSON).get(doc));

  double longitude;
  using accessor = ondemand::json_path::path_accessor<Person, "[\"address\"][\"location\"][\"longitude\"]">;
  ASSERT_SUCCESS(accessor::extract_field(doc, longitude));
  ASSERT_EQUAL(longitude, -71.5678);

  TEST_SUCCEED();
}

// Test 3: Mixed dot and bracket notation on nested structs
bool test_nested_struct_mixed_notation() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_PERSON_JSON).get(doc));

  int zip;
  using accessor = ondemand::json_path::path_accessor<Person, ".address[\"zip\"]">;
  ASSERT_SUCCESS(accessor::extract_field(doc, zip));
  ASSERT_EQUAL(zip, 12345);

  TEST_SUCCEED();
}

// ============================================================================
// Tests for extract_field() with JSON Path - Reflection-based direct extraction
// ============================================================================

// Test 4: extract_field simple string field (JSON Path dot notation)
bool test_extract_field_path_simple() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_USER_JSON).get(doc));

  std::string name;
  using accessor = ondemand::json_path::path_accessor<User, ".name">;
  ASSERT_SUCCESS(accessor::extract_field(doc, name));
  ASSERT_EQUAL(name, "John Doe");

  TEST_SUCCEED();
}

// Test 5: extract_field integer field (JSON Path dot notation)
bool test_extract_field_path_integer() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_USER_JSON).get(doc));

  int age;
  using accessor = ondemand::json_path::path_accessor<User, ".age">;
  ASSERT_SUCCESS(accessor::extract_field(doc, age));
  ASSERT_EQUAL(age, 30);

  TEST_SUCCEED();
}

// Test 6: extract_field bracket notation (JSON Path bracket notation)
bool test_extract_field_path_bracket() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_USER_JSON).get(doc));

  std::string email;
  using accessor = ondemand::json_path::path_accessor<User, "[\"email\"]">;
  ASSERT_SUCCESS(accessor::extract_field(doc, email));
  ASSERT_EQUAL(email, "john@example.com");

  TEST_SUCCEED();
}

// Test 7: JSON Path with nested field using dot notation
bool test_path_nested_dot_notation() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_PERSON_JSON).get(doc));

  std::string street;
  using accessor = ondemand::json_path::path_accessor<Person, ".address.street">;
  ASSERT_SUCCESS(accessor::extract_field(doc, street));
  ASSERT_EQUAL(street, "123 Main St");

  TEST_SUCCEED();
}

// Test 8: JSON Path with deep nested field (3 levels)
bool test_path_deep_nested() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_PERSON_JSON).get(doc));

  double latitude;
  using accessor = ondemand::json_path::path_accessor<Person, ".address.location.latitude">;
  ASSERT_SUCCESS(accessor::extract_field(doc, latitude));
  ASSERT_EQUAL(latitude, 42.1234);

  TEST_SUCCEED();
}

// Test 9: JSON Path with bracket notation for all levels
bool test_path_all_bracket_notation() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_PERSON_JSON).get(doc));

  std::string city;
  using accessor = ondemand::json_path::path_accessor<Person, "[\"address\"][\"city\"]">;
  ASSERT_SUCCESS(accessor::extract_field(doc, city));
  ASSERT_EQUAL(city, "Springfield");

  TEST_SUCCEED();
}

// Test 10: JSON Path mixed notation with integer field
bool test_path_mixed_notation_integer() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_PERSON_JSON).get(doc));

  int age;
  using accessor = ondemand::json_path::path_accessor<Person, "[\"age\"]">;
  ASSERT_SUCCESS(accessor::extract_field(doc, age));
  ASSERT_EQUAL(age, 28);

  TEST_SUCCEED();
}

} // namespace compile_time_json_path_tests

#endif // SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_STATIC_REFLECTION

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
#if SIMDJSON_SUPPORTS_CONCEPTS && SIMDJSON_STATIC_REFLECTION
  std::cout << "Running compile-time JSON Path tests" << std::endl;

  if (!compile_time_json_path_tests::test_nested_struct_path()) { return EXIT_FAILURE; }
  if (!compile_time_json_path_tests::test_nested_struct_bracket_notation()) { return EXIT_FAILURE; }
  if (!compile_time_json_path_tests::test_nested_struct_mixed_notation()) { return EXIT_FAILURE; }
  if (!compile_time_json_path_tests::test_extract_field_path_simple()) { return EXIT_FAILURE; }
  if (!compile_time_json_path_tests::test_extract_field_path_integer()) { return EXIT_FAILURE; }
  if (!compile_time_json_path_tests::test_extract_field_path_bracket()) { return EXIT_FAILURE; }
  if (!compile_time_json_path_tests::test_path_nested_dot_notation()) { return EXIT_FAILURE; }
  if (!compile_time_json_path_tests::test_path_deep_nested()) { return EXIT_FAILURE; }
  if (!compile_time_json_path_tests::test_path_all_bracket_notation()) { return EXIT_FAILURE; }
  if (!compile_time_json_path_tests::test_path_mixed_notation_integer()) { return EXIT_FAILURE; }

  std::cout << "All compile-time JSON Path tests passed!" << std::endl;
  return EXIT_SUCCESS;
#else
  std::cout << "Compile-time JSON Path tests require C++26 reflection support" << std::endl;
  return EXIT_SUCCESS;
#endif
}
