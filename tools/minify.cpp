#include <iostream>

#include "simdjson.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <jsonfile>\n";
    exit(1);
  }
  std::string filename = argv[argc - 1];
  auto [p, error] = simdjson::padded_string::load(filename);
  if (error) {
    std::cerr << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  simdjson::padded_string copy(p.length());
  size_t copy_len;
  error = simdjson::active_implementation->minify((const uint8_t*)p.data(), p.length(), (uint8_t*)copy.data(), copy_len);
  if (error) { std::cerr << error << std::endl; return 1; }
  printf("%s", copy.data());
}
