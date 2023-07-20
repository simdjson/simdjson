#ifndef SIMDJSON_SRC_GENERIC_JSON_CHARACTER_BLOCK_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_JSON_CHARACTER_BLOCK_H
#include <generic/base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {

struct json_character_block {
  static simdjson_inline json_character_block classify(const simd::simd8x64<uint8_t>& in);

  simdjson_inline uint64_t whitespace() const noexcept { return _whitespace; }
  simdjson_inline uint64_t op() const noexcept { return _op; }
  simdjson_inline uint64_t scalar() const noexcept { return ~(op() | whitespace()); }

  uint64_t _whitespace;
  uint64_t _op;
};

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_JSON_CHARACTER_BLOCK_H