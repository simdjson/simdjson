#include <iostream>
#include <unistd.h>

#include "simdjson/jsonioutil.h"
#include "simdjson/jsonparser.h"

using namespace std;

void compute_dump(ParsedJson::iterator &pjh) {
  bool inobject = (pjh.get_type() == '{');
  bool inarray = (pjh.get_type() == '[');
  if ((!inobject) && (!inarray)) {
    pjh.print(std::cout); // just print the lone value
    return; // we are done
  }
  // we have either an array or an object
  bool goingdown = pjh.down();
  if(!goingdown) {
      // we have an empty scope
      if(inobject) std::cout<<"{}";
      else std::cout<<"[]";
      return;
  }
  // we have a non-empty scope and we are at the beginning of it
  if (inobject) {
    std::cout << "{";
    assert(pjh.get_type() == '"');
    pjh.print(std::cout); // must be a string
    std::cout << ":";
    assert(pjh.next());
    compute_dump(pjh); // let us recurse
    while (pjh.next()) {
      std::cout << ",";
      assert(pjh.get_type() == '"');
      pjh.print(std::cout);
      std::cout << ":";
      assert(pjh.next());
      compute_dump(pjh); // let us recurse
    }
    std::cout << "}";
  } else {
    std::cout << "[";
    compute_dump(pjh); // let us recurse
    while (pjh.next()) {
      std::cout << ",";
      compute_dump(pjh); // let us recurse
    }
    std::cout << "]";
  }
  assert(pjh.up()); 
}

int main(int argc, char *argv[]) {
  int c;
  bool rawdump = false;
  bool apidump = false;

  while ((c = getopt(argc, argv, "da")) != -1)
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
  if (optind >= argc) {
    cerr << "Reads json in, out the result of the parsing. " << endl;
    cerr << "Usage: " << argv[0] << " <jsonfile>" << endl;
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
  bool is_ok = json_parse(p, pj); // do the parsing, return false on error
  if (!is_ok) {
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
    is_ok = rawdump ? pj.dump_raw_tape(std::cout) : pj.printjson(std::cout);
    if (!is_ok) {
      std::cerr << " Could not print out parsed result. " << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
