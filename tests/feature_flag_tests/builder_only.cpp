// Test: Builder API only (OnDemand disabled, DOM enabled since Builder depends on it)
// Feature flags set via target_compile_definitions in CMakeLists.txt
#include "simdjson.h"
#include <iostream>

int main() {
  // Use Builder API
  simdjson::builder::string_builder sb;
  sb.start_object();
  sb.append_key_value("count", 2);
  sb.append_comma();
  sb.append_key_value("name", "test");
  sb.end_object();
  std::string_view result;
  auto error = sb.view().get(result);
  if (error) {
    std::cerr << "Builder error: " << error << std::endl;
    return EXIT_FAILURE;
  }
  if (result != R"({"count":2,"name":"test"})") {
    std::cerr << "Builder output mismatch: " << result << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Builder-only test passed." << std::endl;
  return EXIT_SUCCESS;
}
