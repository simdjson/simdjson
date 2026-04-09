// Test: Builder + DOM APIs (OnDemand disabled)
// Feature flags set via target_compile_definitions in CMakeLists.txt
#include "simdjson.h"
#include <iostream>

int main() {
  auto json = R"({"key": "value"})"_padded;

  // Use DOM API
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.parse(json).get(doc);
  if (error) {
    std::cerr << "DOM parse error: " << error << std::endl;
    return EXIT_FAILURE;
  }
  std::string_view val;
  error = doc["key"].get_string().get(val);
  if (error || val != "value") {
    std::cerr << "DOM key lookup failed" << std::endl;
    return EXIT_FAILURE;
  }

  // Use Builder API
  simdjson::builder::string_builder sb;
  sb.start_object();
  sb.append_key_value("greeting", "hello");
  sb.end_object();
  std::string_view result;
  error = sb.view().get(result);
  if (error) {
    std::cerr << "Builder error: " << error << std::endl;
    return EXIT_FAILURE;
  }
  if (result != R"({"greeting":"hello"})") {
    std::cerr << "Builder output mismatch: " << result << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Builder+DOM test passed." << std::endl;
  return EXIT_SUCCESS;
}
