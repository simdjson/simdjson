#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "simdjson.h"
#include "test_macros.h"

// Helper function to count newlines in a string
static size_t count_newlines(const std::string& s) {
  size_t count = 0;
  for (char c : s) {
    if (c == '\n') count++;
  }
  return count;
}

// Test that fractured_json produces valid JSON that can be re-parsed
bool roundtrip_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({"users":[{"id":1,"name":"Alice","active":true},{"id":2,"name":"Bob","active":false}],"count":2})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  // Format with fractured_json
  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Formatted output:\n" << formatted << std::endl;

  // Re-parse the formatted output
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  // Format again and compare
  auto formatted2 = simdjson::fractured_json(doc);
  if (formatted != formatted2) {
    std::cerr << "Formatted outputs differ!" << std::endl;
    return false;
  }

  std::cout << "Roundtrip test passed." << std::endl;
  return true;
}

// Test inline formatting for simple arrays
bool inline_array_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([1, 2, 3, 4, 5])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_inline_complexity = 2;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted: " << formatted << std::endl;

  // Should be inline (single line, no newlines except possibly at end)
  size_t newline_count = count_newlines(formatted);
  if (newline_count > 1) {
    std::cerr << "Simple array should be inline but has " << newline_count << " newlines" << std::endl;
    return false;
  }

  std::cout << "Inline array test passed." << std::endl;
  return true;
}

// Test inline formatting for simple objects
bool inline_object_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({"a": 1, "b": 2})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted: " << formatted << std::endl;

  // Should be inline (single line)
  size_t newline_count = count_newlines(formatted);
  if (newline_count > 1) {
    std::cerr << "Simple object should be inline but has " << newline_count << " newlines" << std::endl;
    return false;
  }

  std::cout << "Inline object test passed." << std::endl;
  return true;
}

// Test expanded formatting for complex nested structures
bool expanded_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({"level1":{"level2":{"level3":{"value":42}}}})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_inline_complexity = 1; // Force expansion

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted:\n" << formatted << std::endl;

  // Should have multiple lines (at least 3 newlines for nested structure)
  size_t newline_count = count_newlines(formatted);
  if (newline_count < 3) {
    std::cerr << "Complex nested object should be expanded" << std::endl;
    return false;
  }

  std::cout << "Expanded test passed." << std::endl;
  return true;
}

// Test compact multiline arrays
bool compact_multiline_test() {
  std::cout << "Running " << __func__ << std::endl;

  // Array with many simple elements
  const char* json = R"([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 40; // Force compact multiline
  opts.enable_compact_multiline = true;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted:\n" << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Compact multiline test passed." << std::endl;
  return true;
}

// Test table formatting for uniform arrays
bool table_format_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([
    {"id": 1, "name": "Alice", "score": 95},
    {"id": 2, "name": "Bob", "score": 87},
    {"id": 3, "name": "Carol", "score": 92},
    {"id": 4, "name": "Dave", "score": 78}
  ])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.enable_table_format = true;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted (table):\n" << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Table format test passed." << std::endl;
  return true;
}

// Test empty containers
bool empty_containers_test() {
  std::cout << "Running " << __func__ << std::endl;

  simdjson::dom::parser parser;
  simdjson::dom::element doc;

  // Empty array
  const char* empty_array = "[]";
  auto error = parser.parse(empty_array, strlen(empty_array)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Empty array: " << formatted << std::endl;

  if (formatted.find("[]") == std::string::npos) {
    std::cerr << "Empty array should format as []" << std::endl;
    return false;
  }

  // Empty object
  const char* empty_object = "{}";
  error = parser.parse(empty_object, strlen(empty_object)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  formatted = simdjson::fractured_json(doc);
  std::cout << "Empty object: " << formatted << std::endl;

  if (formatted.find("{}") == std::string::npos) {
    std::cerr << "Empty object should format as {}" << std::endl;
    return false;
  }

  std::cout << "Empty containers test passed." << std::endl;
  return true;
}

// Test all scalar types
bool scalar_types_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({
    "string": "hello world",
    "int": 42,
    "negative": -123,
    "uint": 18446744073709551615,
    "float": 3.14159,
    "bool_true": true,
    "bool_false": false,
    "null_val": null
  })";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Formatted:\n" << formatted << std::endl;

  // Re-parse and verify
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Scalar types test passed." << std::endl;
  return true;
}

// Test string escaping
bool string_escaping_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({"escaped": "line1\nline2\ttab\"quote\\backslash"})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Formatted: " << formatted << std::endl;

  // Re-parse and verify
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  // Verify the value is preserved
  std::string_view val;
  error = doc["escaped"].get_string().get(val);
  if (error) {
    std::cerr << "Get string error: " << error << std::endl;
    return false;
  }

  if (val != "line1\nline2\ttab\"quote\\backslash") {
    std::cerr << "String value not preserved!" << std::endl;
    return false;
  }

  std::cout << "String escaping test passed." << std::endl;
  return true;
}

