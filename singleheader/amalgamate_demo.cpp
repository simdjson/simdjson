#include <iostream>
#include "simdjson.h"
#include "simdjson.cpp"

int main(int argc, char *argv[]) {
  if(argc < 2) {
    std::cerr << "Please specify at least one file name. " << std::endl;
    return EXIT_FAILURE;
  }
  const char * filename = argv[1];
  simdjson::dom::parser parser;
  simdjson::dom::element elem;
  auto error = parser.load(filename).get(elem); // do the parsing
  if (error) {
    std::cout << "parse failed" << std::endl;
    std::cout << "error code: " << error << std::endl;
    std::cout << error << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cout << "parse valid: " << elem << std::endl;
  }
  if(argc == 2) {
    return EXIT_SUCCESS;
  }

  // parse_many
  const char * filename2 = argv[2];
  simdjson::dom::document_stream stream;
  error = parser.load_many(filename2).get(stream);
  if (!error) {
    for (auto result : stream) {
      error = result.error();
    }
  }
  if (error) {
    std::cout << "parse_many failed" << std::endl;
    std::cout << "error code: " << error << std::endl;
    std::cout << error << std::endl;
    return EXIT_FAILURE;
  } else {
    std::cout << "parse_many valid" << std::endl;
  }
  return EXIT_SUCCESS;
}

