#ifndef SIMDJSON_GENERIC_BASE_H

#ifndef SIMDJSON_AMALGAMATED
#define SIMDJSON_GENERIC_BASE_H
#include "simdjson/base.h"
// If we haven't got an implementation yet, just use the builtin one (for the editor)
#ifndef SIMDJSON_IMPLEMENTATION
#include "simdjson/builtin/begin.h"
#endif
#endif // SIMDJSON_AMALGAMATED

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