// Test custom options
bool custom_options_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({"a": [1, 2, 3], "b": {"x": 1}})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  // Test with different indent sizes
  simdjson::fractured_json_options opts;
  opts.indent_spaces = 2;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "With 2-space indent:\n" << formatted << std::endl;

  opts.indent_spaces = 8;
  formatted = simdjson::fractured_json(doc, opts);
  std::cout << "With 8-space indent:\n" << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Custom options test passed." << std::endl;
  return true;
}

// Test deeply nested structures
bool deep_nesting_test() {
  std::cout << "Running " << __func__ << std::endl;

  // Build deeply nested JSON
  std::string json = "{";
  for (int i = 0; i < 10; i++) {
    json += "\"level" + std::to_string(i) + "\":{";
  }
  json += "\"value\":42";
  for (int i = 0; i < 10; i++) {
    json += "}";
  }
  json += "}";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Deeply nested (first 500 chars):\n" << formatted.substr(0, 500) << "..." << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Deep nesting test passed." << std::endl;
  return true;
}

// Test mixed array types
bool mixed_array_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([1, "two", true, null, {"nested": "object"}, [1, 2, 3]])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Mixed array:\n" << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Mixed array test passed." << std::endl;
  return true;
}

// Test fractured_json_string for formatting JSON strings
bool string_api_test() {
  std::cout << "Running " << __func__ << std::endl;

  // Test with a minified JSON string (simulating output from to_json_string)
  std::string minified = R"({"users":[{"id":1,"name":"Alice"},{"id":2,"name":"Bob"}],"count":2})";

  auto formatted = simdjson::fractured_json_string(minified);
  std::cout << "From string:\n" << formatted << std::endl;

  // Verify it's valid JSON
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  // Test with custom options
  simdjson::fractured_json_options opts;
  opts.indent_spaces = 2;
  formatted = simdjson::fractured_json_string(minified, opts);
  std::cout << "With 2-space indent:\n" << formatted << std::endl;

  std::cout << "String API test passed." << std::endl;
  return true;
}

// Test Unicode strings
bool unicode_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({
    "greeting": "Hello, \u4e16\u754c!",
    "emoji": "\ud83c\udf89\ud83d\ude80\u2728",
    "arabic": "\u0645\u0631\u062d\u0628\u0627",
    "russian": "\u041f\u0440\u0438\u0432\u0435\u0442",
    "mixed": "caf\u00e9 r\u00e9sum\u00e9 na\u00efve"
  })";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Unicode formatted:\n" << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Unicode test passed." << std::endl;
  return true;
}

// Test boundary number values
bool boundary_numbers_test() {
  std::cout << "Running " << __func__ << std::endl;

  // Using string literals for extreme values to ensure valid JSON
  const char* json = R"({
    "max_int64": 9223372036854775807,
    "min_int64": -9223372036854775808,
    "max_uint64": 18446744073709551615,
    "zero": 0,
    "neg_zero": -0,
    "small_double": 2.2250738585072014e-308,
    "large_double": 1.7976931348623157e+308,
    "pi": 3.141592653589793
  })";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Boundary numbers formatted:\n" << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Boundary numbers test passed." << std::endl;
  return true;
}

// Test arrays of arrays (nested arrays)
bool nested_arrays_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([
    [[1, 2], [3, 4]],
    [[5, 6], [7, 8]],
    [[9, 10], [11, 12]]
  ])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Nested arrays formatted:\n" << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Nested arrays test passed." << std::endl;
  return true;
}

// Test empty strings
bool empty_string_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({"empty": "", "not_empty": "value", "also_empty": ""})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Empty string formatted: " << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  // Verify empty strings preserved
  std::string_view val;
  error = doc["empty"].get_string().get(val);
  if (error || !val.empty()) {
    std::cerr << "Empty string not preserved!" << std::endl;
    return false;
  }

  std::cout << "Empty string test passed." << std::endl;
  return true;
}

// Test keys with special characters
bool special_key_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({
    "normal": 1,
    "with space": 2,
    "with-dash": 3,
    "with.dot": 4,
    "with:colon": 5,
    "with\"quote": 6,
    "with\\backslash": 7
  })";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Special keys formatted:\n" << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Special key test passed." << std::endl;
  return true;
}

// Test non-uniform array (shouldn't trigger table mode since it would be too wide)
bool non_uniform_array_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([
    {"id": 1, "name": "Alice"},
    {"id": 2, "email": "bob@example.com"},
    {"id": 3, "name": "Carol", "age": 30},
    {"different": "structure", "completely": true}
  ])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.enable_table_format = true;
  opts.max_total_line_length = 80;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Non-uniform array formatted:\n" << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Non-uniform array test passed." << std::endl;
  return true;
}

