#include <iostream>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include "simdjson.h"

void compute_dump(simdjson::document::iterator &iter) {
  if (iter.is_object()) {
    std::cout << "{";
    if (iter.down()) {
      iter.print(std::cout); // must be a string
      std::cout << ":";
      iter.next();
      compute_dump(iter); // let us recurse
      while (iter.next()) {
        std::cout << ",";
        iter.print(std::cout);
        std::cout << ":";
        iter.next();
        compute_dump(iter); // let us recurse
      }
      iter.up();
    }
    std::cout << "}";
  } else if (iter.is_array()) {
    std::cout << "[";
    if (iter.down()) {
      compute_dump(iter); // let us recurse
      while (iter.next()) {
        std::cout << ",";
        compute_dump(iter); // let us recurse
      }
      iter.up();
    }
    std::cout << "]";
  } else {
    iter.print(std::cout); // just print the lone value
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
  simdjson::padded_string p;
  try {
    simdjson::get_corpus(filename).swap(p);
  } catch (const std::exception &) { // caught by reference to base
    std::cout << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  simdjson::document::parser parser;
  bool allocok = parser.allocate_capacity(p.size(), 1024);
  if (!allocok) {
    std::cerr << "failed to allocate memory" << std::endl;
    return EXIT_FAILURE;
  }
  auto [doc, error] = parser.parse(p);
  if (error) {
    std::cerr << " Parsing failed. Error is '" << error
              << "'." << std::endl;
    return EXIT_FAILURE;
  }
  if (apidump) {
    simdjson::document::iterator iter(parser);
    if (!iter.is_ok()) {
      std::cerr << " Could not iterate parsed result. " << std::endl;
      return EXIT_FAILURE;
    }
    compute_dump(iter);
  } else {
    const bool is_ok =
        rawdump ? parser.dump_raw_tape(std::cout) : parser.print_json(std::cout);
    if (!is_ok) {
      std::cerr << " Could not print out parsed result. " << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
