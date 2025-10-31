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

// ============================================================================
// Tests for Nested Struct Type Validation with Reflection
// ============================================================================

// Test 16: Deep nested field access (3 levels: Person -> Address -> Location -> latitude)
bool test_nested_struct_deep_pointer() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_PERSON_JSON).get(doc));

  double latitude;
  using accessor = ondemand::json_path::pointer_accessor<Person, "/address/location/latitude">;
  ASSERT_SUCCESS(accessor::extract_field(doc, latitude));
  ASSERT_EQUAL(latitude, 42.1234);

  TEST_SUCCEED();
}

// Test 17: Nested struct with int field (2 levels: Person -> Address -> zip)
bool test_nested_struct_integer() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_PERSON_JSON).get(doc));

  int zip;
  using accessor = ondemand::json_path::pointer_accessor<Person, "/address/zip">;
  ASSERT_SUCCESS(accessor::extract_field(doc, zip));
  ASSERT_EQUAL(zip, 12345);

  TEST_SUCCEED();
}

// Test 18: Nested struct with double field (3 levels deep)
bool test_nested_struct_longitude() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_PERSON_JSON).get(doc));

  double longitude;
  using accessor = ondemand::json_path::pointer_accessor<Person, "/address/location/longitude">;
  ASSERT_SUCCESS(accessor::extract_field(doc, longitude));
  ASSERT_EQUAL(longitude, -71.5678);

  TEST_SUCCEED();
}

// Test 19: Nested struct string field (2 levels)
bool test_nested_struct_street() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_PERSON_JSON).get(doc));

  std::string street;
  using accessor = ondemand::json_path::pointer_accessor<Person, "/address/street">;
  ASSERT_SUCCESS(accessor::extract_field(doc, street));
  ASSERT_EQUAL(street, "123 Main St");

  TEST_SUCCEED();
}

// ============================================================================
// Compile-Time Error Tests (Documentation Only - These Should NOT Compile)
// ============================================================================
//
// These tests demonstrate compile-time safety. If you uncomment them, they will
// fail to compile with clear error messages.
//
// Example 1: Type mismatch - trying to extract string into int
// bool test_compile_error_type_mismatch() {
//   ondemand::parser parser;
//   ondemand::document doc;
//   parser.iterate(TEST_USER_JSON).get(doc);
//
//   int name;  // ERROR: name is std::string, not int!
//   using accessor = ondemand::json_path::pointer_accessor<User, "/name">;
//   accessor::extract_field(doc, name);
//   // Compile error: "Target type does not match the field type at the pointer"
//   // static_assert fails: ^^std::string != ^^int
// }
//
// Example 2: Non-existent field
// bool test_compile_error_invalid_field() {
//   ondemand::parser parser;
//   ondemand::document doc;
//   parser.iterate(TEST_USER_JSON).get(doc);
//
//   std::string foo;
//   using accessor = ondemand::json_path::pointer_accessor<User, "/nonexistent">;
//   accessor::extract_field(doc, foo);
//   // Compile error: "JSON Pointer does not match struct definition"
//   // Field "nonexistent" not found in User struct
// }
//
// Example 3: Wrong nested path
// bool test_compile_error_wrong_nested_path() {
//   ondemand::parser parser;
//   ondemand::document doc;
//   parser.iterate(TEST_PERSON_JSON).get(doc);
//
//   std::string foo;
//   using accessor = ondemand::json_path::pointer_accessor<Person, "/address/invalid/field">;
//   accessor::extract_field(doc, foo);
//   // Compile error: "JSON Pointer does not match struct definition"
//   // Field "invalid" not found in Address struct
// }
//
// Example 4: Array index on non-array field
// bool test_compile_error_array_on_scalar() {
//   ondemand::parser parser;
//   ondemand::document doc;
//   parser.iterate(TEST_USER_JSON).get(doc);
//
//   std::string foo;
//   using accessor = ondemand::json_path::pointer_accessor<User, "/name/0">;
//   accessor::extract_field(doc, foo);
//   // Compile error: "JSON Pointer does not match struct definition"
//   // Can't use array index on std::string field
// }
//
// Example 5: Deep nesting type mismatch
// bool test_compile_error_deep_nesting_type_mismatch() {
//   ondemand::parser parser;
//   ondemand::document doc;
//   parser.iterate(TEST_PERSON_JSON).get(doc);
//
//   int latitude;  // ERROR: latitude is double, not int!
//   using accessor = ondemand::json_path::pointer_accessor<Person, "/address/location/latitude">;
//   accessor::extract_field(doc, latitude);
//   // Compile error: "Target type does not match the field type at the pointer"
//   // static_assert fails: ^^double != ^^int
// }