// Test very long string
bool long_string_test() {
  std::cout << "Running " << __func__ << std::endl;

  // Create JSON with a long string value
  std::string long_value(500, 'x');
  std::string json = R"({"short": "abc", "long": ")" + long_value + R"("})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Long string formatted (first 200 chars):\n" << formatted.substr(0, 200) << "..." << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  // Verify long string preserved
  std::string_view val;
  error = doc["long"].get_string().get(val);
  if (error || val.length() != 500) {
    std::cerr << "Long string not preserved! Length: " << val.length() << std::endl;
    return false;
  }

  std::cout << "Long string test passed." << std::endl;
  return true;
}

// Test large array with many elements (triggers compact multiline)
bool large_array_test() {
  std::cout << "Running " << __func__ << std::endl;

  // Create array with 100 elements
  std::string json = "[";
  for (int i = 0; i < 100; i++) {
    if (i > 0) json += ",";
    json += std::to_string(i);
  }
  json += "]";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 80;
  opts.enable_compact_multiline = true;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Large array formatted (first 300 chars):\n" << formatted.substr(0, 300) << "..." << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  // Verify array length
  simdjson::dom::array arr;
  error = doc.get_array().get(arr);
  if (error) {
    std::cerr << "Get array error: " << error << std::endl;
    return false;
  }

  size_t count = 0;
  for (auto elem : arr) {
    (void)elem;
    count++;
  }

  if (count != 100) {
    std::cerr << "Array count mismatch: " << count << " != 100" << std::endl;
    return false;
  }

  std::cout << "Large array test passed." << std::endl;
  return true;
}

// Test reflection API workflow (simulated)
bool reflection_workflow_test() {
  std::cout << "Running " << __func__ << std::endl;

  // Simulate what to_json_string would produce from a struct
  // struct User { int id; std::string name; bool active; };
  // std::vector<User> users = {{1, "Alice", true}, {2, "Bob", false}};
  std::string minified = R"({"users":[{"id":1,"name":"Alice","active":true},{"id":2,"name":"Bob","active":false}],"total":2})";

  // This is the typical workflow with reflection API:
  // auto minified = simdjson::to_json_string(my_struct);
  // auto formatted = simdjson::fractured_json_string(minified.value());

  auto formatted = simdjson::fractured_json_string(minified);
  std::cout << "Reflection workflow output:\n" << formatted << std::endl;

  // Verify it produces valid, readable JSON
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  // Test with custom options
  simdjson::fractured_json_options opts;
  opts.indent_spaces = 2;
  opts.simple_bracket_padding = false;

  formatted = simdjson::fractured_json_string(minified, opts);
  std::cout << "With custom options:\n" << formatted << std::endl;

  std::cout << "Reflection workflow test passed." << std::endl;
  return true;
}

// Test control characters
bool control_characters_test() {
  std::cout << "Running " << __func__ << std::endl;

  // JSON with various control characters (escaped in the source)
  const char* json = R"({"tab": "a\tb", "newline": "a\nb", "cr": "a\rb", "null_char": "a\u0000b"})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Control characters formatted: " << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Control characters test passed." << std::endl;
  return true;
}

// Test single element containers
bool single_element_test() {
  std::cout << "Running " << __func__ << std::endl;

  simdjson::dom::parser parser;
  simdjson::dom::element doc;

  // Single element array
  const char* single_array = "[42]";
  auto error = parser.parse(single_array, strlen(single_array)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  auto formatted = simdjson::fractured_json(doc);
  std::cout << "Single element array: " << formatted << std::endl;

  // Single key object
  const char* single_object = R"({"only_key": "only_value"})";
  error = parser.parse(single_object, strlen(single_object)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  formatted = simdjson::fractured_json(doc);
  std::cout << "Single key object: " << formatted << std::endl;

  std::cout << "Single element test passed." << std::endl;
  return true;
}

// Test option: disable compact multiline
bool disable_compact_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 20;
  opts.enable_compact_multiline = false;  // Force expanded mode

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Compact disabled (expanded):\n" << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Disable compact test passed." << std::endl;
  return true;
}

// Test option: disable table format
bool disable_table_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([
    {"id": 1, "name": "Alice"},
    {"id": 2, "name": "Bob"},
    {"id": 3, "name": "Carol"}
  ])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.enable_table_format = false;  // Disable table mode

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Table disabled:\n" << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Disable table test passed." << std::endl;
  return true;
}

// Test option: no padding
bool no_padding_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({"a": [1, 2, 3], "b": {"x": 1}})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.simple_bracket_padding = false;
  opts.nested_bracket_padding = false;
  opts.colon_padding = false;
  opts.comma_padding = false;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "No padding: " << formatted << std::endl;

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "No padding test passed." << std::endl;
  return true;
}

bool compact_multiline_oversized_item_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([{"a":true,"b":true,"c":true,"d":true}])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 10;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted:\n" << formatted << std::endl;

  const char* expected =
      "[\n"
      "    {\n"
      "        \"a\": true,\n"
      "        \"b\": true,\n"
      "        \"c\": true,\n"
      "        \"d\": true\n"
      "    }\n"
      "]";
  ASSERT_EQUAL(formatted, expected);

  std::cout << "Compact multiline oversized item test passed." << std::endl;
  return true;
}

