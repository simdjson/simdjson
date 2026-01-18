#ifndef SIMDJSON_RVV_STAGE1_H
#define SIMDJSON_RVV_STAGE1_H

#include "simdjson/rvv/base.h"
#include "simdjson/rvv/simd.h"
#include "simdjson/rvv/bitmask.h"

#if SIMDJSON_IS_RVV

namespace simdjson {
namespace rvv {

/**
 * @brief Block-level structural character classification.
 *
 * This struct provides the primitives to identify JSON structural characters
 * ({, }, [, ], :, ,) and whitespace within a 64-byte block using RVV.
 *
 * Architectural Decision (M3):
 * - Uses STORE_AND_PACK_U64 for mask export.
 * - Uses direct vector comparisons (baseline) rather than complex LUTs,
 *   prioritizing portability and simplicity for the initial backend.
 */
struct json_character_block {
  // Bitmasks representing the classification of 64 bytes.
  uint64_t whitespace;
  uint64_t op; // Structural characters: { } [ ] : ,

  /**
   * @brief Classifies the characters in the 64-byte block.
   * @param in The 64-byte vector chunk.
   * @return json_character_block containing the classification masks.
   */
  simdjson_inline static json_character_block classify(const simd8x64<uint8_t>& in) {
    // 1. Whitespace Detection
    // JSON whitespace: space (0x20), tab (0x09), linefeed (0x0A), return (0x0D)
    auto whitespace = in == uint8_t(0x20); // space
    whitespace |= in == uint8_t(0x0A);     // linefeed
    whitespace |= in == uint8_t(0x0D);     // carriage return
    whitespace |= in == uint8_t(0x09);     // tab

    // 2. Structural Character Detection
    // JSON structural: { (0x7B), } (0x7D), [ (0x5B), ] (0x5D), : (0x3A), , (0x2C)
    auto op = in == uint8_t(0x2C); // ,
    op |= in == uint8_t(0x3A);     // :
    op |= in == uint8_t(0x5B);     // [
    op |= in == uint8_t(0x5D);     // ]
    op |= in == uint8_t(0x7B);     // {
    op |= in == uint8_t(0x7D);     // }

    // 3. Export to Scalar Masks
    return { whitespace.to_bitmask(), op.to_bitmask() };
  }
};

} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_IS_RVV
#endif // SIMDJSON_RVV_STAGE1_H
