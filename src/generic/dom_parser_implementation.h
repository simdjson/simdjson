#ifndef SIMDJSON_SRC_GENERIC_DOM_PARSER_IMPLEMENTATION_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_DOM_PARSER_IMPLEMENTATION_H
#include <generic/base.h>
#include <simdjson/generic/dom_parser_implementation.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

// Interface a dom parser implementation must fulfill
namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {

simdjson_inline simd8<bool> must_be_2_3_continuation(const simd8<uint8_t> prev2, const simd8<uint8_t> prev3);
simdjson_inline bool is_ascii(const simd8x64<uint8_t>& input);

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_DOM_PARSER_IMPLEMENTATION_H