bool compact_multiline_respects_max_total_line_length_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json =
      R"(["aaaaaaaaaaaaaaaaaaaaaaaaa","bbbbbbbbbbbbbbbbbbbbbbbbb","ccccccccccccccccccccccccc",)"
      R"("ddddddddddddddddddddddddd","eeeeeeeeeeeeeeeeeeeeeeeee","fffffffffffffffffffffffff"])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 50;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted:\n" << formatted << std::endl;

  std::string line;
  std::istringstream stream(formatted);
  while (std::getline(stream, line)) {
    if (line.size() > opts.max_total_line_length) {
      std::cerr << "Line exceeds max_total_line_length (" << opts.max_total_line_length
                << "): \"" << line << "\" (" << line.size() << " chars)" << std::endl;
      return false;
    }
  }

  // Re-parse to verify validity
  error = parser.parse(formatted).get(doc);
  if (error) {
    std::cerr << "Re-parse error: " << error << std::endl;
    return false;
  }

  std::cout << "Compact multiline max total line length test passed." << std::endl;
  return true;
}

bool bracket_padding_counted_in_array_length_test() {
  std::cout << "Running " << __func__ << std::endl;

  // unpadded content is 14 chars
  const char* json = R"(["0123456789"])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 15;
  opts.simple_bracket_padding = true;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted (limit=15):\n" << formatted << std::endl;

  const char* expected_expanded =
      "[\n"
      "    \"0123456789\"\n"
      "]";
  ASSERT_EQUAL(formatted, expected_expanded);

  opts.max_total_line_length = 14;
  opts.simple_bracket_padding = false;
  formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted (limit=14):\n" << formatted << std::endl;
  ASSERT_EQUAL(formatted, "[\"0123456789\"]");

  opts.max_total_line_length = 16;
  opts.simple_bracket_padding = true;
  formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted (limit=16):\n" << formatted << std::endl;
  ASSERT_EQUAL(formatted, "[ \"0123456789\" ]");

  std::cout << "Bracket padding counted in length test passed." << std::endl;
  return true;
}

bool bracket_padding_counted_in_object_length_test() {
  std::cout << "Running " << __func__ << std::endl;

  // unpadded content is 19 chars
  const char* json = R"({"x":"0123456789"})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 20;
  opts.simple_bracket_padding = true;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted (limit=20):\n" << formatted << std::endl;

  const char* expected_expanded =
      "{\n"
      "    \"x\": \"0123456789\"\n"
      "}";
  ASSERT_EQUAL(formatted, expected_expanded);

  opts.max_total_line_length = 19;
  opts.simple_bracket_padding = false;
  formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted (limit=19):\n" << formatted << std::endl;
  ASSERT_EQUAL(formatted, "{\"x\": \"0123456789\"}");

  opts.max_total_line_length = 21;
  opts.simple_bracket_padding = true;
  formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted (limit=21):\n" << formatted << std::endl;
  ASSERT_EQUAL(formatted, "{ \"x\": \"0123456789\" }");

  std::cout << "Bracket padding counted in length test passed." << std::endl;
  return true;
}

bool compact_multiline_breaks_after_expanded_item_test() {
  std::cout << "Running " << __func__ << std::endl;

  std::string big_string(40, '0');
  std::string json = R"([{"a":true,"b":")" + big_string + R"("},{"a":true,"b":"x"}])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 44;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted:\n" << formatted << std::endl;

  std::string expected =
      "[\n"
      "    {\n"
      "        \"a\": true,\n"
      "        \"b\": \"" + big_string + "\"\n"
      "    },\n"
      "    { \"a\": true, \"b\": \"x\" }\n"
      "]";
  ASSERT_EQUAL(formatted, expected);

  std::cout << "Compact multiline breaks after expanded item test passed." << std::endl;
  return true;
}

bool nested_object_respects_max_total_line_length_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({"a":{"b":{"c":{"d":"hello world"}}}})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 30;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted:\n" << formatted << std::endl;

  const char* expected =
      "{\n"
      "    \"a\": {\n"
      "        \"b\": {\n"
      "            \"c\": {\n"
      "                \"d\": \"hello world\"\n"
      "            }\n"
      "        }\n"
      "    }\n"
      "}";
  ASSERT_EQUAL(formatted, expected);

  std::cout << "Nested object max total line length test passed." << std::endl;
  return true;
}

bool nested_array_respects_max_total_line_length_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({"a":{"b":{"c":["hello world"]}}})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 25;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted:\n" << formatted << std::endl;

  const char* expected =
      "{\n"
      "    \"a\": {\n"
      "        \"b\": {\n"
      "            \"c\": [\n"
      "                \"hello world\"\n"
      "            ]\n"
      "        }\n"
      "    }\n"
      "}";
  ASSERT_EQUAL(formatted, expected);

  std::cout << "Nested array max total line length test passed." << std::endl;
  return true;
}

