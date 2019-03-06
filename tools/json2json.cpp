#include <iostream>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include "simdjson/jsonioutil.h"
#include "simdjson/jsonparser.h"

using namespace std;

void compute_dump(ParsedJson::iterator &pjh) {
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
    cerr << "Reads json in, out the result of the parsing. " << endl;
    cerr << "Usage: " << argv[0] << " <jsonfile>" << endl;
    cerr << "The -d flag dumps the raw content of the tape." << endl;

    exit(1);
  }
  const char *filename = argv[optind];
  if (optind + 1 < argc) {
    cerr << "warning: ignoring everything after " << argv[optind + 1] << endl;
  }
  std::string_view p;
  try {
    p = get_corpus(filename);
  } catch (const std::exception &e) { // caught by reference to base
    std::cout << "Could not load the file " << filename << std::endl;
    return EXIT_FAILURE;
  }
  ParsedJson pj;
  bool allocok = pj.allocateCapacity(p.size(), 1024);
  if (!allocok) {
    std::cerr << "failed to allocate memory" << std::endl;
    return EXIT_FAILURE;
  }
  int res = json_parse(p, pj); // do the parsing, return false on error
  aligned_free((void *)p.data());
  if (res) {
    std::cerr << " Parsing failed. " << std::endl;
    return EXIT_FAILURE;
  }
  if (apidump) {
    ParsedJson::iterator pjh(pj);
    if (!pjh.isOk()) {
      std::cerr << " Could not iterate parsed result. " << std::endl;
      return EXIT_FAILURE;
    }
    compute_dump(pjh);
  } else {
    const bool is_ok = rawdump ? pj.dump_raw_tape(std::cout) : pj.printjson(std::cout);
    if (!is_ok) {
      std::cerr << " Could not print out parsed result. " << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
