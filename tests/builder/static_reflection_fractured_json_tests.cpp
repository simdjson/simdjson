#include "simdjson.h"
#include "test_builder.h"
#include <string>
#include <string_view>
#include <vector>

using namespace simdjson;

// Test structures for FracturedJson builder integration
struct SimpleUser {
  int id;
  std::string name;
  bool active;
};

struct UserWithEmail {
  int id;
  std::string name;
  std::string email;
  bool active;
};

struct NestedData {
  std::string title;
  std::vector<SimpleUser> users;
  int count;
};

struct TableTestData {
  std::vector<SimpleUser> records;
};

namespace fractured_json_builder_tests {

// Test basic to_fractured_json_string functionality
bool basic_fractured_json_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  SimpleUser user{1, "Alice", true};

  std::string formatted;
  ASSERT_SUCCESS(simdjson::to_fractured_json_string(user).get(formatted));

  // Verify it produces valid JSON by re-parsing
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  ASSERT_SUCCESS(parser.parse(formatted).get(doc));

  // Verify values are preserved
  int64_t id;
  ASSERT_SUCCESS(doc["id"].get_int64().get(id));
  ASSERT_EQUAL(id, 1);

  std::string_view name;
  ASSERT_SUCCESS(doc["name"].get_string().get(name));
  ASSERT_EQUAL(name, "Alice");

  bool active;
  ASSERT_SUCCESS(doc["active"].get_bool().get(active));
  ASSERT_EQUAL(active, true);

  // Output should be inline since it's simple
  std::cout << "Basic FracturedJson output: " << formatted << std::endl;
#endif
  TEST_SUCCEED();
}

// Test with custom formatting options
bool custom_options_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  NestedData data{"Test", {{1, "Alice", true}, {2, "Bob", false}}, 2};

  fractured_json_options opts;
  opts.indent_spaces = 2;
  opts.simple_bracket_padding = false;

  std::string formatted;
  ASSERT_SUCCESS(simdjson::to_fractured_json_string(data, opts).get(formatted));

  // Verify it produces valid JSON
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  ASSERT_SUCCESS(parser.parse(formatted).get(doc));

  std::cout << "Custom options output:\n" << formatted << std::endl;
#endif
  TEST_SUCCEED();
}

// Test to_fractured_json with output parameter
bool output_param_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  SimpleUser user{42, "Charlie", false};

  std::string output;
  ASSERT_SUCCESS(simdjson::to_fractured_json_string(user).get(output));

  // Verify it produces valid JSON
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  ASSERT_SUCCESS(parser.parse(output).get(doc));

  std::cout << "Output param result: " << output << std::endl;
#endif
  TEST_SUCCEED();
}

// Test extract_fractured_json for partial serialization
bool extract_fields_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  UserWithEmail user{1, "Alice", "alice@example.com", true};

  // Extract only id and name fields
 std::string formatted;
  ASSERT_SUCCESS(simdjson::extract_fractured_json<"id", "name">(user).get(formatted));

  // Verify it produces valid JSON
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  ASSERT_SUCCESS(parser.parse(formatted).get(doc));

  // Should have id and name but NOT email and active
  int64_t id;
  ASSERT_SUCCESS(doc["id"].get_int64().get(id));
  ASSERT_EQUAL(id, 1);

  std::string_view name;
  ASSERT_SUCCESS(doc["name"].get_string().get(name));
  ASSERT_EQUAL(name, "Alice");

  // email should not be present
  simdjson::dom::element email_elem;
  auto err = doc["email"].get(email_elem);
  ASSERT_TRUE(err != SUCCESS); // Should fail - field not present

  std::cout << "Extract fields output: " << formatted << std::endl;
#endif
  TEST_SUCCEED();
}

// Test table formatting with array of similar objects
bool table_format_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  TableTestData data{
    {{1, "Alice", true}, {2, "Bob", false}, {3, "Carol", true}, {4, "Dave", false}}
  };

  fractured_json_options opts;
  opts.enable_table_format = true;
  opts.min_table_rows = 3;

  std::string formatted;
  ASSERT_SUCCESS(simdjson::to_fractured_json_string(data, opts).get(formatted));

  // Verify it produces valid JSON
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  ASSERT_SUCCESS(parser.parse(formatted).get(doc));

  std::cout << "Table format output:\n" << formatted << std::endl;
#endif
  TEST_SUCCEED();
}

// Test roundtrip: serialize with reflection, parse back
bool roundtrip_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  SimpleUser original{123, "TestUser", true};

  // Serialize with FracturedJson
  std::string formatted;
  ASSERT_SUCCESS(simdjson::to_fractured_json_string(original).get(formatted));

  // Parse back using ondemand
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(simdjson::pad(formatted)).get(doc));

  // Deserialize back to struct
  SimpleUser parsed;
  ASSERT_SUCCESS(doc.get<SimpleUser>().get(parsed));

  // Verify values match
  ASSERT_EQUAL(parsed.id, original.id);
  ASSERT_EQUAL(parsed.name, original.name);
  ASSERT_EQUAL(parsed.active, original.active);

  std::cout << "Roundtrip test passed with: " << formatted << std::endl;
#endif
  TEST_SUCCEED();
}

// Test global namespace functions
bool global_namespace_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  SimpleUser user{99, "GlobalTest", false};

  // Test global to_fractured_json_string
  std::string formatted;
  ASSERT_SUCCESS(simdjson::to_fractured_json_string(user).get(formatted));
  // Verify output
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  ASSERT_SUCCESS(parser.parse(simdjson::pad(formatted)).get(doc));
  std::cout << "Global namespace output: " << formatted << std::endl;
#endif
  TEST_SUCCEED();
}

bool run() {
  return basic_fractured_json_test() &&
         custom_options_test() &&
         output_param_test() &&
         extract_fields_test() &&
         table_format_test() &&
         roundtrip_test() &&
         global_namespace_test();
}

} // namespace fractured_json_builder_tests

int main(int argc, char *argv[]) {
  std::cout << "Running FracturedJson builder integration tests..." << std::endl;
#if SIMDJSON_STATIC_REFLECTION
  std::cout << "Static reflection is ENABLED" << std::endl;
#else
  std::cout << "Static reflection is DISABLED - tests will be skipped" << std::endl;
#endif
  return test_main(argc, argv, fractured_json_builder_tests::run);
}
