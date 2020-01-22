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
//        std::cout << "Could not load the file " << filename << std::endl;
//      }
inline padded_string get_corpus(std::string_view filename) {
  std::FILE *fp = std::fopen(filename.data(), "rb");
  if (fp != nullptr) {
    if (std::fseek(fp, 0, SEEK_END) < 0) {
      std::fclose(fp);
      throw std::runtime_error("cannot seek in the file");
    }
    long llen = std::ftell(fp);
    if ((llen < 0) || (llen == LONG_MAX)) {
      std::fclose(fp);
      throw std::runtime_error("cannot tell where we are in the file");
    }
    size_t len = (size_t)llen;
    padded_string s(len);
    if (s.data() == nullptr) {
      std::fclose(fp);
      throw std::runtime_error("could not allocate memory");
    }
    std::rewind(fp);
    size_t readb = std::fread(s.data(), 1, len, fp);
    std::fclose(fp);
    if (readb != len) {
      throw std::runtime_error("could not read the data");
    }
    return s;
  }
  throw std::runtime_error("could not load corpus");
}

} // namespace simdjson

#endif
