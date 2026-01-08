#ifndef SIMDJSON_RVV_MINIFY_H
#define SIMDJSON_RVV_MINIFY_H

#include "simdjson/rvv/base.h"
#include "simdjson/rvv/intrinsics.h"
#include "simdjson/rvv/rvv_config.h"

namespace simdjson {
namespace rvv {

/**
 * @brief Vectorized whitespace removal (Minify).
 *
 * Scans the input buffer, removes JSON whitespace characters 
 * (Space, Line Feed, Carriage Return, Tab), and writes the 
 * result to the destination.
 *
 * Architecture:
 * - Uses `vcompress` to pack valid bytes contiguously in a register.
 * - Processes up to 64 bytes per loop (clamped by RVV_BLOCK_BYTES).
 * - Fully tail-safe.
 */
simdjson_inline error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) noexcept {
  const uint8_t *src_ptr = buf;
  uint8_t *dst_ptr = dst;
  size_t remaining = len;

  while (remaining > 0) {
    // 1. Configure Vector Length (Tail-safe)
    // We clamp AVL to RVV_BLOCK_BYTES (64) to strictly follow the backend policy,
    // ensuring we don't process massive chunks on large-vector hardware 
    // which could affect latency consistency.
    size_t avl = (remaining < RVV_BLOCK_BYTES) ? remaining : RVV_BLOCK_BYTES;
    size_t vl = __riscv_vsetvl_e8m1(avl);

    // 2. Load Data
    vuint8m1_t v_data = __riscv_vle8_v_u8m1(src_ptr, vl);

    // 3. Identify Whitespace
    // JSON Whitespace: Space (0x20), LineFeed (0x0A), CarriageReturn (0x0D), Tab (0x09)
    // We perform parallel comparisons.
    vbool8_t is_space = __riscv_vmseq_vx_u8m1_b8(v_data, 0x20, vl);
    vbool8_t is_lf    = __riscv_vmseq_vx_u8m1_b8(v_data, 0x0A, vl);
    vbool8_t is_cr    = __riscv_vmseq_vx_u8m1_b8(v_data, 0x0D, vl);
    vbool8_t is_tab   = __riscv_vmseq_vx_u8m1_b8(v_data, 0x09, vl);

    // Combine masks: ws = space | lf | cr | tab
    vbool8_t is_ws;
    is_ws = __riscv_vmor_mm_b8(is_space, is_lf, vl);
    is_ws = __riscv_vmor_mm_b8(is_ws, is_cr, vl);
    is_ws = __riscv_vmor_mm_b8(is_ws, is_tab, vl);

    // 4. Compute Keep Mask (NOT whitespace)
    vbool8_t keep = __riscv_vmnot_m_b8(is_ws, vl);

    // 5. Compress
    // Packs elements where 'keep' is 1 to the beginning of v_compressed.
    vuint8m1_t v_compressed = __riscv_vcompress_vm_u8m1(v_data, keep, vl);

    // 6. Count output bytes
    // vcpop counts the number of set bits (valid characters)
    unsigned long keep_count = __riscv_vcpop_m_b8(keep, vl);

    // 7. Store valid bytes
    if (keep_count > 0) {
      // We rely on v_compressed having data at index 0..keep_count-1.
      __riscv_vse8_v_u8m1(dst_ptr, v_compressed, keep_count);
      dst_ptr += keep_count;
    }

    // Advance
    src_ptr += vl;
    remaining -= vl;
  }

  dst_len = static_cast<size_t>(dst_ptr - dst);
  return SUCCESS;
}

} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_MINIFY_H