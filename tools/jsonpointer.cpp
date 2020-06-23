#include "simdjson.h"
#include <iostream>


int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <jsonfile> <jsonpath>" << std::endl;
    std::cerr << "Follows the rfc6901 standard's syntax: "
                 "https://tools.ietf.org/html/rfc6901"
              << std::endl;
    std::cerr << " Example: " << argv[0]
              << " jsonexamples/small/demo.json /Image/Width /Image/Height "
                 "/Image/IDs/2 "
              << std::endl;
    std::cerr << "Multiple <jsonpath> can be issued in the same command, but "
                 "at least one is needed."
              << std::endl;
    exit(1);
  }

  const char *filename = argv[1];
  simdjson::dom::parser parser;
  simdjson::dom::element doc;
  auto error = parser.load(filename).get(doc);
  if (error) { std::cerr << "Error parsing " << filename << ": " << error << std::endl; }

  std::cout << "[" << std::endl;
  for (int idx = 2; idx < argc; idx++) {
    const char *json_pointer = argv[idx];
    simdjson::dom::element value;
    std::cout << "{\"jsonpath\": \"" << json_pointer << "\"";
    if ((error = doc[json_pointer].get(value))) {
      std::cout << ",\"error\":\"" << error << "\"";
    } else {
      std::cout << ",\"value\":" << value;
    }
    std::cout << "}";
    if (idx + 1 < argc) {
      std::cout << ",";
    }
    std::cout << std::endl;
  }
  std::cout << "]" << std::endl;
  return EXIT_SUCCESS;
}
