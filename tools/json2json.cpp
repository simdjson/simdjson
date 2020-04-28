#include <iostream>
#include <unistd.h>
#include "simdjson.h"

void usage(const char *exe) {
  std::cerr << exe << " v" << STRINGIFY(SIMDJSON_VERSION) << " (" << simdjson::active_implementation->name() << ")" << std::endl;
  std::cerr << std::endl;
  std::cerr << "Reads json in, out the result of the parsing. " << std::endl;
  std::cerr << "Usage: " << exe << " <jsonfile>" << std::endl;
  std::cerr << "The -d flag dumps the raw content of the tape." << std::endl;
}
int main(int argc, char *argv[]) {
  bool rawdump = false;

  int c;

  while ((c = getopt(argc, argv, "dh")) != -1) {
    switch (c) {
    case 'd':
      rawdump = true;
      break;
    case 'h':
      usage(argv[0]);
      return EXIT_SUCCESS;
    default:
      abort();
    }
  }
  if (optind >= argc) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }
  const char *filename = argv[optind];
  if (optind + 1 < argc) {
    std::cerr << "warning: ignoring everything after " << argv[optind + 1]
              << std::endl;
  }
  simdjson::dom::parser parser;
  auto [doc, error] = parser.load(filename); // do the parsing, return false on error
  if (error != simdjson::SUCCESS) {
    std::cerr << " Parsing failed. Error is '" << simdjson::error_message(error)
              << "'." << std::endl;
    return EXIT_FAILURE;
  }
  if(rawdump) {
    doc.dump_raw_tape(std::cout);
  } else {
    std::cout << doc;
  }
  return EXIT_SUCCESS;
}
