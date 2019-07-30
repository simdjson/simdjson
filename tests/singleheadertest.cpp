#include "../singleheader/simdjson.h"
#include <iostream>

using namespace simdjson;

int main() {
  const char *filename = JSON_TEST_PATH;
  padded_string p = get_corpus(filename);
  ParsedJson pj = build_parsed_json(p); // do the parsing
  if (!pj.is_valid()) {
    return EXIT_FAILURE;
  }
  if (!pj.allocate_capacity(p.size())) {
    return EXIT_FAILURE;
  }
  const int res = json_parse(p, pj);
  if (res) {
    std::cerr << error_message(res) << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
