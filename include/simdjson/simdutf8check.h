#ifndef SIMDJSON_SIMDUTF8CHECK_H
#define SIMDJSON_SIMDUTF8CHECK_H

#include "simdjson/simdjson.h"
#include "simdjson/simd_input.h"

namespace simdjson {

// Holds the state required to perform check_utf8().
template <Architecture> struct utf8_checking_state;

template <Architecture T>
void check_utf8(simd_input<T> in, utf8_checking_state<T> &state);

// Checks if the utf8 validation has found any error.
template <Architecture T>
ErrorValues check_utf8_errors(utf8_checking_state<T> &state);

} // namespace simdjson

#endif // SIMDJSON_SIMDUTF8CHECK_H
