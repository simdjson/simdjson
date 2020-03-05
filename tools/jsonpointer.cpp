#include "simdjson.h"
#include <iostream>

void compute_dump(simdjson::document::iterator &iter) {
  if (iter.is_object()) {
    std::cout << "{";
    if (iter.down()) {
      iter.print(std::cout); // must be a string
      std::cout << ":";
      iter.next();
      compute_dump(iter); // let us recurse
      while (iter.next()) {
        std::cout << ",";
        iter.print(std::cout);
        std::cout << ":";
        iter.next();
        compute_dump(iter); // let us recurse
      }
      iter.up();
    }
    std::cout << "}";
  } else if (iter.is_array()) {
    std::cout << "[";
    if (iter.down()) {
      compute_dump(iter); // let us recurse
      while (iter.next()) {
        std::cout << ",";
        compute_dump(iter); // let us recurse
      }
      iter.up();
    }
    std::cout << "]";
  } else {
    iter.print(std::cout); // just print the lone value
  }
}

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
  simdjson::padded_string p;
  try {
    simdjson::get_corpus(filename).swap(p);
  } catch (const std::exception &e) { // caught by reference to base
    std::cout << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  simdjson::document::parser parser;
  bool allocok = parser.allocate_capacity(p.size(), 1024);
  if (!allocok) {
    std::cerr << "failed to allocate memory" << std::endl;
    return EXIT_FAILURE;
  }
  auto [doc, error] = parser.parse(p); // do the parsing
  if (error) {
    std::cerr << " Parsing failed with error " << error
              << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "[" << std::endl;
  for (int idx = 2; idx < argc; idx++) {
    const char *jsonpath = argv[idx];
    simdjson::document::iterator iter(parser);
    if (iter.move_to(std::string(jsonpath))) {
      std::cout << "{\"jsonpath\": \"" << jsonpath << "\"," << std::endl;
      std::cout << "\"value\":";
      compute_dump(iter);
      std::cout << "}" << std::endl;
    } else {
      std::cout << "null" << std::endl;
    }
    if (idx + 1 < argc) {
      std::cout << "," << std::endl;
    }
  }
  std::cout << "]" << std::endl;
  return EXIT_SUCCESS;
}
