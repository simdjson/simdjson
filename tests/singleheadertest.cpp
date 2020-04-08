// This file is not part of our main, regular tests.
#include "../singleheader/simdjson.h"
#include <iostream>

using namespace simdjson;

int main() {
  const char *filename = SIMDJSON_BENCHMARK_DATA_DIR "/twitter.json";
  padded_string p = get_corpus(filename);
  dom::parser parser;
  auto [doc, error] = parser.parse(p);
  if(error) {
    std::cerr << error << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
