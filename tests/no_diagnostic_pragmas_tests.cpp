// Built with SIMDJSON_NO_DIAGNOSTIC_PRAGMAS (and SIMDJSON_DISABLE_DEPRECATED_API)
// so that simdjson does not suppress any compiler diagnostic: the headers must
// still compile without warnings under the strict developer flags (-Werror).
#include "simdjson.h"
#include <cstdlib>
#include <iostream>

int main() {
  simdjson::ondemand::parser parser;
  simdjson::padded_string json = simdjson::padded_string(std::string(R"({"a":1.5,"b":[1,2,3]})"));
  simdjson::ondemand::document doc;
  if (parser.iterate(json).get(doc)) { return EXIT_FAILURE; }
  double a;
  if (doc["a"].get_double().get(a) || a != 1.5) { return EXIT_FAILURE; }
  simdjson::dom::parser dparser;
  simdjson::dom::element elem;
  if (dparser.parse(json).get(elem)) { return EXIT_FAILURE; }
  int64_t b2;
  if (elem.at_pointer("/b/2").get_int64().get(b2) || b2 != 3) { return EXIT_FAILURE; }
  std::cout << "no diagnostic pragmas test passed" << std::endl;
  return EXIT_SUCCESS;
}
