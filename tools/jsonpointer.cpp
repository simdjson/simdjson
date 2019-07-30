#include "simdjson/jsonioutil.h"
#include "simdjson/jsonparser.h"
#include <iostream>

void compute_dump(simdjson::ParsedJson::Iterator &pjh) {
  if (pjh.is_object()) {
    std::cout << "{";
    if (pjh.down()) {
      pjh.print(std::cout); // must be a string
      std::cout << ":";
      pjh.next();
      compute_dump(pjh); // let us recurse
      while (pjh.next()) {
        std::cout << ",";
        pjh.print(std::cout);
        std::cout << ":";
        pjh.next();
        compute_dump(pjh); // let us recurse
      }
      pjh.up();
    }
    std::cout << "}";
  } else if (pjh.is_array()) {
    std::cout << "[";
    if (pjh.down()) {
      compute_dump(pjh); // let us recurse
      while (pjh.next()) {
        std::cout << ",";
        compute_dump(pjh); // let us recurse
      }
      pjh.up();
    }
    std::cout << "]";
  } else {
    pjh.print(std::cout); // just print the lone value
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
  simdjson::ParsedJson pj;
  bool allocok = pj.allocate_capacity(p.size(), 1024);
  if (!allocok) {
    std::cerr << "failed to allocate memory" << std::endl;
    return EXIT_FAILURE;
  }
  int res =
      simdjson::json_parse(p, pj); // do the parsing, return false on error
  if (res) {
    std::cerr << " Parsing failed with error " << simdjson::error_message(res)
              << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "[" << std::endl;
  for (int idx = 2; idx < argc; idx++) {
    const char *jsonpath = argv[idx];
    simdjson::ParsedJson::Iterator it(pj);
    if (it.move_to(std::string(jsonpath))) {
      std::cout << "{\"jsonpath\": \"" << jsonpath << "\"," << std::endl;
      std::cout << "\"value\":";
      compute_dump(it);
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
