// This file is not part of our main, regular tests.
#include "../singleheader/simdjson.h"
#include <iostream>

using namespace simdjson;

int main() {
  const char *filename = JSON_TEST_PATH;
  padded_string p = get_corpus(filename);
  document::parser parser;
  auto [pj, errorcode] = parser.parse(p);
  if(errorcode != error_code::SUCCESS) {
    std::cerr << error_message(errorcode) << std::endl;
    return EXIT_FAILURE;
  }
  if(!pj.is_valid()) {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
