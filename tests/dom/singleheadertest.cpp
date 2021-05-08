// This file is not part of our main, regular tests.
#include "../singleheader/simdjson.h"
#include <iostream>

using namespace simdjson;

int main() {
  const char *filename = SIMDJSON_BENCHMARK_DATA_DIR "/twitter.json";
  dom::parser parser;
  dom::element doc;
  auto error = parser.load(filename).get(doc);
  if (error) {
    std::cerr << error << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << doc << std::endl;
  return EXIT_SUCCESS;
}
