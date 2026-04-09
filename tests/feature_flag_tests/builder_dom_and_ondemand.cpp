// Test: All three APIs enabled (Builder + DOM + OnDemand)
// Feature flags set via target_compile_definitions in CMakeLists.txt
#include "simdjson.h"
#include <iostream>

int main() {
  auto json = R"({"name": "simdjson", "version": 5})"_padded;

  // Use DOM API
  simdjson::dom::parser dom_parser;
  simdjson::dom::element dom_doc;
  auto error = dom_parser.parse(json).get(dom_doc);
  if (error) {
    std::cerr << "DOM parse error: " << error << std::endl;
    return EXIT_FAILURE;
  }
  std::string_view name;
  error = dom_doc["name"].get_string().get(name);
  if (error || name != "simdjson") {
    std::cerr << "DOM name lookup failed" << std::endl;
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
  std::string_view od_name;
  error = od_doc["name"].get_string().get(od_name);
  if (error || od_name != "simdjson") {
    std::cerr << "OnDemand name lookup failed" << std::endl;
    return EXIT_FAILURE;
  }

  // Use Builder API
  simdjson::builder::string_builder sb;
  sb.start_object();
  sb.append_key_value("name", "simdjson");
  sb.append_comma();
  sb.append_key_value("version", 5);
  sb.end_object();
  std::string_view result;
  error = sb.view().get(result);
  if (error) {
    std::cerr << "Builder error: " << error << std::endl;
    return EXIT_FAILURE;
  }
  if (result != R"({"name":"simdjson","version":5})") {
    std::cerr << "Builder output mismatch: " << result << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Builder+DOM+OnDemand test passed." << std::endl;
  return EXIT_SUCCESS;
}
