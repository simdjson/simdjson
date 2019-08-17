#ifndef SIMDJSON_SIMDUTF8CHECK_H
#define SIMDJSON_SIMDUTF8CHECK_H

#include "simdjson/simdjson.h"
#include "simd_input.h"

namespace simdjson {

// Checks UTF8, chunk by chunk.
template <Architecture T>
struct utf8_checker {
    // Process the next chunk of input.
    void check_next_input(simd_input<T> in);
    // Find out what (if any) errors have occurred
    ErrorValues errors();
};

} // namespace simdjson

#endif // SIMDJSON_SIMDUTF8CHECK_H
