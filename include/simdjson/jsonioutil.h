#ifndef SIMDJSON_JSONIOUTIL_H
#define SIMDJSON_JSONIOUTIL_H

#include "simdjson/common_defs.h"
#include "simdjson/padded_string.h"

namespace simdjson {

#if SIMDJSON_EXCEPTIONS

[[deprecated("Use padded_string::load() instead")]]
inline padded_string get_corpus(const char *path) {
  return padded_string::load(path);
}

#endif // SIMDJSON_EXCEPTIONS

} // namespace simdjson

#endif // SIMDJSON_JSONIOUTIL_H
