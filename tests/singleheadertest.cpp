#include <iostream>
#include "../singleheader/simdjson.h"

int main() {
  const char * filename = JSON_TEST_PATH; 
  std::string_view p = get_corpus(filename);
  ParsedJson pj = build_parsed_json(p); // do the parsing
  if( ! pj.isValid() ) {
    return EXIT_FAILURE;
  }
  pj.allocateCapacity(p.size());
  bool is_ok = json_parse(p, pj);
  if (!is_ok) {return EXIT_FAILURE;}
  free((void*)p.data());
  return EXIT_SUCCESS;
}