#include <iostream>
#include "../singleheader/simdjson.h"

int main() {
  const char * filename = JSON_TEST_PATH; 
  padded_string p = get_corpus(filename);
  bool automated_reallocation = false; 
  ParsedJson pj = build_parsed_json(p, automated_reallocation); // do the parsing
  if( ! pj.isValid() ) {
    return EXIT_FAILURE;
  }
  if( ! pj.allocateCapacity(p.size()) ) {
    return EXIT_FAILURE;
  }
  automated_reallocation = false;
  const int res = json_parse(p, pj, automated_reallocation);
  if (res) {
    std::cerr << simdjson::errorMsg(res) << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}