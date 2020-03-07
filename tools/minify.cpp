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
  simdjson::json_minify(p, p.data());
  printf("%s", p.data());
}
