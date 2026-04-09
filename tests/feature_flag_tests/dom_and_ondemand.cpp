// Test: Both DOM and OnDemand APIs (Builder disabled)
// Feature flags set via target_compile_definitions in CMakeLists.txt
#include "simdjson.h"
#include <iostream>

int main() {
  auto json = R"({"key": "value", "number": 42})"_padded;

  // Use DOM API
  simdjson::dom::parser dom_parser;
  simdjson::dom::element dom_doc;
  auto error = dom_parser.parse(json).get(dom_doc);
  if (error) {
    std::cerr << "DOM parse error: " << error << std::endl;
    return EXIT_FAILURE;
  }
  std::string_view dom_val;
  error = dom_doc["key"].get_string().get(dom_val);
  if (error || dom_val != "value") {
    std::cerr << "DOM key lookup failed" << std::endl;
    return EXIT_FAILURE;
  }

  // Use OnDemand API
  simdjson::ondemand::parser od_parser;
  simdjson::ondemand::document od_doc;
  error = od_parser.iterate(json).get(od_doc);
  if (error) {
    std::cerr << "OnDemand parse error: " << error << std::endl;
    return EXIT_FAILURE;
  }
  std::string_view od_val;
  error = od_doc["key"].get_string().get(od_val);
  if (error || od_val != "value") {
    std::cerr << "OnDemand key lookup failed" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "DOM+OnDemand test passed." << std::endl;
  return EXIT_SUCCESS;
}
