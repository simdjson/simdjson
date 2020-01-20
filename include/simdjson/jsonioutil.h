#ifndef SIMDJSON_JSONIOUTIL_H
#define SIMDJSON_JSONIOUTIL_H

#include "simdjson/common_defs.h"
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "simdjson/padded_string.h"

namespace simdjson {

// load a file in memory...
// get a corpus; pad out to cache line so we can always use SIMD
// throws exceptions in case of failure
// first element of the pair is a string (null terminated)
// whereas the second element is the length.
//
// throws an exception if the file cannot be opened, use try/catch
//
//      padded_string p{} ;
//      try {
//        p = get_corpus(filename);
//      } catch (const std::exception& e) {
//          p.~padded_string() ;
//        std::cout << "Could not load the file " << filename << std::endl;
//      }
inline padded_string get_corpus(std::string_view filename) {
  using namespace std;

  ifstream ifs_(filename.data());
  string str_(istreambuf_iterator<char>{ifs_}, {});

  return padded_string{str_};
}

} // namespace simdjson

#endif
