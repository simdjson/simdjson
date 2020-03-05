/* auto-generated on Thu Mar  5 15:35:24 PST 2020. Do not edit! */

#include <iostream>
#include "simdjson.h"
#include "simdjson.cpp"
int main(int argc, char *argv[]) {
  if(argc < 2) {
    std::cerr << "Please specify at least one file name. " << std::endl;
  }
  const char * filename = argv[1];
  simdjson::padded_string p = simdjson::get_corpus(filename);
  auto [doc, error] = simdjson::document::parse(p); // do the parsing
  if (error) {
    std::cout << "parse failed" << std::endl;
    std::cout << error << std::endl;
  } else {
    std::cout << "parse valid" << std::endl;
  }
  if(argc == 2) {
    return EXIT_SUCCESS;
  }

  // parse_many
  const char * filename2 = argv[2];
  simdjson::padded_string p2 = simdjson::get_corpus(filename2);
  simdjson::document::parser parser;
  for (auto result : parser.parse_many(p2)) {
    error = result.error;
  }
  if (error) {
    std::cout << "parse_many failed" << std::endl;
    std::cout << error << std::endl;
  } else {
    std::cout << "parse_many valid" << std::endl;
  }
  return EXIT_SUCCESS;
}

