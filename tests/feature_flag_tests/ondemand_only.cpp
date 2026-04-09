// Test: OnDemand API only (Builder disabled)
// Note: DOM must remain enabled because the builtin infrastructure
// (SIMDJSON_BUILTIN_IMPLEMENTATION_IS) is currently included via the DOM path.
// Feature flags set via target_compile_definitions in CMakeLists.txt
#include "simdjson.h"
#include <iostream>

int main() {
  simdjson::ondemand::parser parser;
  auto json = R"({"key": "value", "number": 42})"_padded;
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
  std::cout << "OnDemand-only test passed." << std::endl;
  return EXIT_SUCCESS;
}
