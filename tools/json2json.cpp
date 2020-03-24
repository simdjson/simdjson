#include <iostream>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include "simdjson.h"

void compute_dump(simdjson::ParsedJson::Iterator &pjh) {
  if (pjh.is_object()) {
    std::cout << "{";
    if (pjh.down()) {
      pjh.print(std::cout); // must be a string
      std::cout << ":";
      pjh.next();
      compute_dump(pjh); // let us recurse
      while (pjh.next()) {
        std::cout << ",";
        pjh.print(std::cout);
        std::cout << ":";
        pjh.next();
        compute_dump(pjh); // let us recurse
      }
      pjh.up();
    }
    std::cout << "}";
  } else if (pjh.is_array()) {
    std::cout << "[";
    if (pjh.down()) {
      compute_dump(pjh); // let us recurse
      while (pjh.next()) {
        std::cout << ",";
        compute_dump(pjh); // let us recurse
      }
      pjh.up();
    }
    std::cout << "]";
  } else {
    pjh.print(std::cout); // just print the lone value
  }
}

int main(int argc, char *argv[]) {
  bool rawdump = false;
  bool apidump = false;

#ifndef _MSC_VER
  int c;

  while ((c = getopt(argc, argv, "da")) != -1) {
    switch (c) {
    case 'd':
      rawdump = true;
      break;
    case 'a':
      apidump = true;
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
  simdjson::document::parser parser;
  auto [doc, error] = parser.load(filename); // do the parsing, return false on error
  if (error != simdjson::SUCCESS) {
    std::cerr << " Parsing failed. Error is '" << simdjson::error_message(error)
              << "'." << std::endl;
    return EXIT_FAILURE;
  }
  if (apidump) {
    simdjson::document::iterator i(doc);
    compute_dump(i);
  } else {
    std::cout << doc;
  }
  return EXIT_SUCCESS;
}
