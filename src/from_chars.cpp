#ifndef SIMDJSON_SRC_FROM_CHARS_CPP
#define SIMDJSON_SRC_FROM_CHARS_CPP

#include <base.h>

#include "simdjson/internal/fast_float.h"

#include <cstdint>
#include <cstring>
#include <limits>

namespace simdjson {
namespace internal {

/**
 * These functions handle floating-point parsing when the fast path in
 * numberparsing.h gives up: more than 19 digits in the decimal mantissa, an
 * exponent outside the range the power-of-five table covers, or one of the rare
 * inputs where the truncated Eisel-Lemire product is not accurate enough to
 * round. That should only be seen in adversarial scenarios; we do not expect
 * production systems to even produce such floating-point numbers.
 *
 * The work is handed to fast_float (vendored in
 * include/simdjson/internal/fast_float.h), which settles the rounding by
 * comparing a bigint against a scaled power of five. It is correctly rounded,
 * and quick enough that an adversarial document is no longer worth worrying
 * about.
 **/

namespace {

// fast_float wants the end of the number, and the callers only promise that a
// number is followed by a character which cannot be part of one -- the input
// has already been validated against the JSON grammar, and the buffer is padded,
// so such a character is always there to be found. Locating it costs a pass over
// digits we are about to parse anyway, and in exchange fast_float can bound its
// inner loops instead of re-checking a far-away end pointer.
const char *find_end_of_number(const char *first) noexcept {
  const char *p = first;
  while ((*p >= '0' && *p <= '9') || *p == '-' || *p == '+' || *p == '.' ||
         *p == 'e' || *p == 'E') {
    p++;
  }
  return p;
}

// The input is JSON, so parse it under the JSON grammar: no hexadecimal, no
// leading plus, and no infinity or NaN spellings. Those are handled (or
// rejected) before we ever get here.
constexpr simdjson_fast_float::parse_options json_options{
    simdjson_fast_float::chars_format::json};

// fast_float reports result_out_of_range for a value at either edge of the
// format, writing +/-0 when it underflows and +/-infinity when it overflows.
// Both are exactly what the callers of these functions expect to receive: they
// accept a zero and treat an infinity as an error. A malformed number cannot
// happen on validated input, but if it somehow did, returning zero matches what
// the previous implementation did with digits it could not use.
template <typename T> T parse_with_fast_float(const char *first, const char *end) noexcept {
  T value{};
  auto answer =
      simdjson_fast_float::from_chars_advanced(first, end, value, json_options);
  if (answer.ec == std::errc::invalid_argument) { return T(0); }
  return value;
}

} // namespace

double from_chars(const char *first) noexcept {
  return parse_with_fast_float<double>(first, find_end_of_number(first));
}

double from_chars(const char *first, const char *end) noexcept {
  return parse_with_fast_float<double>(first, end);
}

float from_chars_float(const char *first) noexcept {
  return parse_with_fast_float<float>(first, find_end_of_number(first));
}

} // internal
} // simdjson

#endif // SIMDJSON_SRC_FROM_CHARS_CPP
