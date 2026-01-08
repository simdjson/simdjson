#ifndef SIMDJSON_RVV_UTF8_VALIDATION_H
#define SIMDJSON_RVV_UTF8_VALIDATION_H

#include "simdjson/rvv/base.h"
#include "simdjson/rvv/intrinsics.h"
#include "simdjson/rvv/rvv_config.h"

// Fallback to generic for complex multi-byte sequences (robustness)
#include "simdjson/generic/utf8_validation.h"

#if SIMDJSON_IS_RVV

namespace simdjson {
namespace rvv {

/**
 * Validates that the string is valid UTF-8.
 *
 * Strategy:
 * 1. RVV Vectorized ASCII fast-path:
 *   - Loads up to RVV_BLOCK_BYTES (64 bytes) with strict tail safety.
 *   - Detects any non-ASCII byte (>= 0x80) using signed < 0.
 * 2. Scalar fallback:
 *   - On first non-ASCII byte, hand over to the proven generic validator
 *     for the remainder of the buffer to guarantee correctness.
 */
simdjson_inline bool validate_utf8(const char *buf, size_t len) {
  const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
  size_t remaining = len;

  while (remaining > 0) {
    const size_t avl = (remaining < RVV_BLOCK_BYTES) ? remaining : RVV_BLOCK_BYTES;

    // LMUL=1 (fixed by rvv_config.h policy)
    const size_t vl = __riscv_vsetvl_e8m1(avl);

    // Tail-safe load: reads exactly vl bytes
    const vuint8m1_t v_data = __riscv_vle8_v_u8m1(data, vl);

    // ASCII check: treat bytes as signed; any byte with high bit set becomes negative
    const vint8m1_t v_signed = __riscv_vreinterpret_v_u8m1_i8m1(v_data);

    // mask[i] = (v_signed[i] < 0)
    const vbool8_t non_ascii = __riscv_vmslt_vx_i8m1_b8(v_signed, 0, vl);

    // Returns first active index or -1 if none
    const long first_non_ascii = __riscv_vfirst_m_b8(non_ascii, vl);

    if (simdjson_unlikely(first_non_ascii >= 0)) {
      // Handover to generic validator from the first non-ASCII byte
      return simdjson::generic::validate_utf8(
        reinterpret_cast<const char *>(data + static_cast<size_t>(first_non_ascii)),
        remaining - static_cast<size_t>(first_non_ascii)
      );
    }

    data += vl;
    remaining -= vl;
  }

  return true;
}

} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_IS_RVV
#endif // SIMDJSON_RVV_UTF8_VALIDATION_H
