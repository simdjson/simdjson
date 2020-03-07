#ifndef SIMDJSON_JSONIOUTIL_H
#define SIMDJSON_JSONIOUTIL_H

#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "simdjson/common_defs.h"
#include "simdjson/padded_string.h"

namespace simdjson {

inline padded_string get_corpus(const std::string &filename) {
  return padded_string::load(filename);
}

} // namespace simdjson

#endif
