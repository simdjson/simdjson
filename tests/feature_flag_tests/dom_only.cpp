// Test: DOM API only (OnDemand and Builder disabled)
// Feature flags set via target_compile_definitions in CMakeLists.txt
#include "simdjson.h"
#include <iostream>

int main() {
  simdjson::dom::parser parser;
  auto json = R"({"key": "value", "number": 42})"_padded;
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
  int64_t num;
  error = doc["number"].get_int64().get(num);
  if (error || num != 42) {
    std::cerr << "DOM number lookup failed" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "DOM-only test passed." << std::endl;
  return EXIT_SUCCESS;
}
