#include "../singleheader/simdjson.h"
#include <iostream>

int main() {
  const char *filename = JSON_TEST_PATH;
  SimdJson::padded_string p = SimdJson::get_corpus(filename);
  SimdJson::ParsedJson pj = SimdJson::build_parsed_json(p); // do the parsing
  if (!pj.isValid()) {
    return EXIT_FAILURE;
  }
  if (!pj.allocateCapacity(p.size())) {
    return EXIT_FAILURE;
  }
  const int res = json_parse(p, pj);
  if (res) {
    std::cerr << SimdJson::simdjson::errorMsg(res) << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
