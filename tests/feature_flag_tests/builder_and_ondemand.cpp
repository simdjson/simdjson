// Test: Builder + OnDemand APIs (DOM also enabled since Builder depends on it)
// Feature flags set via target_compile_definitions in CMakeLists.txt
#include "simdjson.h"
#include <iostream>

int main() {
  // Use OnDemand API
  auto json = R"({"key": "value"})"_padded;
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  auto error = parser.iterate(json).get(doc);
  if (error) {
    std::cerr << "OnDemand parse error: " << error << std::endl;
    return EXIT_FAILURE;
  }
  std::string_view val;
  error = doc["key"].get_string().get(val);
  if (error || val != "value") {
    std::cerr << "OnDemand key lookup failed" << std::endl;
    return EXIT_FAILURE;
  }

  // Use Builder API
  simdjson::builder::string_builder sb;
  sb.start_array();
  sb.append(1);
  sb.append_comma();
  sb.append(2);
  sb.append_comma();
  sb.append(3);
  sb.end_array();
  std::string_view result;
  error = sb.view().get(result);
  if (error) {
    std::cerr << "Builder error: " << error << std::endl;
    return EXIT_FAILURE;
  }
  if (result != "[1,2,3]") {
    std::cerr << "Builder output mismatch: " << result << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Builder+OnDemand test passed." << std::endl;
  return EXIT_SUCCESS;
}