// ============================================================================
// Tests for extract_field() - Reflection-based direct extraction
// ============================================================================

// ============================================================================
// Tests for extract_field() with JSON Pointer - Reflection-based direct extraction
// ============================================================================

// Test 20: extract_field simple string field (JSON Pointer)
bool test_extract_field_pointer_simple() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_USER_JSON).get(doc));

  std::string name;
  using accessor = ondemand::json_path::pointer_accessor<User, "/name">;
  ASSERT_SUCCESS(accessor::extract_field(doc, name));
  ASSERT_EQUAL(name, "John Doe");

  TEST_SUCCEED();
}

// Test 21: extract_field integer field (JSON Pointer)
bool test_extract_field_pointer_integer() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_USER_JSON).get(doc));

  int age;
  using accessor = ondemand::json_path::pointer_accessor<User, "/age">;
  ASSERT_SUCCESS(accessor::extract_field(doc, age));
  ASSERT_EQUAL(age, 30);

  TEST_SUCCEED();
}

// Test 22: extract_field email field (JSON Pointer)
bool test_extract_field_pointer_email() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_USER_JSON).get(doc));

  std::string email;
  using accessor = ondemand::json_path::pointer_accessor<User, "/email">;
  ASSERT_SUCCESS(accessor::extract_field(doc, email));
  ASSERT_EQUAL(email, "john@example.com");

  TEST_SUCCEED();
}

// Test 23: extract_field with Car struct (JSON Pointer)
bool test_extract_field_pointer_car_make() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_CAR_JSON).get(doc));

  std::string make;
  using accessor = ondemand::json_path::pointer_accessor<Car, "/make">;
  ASSERT_SUCCESS(accessor::extract_field(doc, make));
  ASSERT_EQUAL(make, "Toyota");

  TEST_SUCCEED();
}

// Test 24: extract_field with year (int64_t) (JSON Pointer)
bool test_extract_field_pointer_car_year() {
  TEST_START();
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(TEST_CAR_JSON).get(doc));

  int64_t year;
  using accessor = ondemand::json_path::pointer_accessor<Car, "/year">;
  ASSERT_SUCCESS(accessor::extract_field(doc, year));
  ASSERT_EQUAL(year, 2018);

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

  // Test nested struct type validation with reflection (JSON Pointer syntax only)
  std::cout << "\nRunning nested struct validation tests..." << std::endl;
  if (!compile_time_json_pointer_tests::test_nested_struct_deep_pointer()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_nested_struct_integer()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_nested_struct_longitude()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_nested_struct_street()) { return EXIT_FAILURE; }

  // Test extract_field() API with JSON Pointer
  std::cout << "\nRunning extract_field() with JSON Pointer..." << std::endl;
  if (!compile_time_json_pointer_tests::test_extract_field_pointer_simple()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_extract_field_pointer_integer()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_extract_field_pointer_email()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_extract_field_pointer_car_make()) { return EXIT_FAILURE; }
  if (!compile_time_json_pointer_tests::test_extract_field_pointer_car_year()) { return EXIT_FAILURE; }

  std::cout << "All compile-time JSON Pointer tests passed!" << std::endl;
  return EXIT_SUCCESS;
#else
  std::cout << "Compile-time JSON Pointer tests require C++26 reflection support" << std::endl;
  return EXIT_SUCCESS;
#endif
}
