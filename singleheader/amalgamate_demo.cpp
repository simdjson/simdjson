/* auto-generated on Tue 19 May 2020 17:35:03 EDT. Do not edit! */

#include <iostream>
#include "simdjson.h"
#include "simdjson.cpp"
int main(int argc, char *argv[]) {
  if(argc < 2) {
    std::cerr << "Please specify at least one file name. " << std::endl;
  }
  const char * filename = argv[1];
  simdjson::dom::parser parser;
  simdjson::error_code error;
  UNUSED simdjson::dom::element elem;
  parser.load(filename).tie(elem, error); // do the parsing
  if (error) {
    std::cout << "parse failed" << std::endl;
    std::cout << "error code: " << error << std::endl;
    std::cout << error << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cout << "parse valid" << std::endl;
  }
  if(argc == 2) {
    return EXIT_SUCCESS;
  }

  // parse_many
  const char * filename2 = argv[2];
  for (auto result : parser.load_many(filename2)) {
    error = result.error();
  }
  if (error) {
    std::cout << "parse_many failed" << std::endl;
    std::cout << "error code: " << error << std::endl;
    std::cout << error << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cout << "parse_many valid" << std::endl;
  }
  return EXIT_SUCCESS;
}