bool compact_multiline_item_respects_max_total_line_length_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json =
      R"([{"a":true,"b":"ABCDEFGHIJKLMNOPQRST"},{"a":true,"b":"ABCDEFGHIJKLMNOPQRST"}])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 44;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted:\n" << formatted << std::endl;

  const char* expected =
      "[\n"
      "    {\n"
      "        \"a\": true,\n"
      "        \"b\": \"ABCDEFGHIJKLMNOPQRST\"\n"
      "    },\n"
      "    {\n"
      "        \"a\": true,\n"
      "        \"b\": \"ABCDEFGHIJKLMNOPQRST\"\n"
      "    }\n"
      "]";
  ASSERT_EQUAL(formatted, expected);

  std::cout << "Compact multiline item max total line length test passed." << std::endl;
  return true;
}

bool trailing_comma_counted_in_length_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([{"x":"0123456789"},{"y":"0123456789"}])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 25;
  opts.enable_compact_multiline = false;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted:\n" << formatted << std::endl;

  const char* expected =
      "[\n"
      "    {\n"
      "        \"x\": \"0123456789\"\n"
      "    },\n"
      "    { \"y\": \"0123456789\" }\n"
      "]";
  ASSERT_EQUAL(formatted, expected);

  std::cout << "Trailing comma counted in length test passed." << std::endl;
  return true;
}

bool nested_bracket_padding_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({"a":[1,2]})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  {
    simdjson::fractured_json_options opts;
    opts.max_inline_complexity = 3;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted, "{ \"a\": [ 1, 2 ] }");
  }
  {
    simdjson::fractured_json_options opts;
    opts.max_inline_complexity = 3;
    opts.nested_bracket_padding = false;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted, "{\"a\": [ 1, 2 ]}");
  }
  {
    simdjson::fractured_json_options opts;
    opts.max_inline_complexity = 3;
    opts.simple_bracket_padding = false;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted, "{ \"a\": [1, 2] }");
  }

  std::cout << "Nested bracket padding test passed." << std::endl;
  return true;
}

bool always_expand_depth_test() {
  std::cout << "Running " << __func__ << std::endl;

  {
    const char* json = R"({"a":1,"b":2})";
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    auto error = parser.parse(json, strlen(json)).get(doc);
    if (error) {
      std::cerr << "Parse error: " << error << std::endl;
      return false;
    }

    simdjson::fractured_json_options opts;
    opts.always_expand_depth = 0;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted, "{\n    \"a\": 1,\n    \"b\": 2\n}");
  }
  {
    // A uniform array at the root is still table-formatted, not exploded
    // into one fully-expanded object per row.
    const char* json = R"([{"id":1,"name":"Alice"},{"id":2,"name":"Bob"},{"id":3,"name":"Carol"}])";
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    auto error = parser.parse(json, strlen(json)).get(doc);
    if (error) {
      std::cerr << "Parse error: " << error << std::endl;
      return false;
    }

    simdjson::fractured_json_options opts;
    opts.always_expand_depth = 0;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "[\n"
        "    { \"id\": 1, \"name\": \"Alice\" },\n"
        "    { \"id\": 2, \"name\": \"Bob\"   },\n"
        "    { \"id\": 3, \"name\": \"Carol\" }\n"
        "]");
  }
  {
    // depth 1 forces the root's direct children to expand too, even
    // though "a"'s value alone would easily fit inline.
    const char* json = R"({"a":{"x":1,"y":2}})";
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    auto error = parser.parse(json, strlen(json)).get(doc);
    if (error) {
      std::cerr << "Parse error: " << error << std::endl;
      return false;
    }

    simdjson::fractured_json_options opts;
    opts.always_expand_depth = 1;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "{\n"
        "    \"a\": {\n"
        "        \"x\": 1,\n"
        "        \"y\": 2\n"
        "    }\n"
        "}");
  }

  std::cout << "Always expand depth test passed." << std::endl;
  return true;
}


bool table_format_union_columns_with_blank_padding_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({"test":[{"test":1,"test3":2,"test2":2},{"test":1,"test2":3,"test4":5}]})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 70; // no single row but fit table

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted:\n" << formatted << std::endl;

  const char* expected =
      "{\n"
      "    \"test\": [\n"
      "        { \"test\": 1, \"test3\": 2, \"test2\": 2             },\n"
      "        { \"test\": 1,             \"test2\": 3, \"test4\": 5 }\n"
      "    ]\n"
      "}";
  ASSERT_EQUAL(formatted, expected);

  std::cout << "Table format union columns with blank padding test passed." << std::endl;
  return true;
}

