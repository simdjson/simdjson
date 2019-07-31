#include <iostream>

#include "simdjson/jsonioutil.h"
#include "simdjson/jsonminifier.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <jsonfile>\n";
    exit(1);
  }
  simdjson::padded_string p;
  std::string filename = argv[argc - 1];
  try {
    simdjson::get_corpus(filename).swap(p);
  } catch (const std::exception &) {
    std::cout << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  simdjson::json_minify(p, p.data());
  printf("%s", p.data());
}
