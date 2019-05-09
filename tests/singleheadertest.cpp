#include <iostream>
#include "../singleheader/simdjson.h"

int main() {
  const char * filename = JSON_TEST_PATH; 
  padded_string p = get_corpus(filename);
  ParsedJson pj = build_parsed_json(p); // do the parsing
  if( ! pj.isValid() ) {
    return EXIT_FAILURE;
  }
  if( ! pj.allocateCapacity(p.size()) ) {
    return EXIT_FAILURE;
  }
  const int res = json_parse(p, pj);
  if (res) {
    std::cerr << simdjson::errorMsg(res) << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