bool table_format_respects_max_total_line_length_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([{"a":1,"b":2,"c":3},{"a":4,"b":5,"c":6},{"a":7,"b":8,"c":9}])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.max_total_line_length = 15; // too small for a 3-column table row

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted:\n" << formatted << std::endl;

  const char* expected =
      "[\n"
      "    {\n"
      "        \"a\": 1,\n"
      "        \"b\": 2,\n"
      "        \"c\": 3\n"
      "    },\n"
      "    {\n"
      "        \"a\": 4,\n"
      "        \"b\": 5,\n"
      "        \"c\": 6\n"
      "    },\n"
      "    {\n"
      "        \"a\": 7,\n"
      "        \"b\": 8,\n"
      "        \"c\": 9\n"
      "    }\n"
      "]";
  ASSERT_EQUAL(formatted, expected);

  std::cout << "Table format max total line length test passed." << std::endl;
  return true;
}

bool table_nested_cell_padding_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([{"a":{"x":1}},{"a":{"x":22}}])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;

  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted:\n" << formatted << std::endl;

  // "a" is a uniform object column, so its "x" sub-column is aligned recursively
  const char* expected =
      "[\n"
      "    { \"a\": { \"x\": 1  } },\n"
      "    { \"a\": { \"x\": 22 } }\n"
      "]";
  ASSERT_EQUAL(formatted, expected);

  std::cout << "Table nested cell padding test passed." << std::endl;
  return true;
}

bool table_row_bracket_padding_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([{"a":{"x":1}},{"a":{"x":22}}])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  {
    simdjson::fractured_json_options opts;
    opts.simple_bracket_padding = true;
    opts.nested_bracket_padding = false;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "[\n"
        "    {\"a\": { \"x\": 1  }},\n"
        "    {\"a\": { \"x\": 22 }}\n"
        "]");
  }
  {
    simdjson::fractured_json_options opts;
    opts.simple_bracket_padding = false;
    opts.nested_bracket_padding = true;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "[\n"
        "    { \"a\": {\"x\": 1 } },\n"
        "    { \"a\": {\"x\": 22} }\n"
        "]");
  }

  std::cout << "Table row bracket padding test passed." << std::endl;
  return true;
}

bool table_row_bracket_padding_consistent_across_ragged_rows_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([{"a":{"x":1},"b":1},{"b":2}])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  simdjson::fractured_json_options opts;
  opts.simple_bracket_padding = false;
  opts.nested_bracket_padding = true;
  auto formatted = simdjson::fractured_json(doc, opts);
  std::cout << "Formatted:\n" << formatted << std::endl;

  // Row 2 lacks "a" entirely  but it still must use nested_bracket_padding
  ASSERT_EQUAL(formatted,
      "[\n"
      "    { \"a\": {\"x\": 1}, \"b\": 1 },\n"
      "    {                \"b\": 2 }\n"
      "]");

  std::cout << "Table row bracket padding consistent across ragged rows test passed." << std::endl;
  return true;
}

bool table_format_prunes_recursive_columns_to_fit_width_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json =
      R"({"test":[{"test1":{"test1":1},"test2":{"test1":1}},)"
      R"({"test1":{"test2":1},"test2":{"test2":1}}]})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  {
    // Not enough room for recursive alignment so keep inner parts compact
    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 70;
    opts.max_table_row_complexity = 10;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "{\n"
        "    \"test\": [\n"
        "        { \"test1\": { \"test1\": 1 }, \"test2\": { \"test1\": 1 } },\n"
        "        { \"test1\": { \"test2\": 1 }, \"test2\": { \"test2\": 1 } }\n"
        "    ]\n"
        "}");
  }
  {
    // Enough room for full recursive alignment
    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 90;
    opts.max_table_row_complexity = 10;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "{\n"
        "    \"test\": [\n"
        "        { \"test1\": { \"test1\": 1             }, \"test2\": { \"test1\": 1             } },\n"
        "        { \"test1\": {             \"test2\": 1 }, \"test2\": {             \"test2\": 1 } }\n"
        "    ]\n"
        "}");
  }

  std::cout << "Table format prunes recursive columns to fit width test passed." << std::endl;
  return true;
}

bool table_format_prunes_deepest_column_only_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json =
      R"([{"a":{"b":{"c":1}},"b":{"c":1}},{"a":{"b":{"d":1}},"b":{"d":1}}])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  {
    // Room for full recursive alignment of both "a.b" and "b".
    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 70;
    opts.max_table_row_complexity = 10;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "[\n"
        "    { \"a\": { \"b\": { \"c\": 1         } }, \"b\": { \"c\": 1         } },\n"
        "    { \"a\": { \"b\": {         \"d\": 1 } }, \"b\": {         \"d\": 1 } }\n"
        "]");
  }
  {
    // Not enough room for both, but enough for one: only "a.b" gets pruned to plain;
    // "b" stays recursively aligned.
    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 63;
    opts.max_table_row_complexity = 10;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "[\n"
        "    { \"a\": { \"b\": { \"c\": 1 } }, \"b\": { \"c\": 1         } },\n"
        "    { \"a\": { \"b\": { \"d\": 1 } }, \"b\": {         \"d\": 1 } }\n"
        "]");
  }
  {
    // Not enough room for either: both columns end up pruned to plain.
    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 50;
    opts.max_table_row_complexity = 10;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "[\n"
        "    { \"a\": { \"b\": { \"c\": 1 } }, \"b\": { \"c\": 1 } },\n"
        "    { \"a\": { \"b\": { \"d\": 1 } }, \"b\": { \"d\": 1 } }\n"
        "]");
  }

  std::cout << "Table format prunes deepest column only test passed." << std::endl;
  return true;
}

