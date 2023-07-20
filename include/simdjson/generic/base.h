#ifndef SIMDJSON_GENERIC_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_BASE_H
#include "simdjson/base.h"
// If we haven't got an implementation yet, we're in the editor, editing a generic file! Just
// use the most advanced one we can so the most possible stuff can be tested.
#ifndef SIMDJSON_IMPLEMENTATION
#include "simdjson/implementation_detection.h"
#if SIMDJSON_IMPLEMENTATION_ICELAKE
#include "simdjson/icelake/begin.h"
#elif SIMDJSON_IMPLEMENTATION_HASWELL
#include "simdjson/haswell/begin.h"
#elif SIMDJSON_IMPLEMENTATION_WESTMERE
#include "simdjson/westmere/begin.h"
#elif SIMDJSON_IMPLEMENTATION_ARM64
#include "simdjson/arm64/begin.h"
#elif SIMDJSON_IMPLEMENTATION_PPC64
#include "simdjson/ppc64/begin.h"
#elif SIMDJSON_IMPLEMENTATION_FALLBACK
#include "simdjson/fallback/begin.h"
#else
#error "All possible implementations (including fallback) have been disabled! simdjson will not run."
#endif
#endif // SIMDJSON_IMPLEMENTATION
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

struct open_container;
class dom_parser_implementation;

/**
 * The type of a JSON number
 */
enum class number_type {
    floating_point_number=1, /// a binary64 number
    signed_integer,          /// a signed integer that fits in a 64-bit word using two's complement
    unsigned_integer         /// a positive integer larger or equal to 1<<63
};

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_BASE_H