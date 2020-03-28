#include <iostream>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include "simdjson.h"

int main(int argc, char *argv[]) {
  bool rawdump = false;

#ifndef _MSC_VER
  int c;

  while ((c = getopt(argc, argv, "d")) != -1) {
    switch (c) {
    case 'd':
      rawdump = true;
      break;
    default:
      abort();
    }
  }
#else
  int optind = 1;
#endif
  if (optind >= argc) {
    std::cerr << "Reads json in, out the result of the parsing. " << std::endl;
    std::cerr << "Usage: " << argv[0] << " <jsonfile>" << std::endl;
    std::cerr << "The -d flag dumps the raw content of the tape." << std::endl;

    exit(1);
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