bool compact_multiline_aligned_packing_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json =
      R"([{"id":1,"name":"Alice"},{"id":2,"name":"Bob"},{"id":3,"name":"Carol"},)"
      R"({"id":4,"name":"Dave"},{"id":5,"name":"Eve"}])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  {
    // Not enough room for 3 aligned rows on one line so falls back to table
    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 40;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "[\n"
        "    { \"id\": 1, \"name\": \"Alice\" },\n"
        "    { \"id\": 2, \"name\": \"Bob\"   },\n"
        "    { \"id\": 3, \"name\": \"Carol\" },\n"
        "    { \"id\": 4, \"name\": \"Dave\"  },\n"
        "    { \"id\": 5, \"name\": \"Eve\"   }\n"
        "]");
  }
  {
    // Enough room for 3+ aligned rows per line. Packs them, wrapping to a
    // new line only when the next row wouldn't fit
    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 120;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "[\n"
        "    { \"id\": 1, \"name\": \"Alice\" }, { \"id\": 2, \"name\": \"Bob\"   }, { \"id\": 3, \"name\": \"Carol\" },\n"
        "    { \"id\": 4, \"name\": \"Dave\"  }, { \"id\": 5, \"name\": \"Eve\"   }\n"
        "]");
  }

  std::cout << "Compact multiline aligned packing test passed." << std::endl;
  return true;
}

bool compact_multiline_scalar_list_alignment_test() {
  std::cout << "Running " << __func__ << std::endl;

  {
    // Numbers (with null tolerated)
    const char* json = R"([1, 10, 4, 10, 10, 10, 10, 10, 10, 1.111, null])";
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    auto error = parser.parse(json, strlen(json)).get(doc);
    if (error) {
      std::cerr << "Parse error: " << error << std::endl;
      return false;
    }

    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 40;
    opts.simple_bracket_padding = false;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "[\n"
        "    1    , 10   , 4    , 10   , 10   ,\n"
        "    10   , 10   , 10   , 10   , 1.111,\n"
        "    null \n"
        "]");
  }
  {
    // Strings (with null tolerated)
    const char* json = R"(["a", "bb", null])";
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    auto error = parser.parse(json, strlen(json)).get(doc);
    if (error) {
      std::cerr << "Parse error: " << error << std::endl;
      return false;
    }

    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 15;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "[\n"
        "    \"a\" ,\n"
        "    \"bb\",\n"
        "    null\n"
        "]");
  }
  {
    // mixed not aligned
    const char* json = R"(["a", 1, "bb", 22])";
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    auto error = parser.parse(json, strlen(json)).get(doc);
    if (error) {
      std::cerr << "Parse error: " << error << std::endl;
      return false;
    }

    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 15;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "[\n"
        "    \"a\",\n"
        "    1,\n"
        "    \"bb\",\n"
        "    22\n"
        "]");
  }

  std::cout << "Compact multiline scalar list alignment test passed." << std::endl;
  return true;
}

bool table_format_array_rows_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"({"test":[[1,10],[1,10],[1,10],[10,1],["test",1]]})";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  {
    // Table format: one row per line, columns aligned by position.
    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 50;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "{\n"
        "    \"test\": [\n"
        "        [ 1     , 10 ],\n"
        "        [ 1     , 10 ],\n"
        "        [ 1     , 10 ],\n"
        "        [ 10    , 1  ],\n"
        "        [ \"test\", 1  ]\n"
        "    ]\n"
        "}");
  }
  {
    // Enough room to pack several aligned rows per line.
    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 64;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "{\n"
        "    \"test\": [\n"
        "        [ 1     , 10 ], [ 1     , 10 ], [ 1     , 10 ],\n"
        "        [ 10    , 1  ], [ \"test\", 1  ]\n"
        "    ]\n"
        "}");
  }

  {
    // Enough room to pack all rows in one line
    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 73;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "{\n"
        "    \"test\": [ [ 1, 10 ], [ 1, 10 ], [ 1, 10 ], [ 10, 1 ], [ \"test\", 1 ] ]\n"
        "}");
  }

  std::cout << "Table format array rows test passed." << std::endl;
  return true;
}

bool table_format_array_rows_ragged_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([[1,2,33],[1,2]])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  {
    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 17;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "[\n"
        "    [ 1, 2, 33 ],\n"
        "    [ 1, 2     ]\n"
        "]");
  }

  {
    simdjson::fractured_json_options opts;
    opts.max_total_line_length = 26;
    auto formatted = simdjson::fractured_json(doc, opts);
    ASSERT_EQUAL(formatted,
        "[ [ 1, 2, 33 ], [ 1, 2 ] ]");
  }

  std::cout << "Table format array rows ragged test passed." << std::endl;
  return true;
}

bool table_format_respects_max_table_row_complexity_test() {
  std::cout << "Running " << __func__ << std::endl;

  const char* json = R"([{"a":{"b":{"x":1}}},{"a":{"b":{"x":2}}},{"a":{"b":{"x":3}}}])";

  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json, strlen(json)).get(doc);
  if (error) {
    std::cerr << "Parse error: " << error << std::endl;
    return false;
  }

  {
    simdjson::fractured_json_options opts;
    opts.max_table_row_complexity = 2;

    auto formatted = simdjson::fractured_json(doc, opts);
    std::cout << "Formatted:\n" << formatted << std::endl;

    const char* expected =
        "[\n"
        "    {\n"
        "        \"a\": { \"b\": { \"x\": 1 } }\n"
        "    },\n"
        "    {\n"
        "        \"a\": { \"b\": { \"x\": 2 } }\n"
        "    },\n"
        "    {\n"
        "        \"a\": { \"b\": { \"x\": 3 } }\n"
        "    }\n"
        "]";
    ASSERT_EQUAL(formatted, expected);
  }

  {
    simdjson::fractured_json_options opts;
    opts.max_table_row_complexity = 3;

    auto formatted = simdjson::fractured_json(doc, opts);
    std::cout << "Formatted:\n" << formatted << std::endl;

    const char* expected =
        "[\n"
        "    { \"a\": { \"b\": { \"x\": 1 } } },\n"
        "    { \"a\": { \"b\": { \"x\": 2 } } },\n"
        "    { \"a\": { \"b\": { \"x\": 3 } } }\n"
        "]";
    ASSERT_EQUAL(formatted, expected);
  }

  std::cout << "Table format max table row complexity test passed." << std::endl;
  return true;
}

int main() {
  bool success = true;

  // Original tests
  success = roundtrip_test() && success;
  success = inline_array_test() && success;
  success = inline_object_test() && success;
  success = expanded_test() && success;
  success = compact_multiline_test() && success;
  success = table_format_test() && success;
  success = empty_containers_test() && success;
  success = scalar_types_test() && success;
  success = string_escaping_test() && success;
  success = custom_options_test() && success;
  success = deep_nesting_test() && success;
  success = mixed_array_test() && success;
  success = string_api_test() && success;

  // Edge case tests
  success = unicode_test() && success;
  success = boundary_numbers_test() && success;
  success = nested_arrays_test() && success;
  success = empty_string_test() && success;
  success = special_key_test() && success;
  success = non_uniform_array_test() && success;
  success = long_string_test() && success;
  success = large_array_test() && success;
  success = reflection_workflow_test() && success;
  success = control_characters_test() && success;
  success = single_element_test() && success;

  // Option tests
  success = disable_compact_test() && success;
  success = disable_table_test() && success;
  success = no_padding_test() && success;

  // Layout tests
  success = compact_multiline_oversized_item_test() && success;
  success = compact_multiline_respects_max_total_line_length_test() && success;
  success = bracket_padding_counted_in_object_length_test() && success;
  success = bracket_padding_counted_in_array_length_test() && success;
  success = compact_multiline_breaks_after_expanded_item_test() && success;
  success = nested_object_respects_max_total_line_length_test() && success;
  success = nested_array_respects_max_total_line_length_test() && success;
  success = compact_multiline_item_respects_max_total_line_length_test() && success;
  success = trailing_comma_counted_in_length_test() && success;
  success = nested_bracket_padding_test() && success;
  success = always_expand_depth_test() && success;
  success = table_format_union_columns_with_blank_padding_test() && success;
  success = table_format_respects_max_total_line_length_test() && success;
  success = table_nested_cell_padding_test() && success;
  success = table_row_bracket_padding_test() && success;
  success = table_row_bracket_padding_consistent_across_ragged_rows_test() && success;
  success = table_format_prunes_recursive_columns_to_fit_width_test() && success;
  success = table_format_prunes_deepest_column_only_test() && success;
  success = table_format_array_rows_test() && success;
  success = table_format_array_rows_ragged_test() && success;
  success = compact_multiline_aligned_packing_test() && success;
  success = compact_multiline_scalar_list_alignment_test() && success;
  success = table_format_respects_max_table_row_complexity_test() && success;

  if (success) {
    std::cout << "\nAll fractured_json tests passed! (" << 50 << " tests)" << std::endl;
    return EXIT_SUCCESS;
  } else {
    std::cerr << "\nSome tests failed!" << std::endl;
    return EXIT_FAILURE;
  }
}
