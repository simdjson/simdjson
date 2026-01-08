#ifndef SIMDJSON_RVV_IMPLEMENTATION_H
#define SIMDJSON_RVV_IMPLEMENTATION_H

#include <memory>
#include <string>
#include <vector>

// The begin.h header sets up the specific namespace (simdjson::rvv)
// and ensures necessary macros (like SIMDJSON_TARGET_RVV) are active.
#include "simdjson/rvv/begin.h"

namespace simdjson {
namespace rvv {

/**
 * RISC-V Vector (RVV) implementation.
 * * Target Architecture: RV64GCV
 * Vector Spec: v1.0
 * Strategy: Variable-Length Agnostic (VLA) adapted to 64-byte chunks.
 */
class implementation final : public simdjson::implementation {
public:
  /**
   * Constructor registers this implementation with the runtime system.
   * * @param name "rvv" (used for manual selection via SIMDJSON_FORCE_IMPL)
   * @param description Human-readable description
   * @param required_instruction_set Bitmask ensuring we only run on capable hardware
   */
  simdjson_inline implementation() : simdjson::implementation(
      "rvv",
      "RISC-V Vector Extension (RVV 1.0)",
      internal::instruction_set::RVV
  ) {}

  /**
   * Create a DOM parser implementation optimized for RVV.
   * This typically instantiates a generic template specialized for the rvv namespace.
   */
  simdjson_warn_unused error_code create_dom_parser_implementation(
      size_t capacity,
      size_t max_depth,
      std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;

  /**
   * Minify JSON string (remove whitespace).
   * Implemented in src/rvv.cpp using RVV vector whitespace lookup.
   */
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;

  /**
   * Validate UTF-8 encoding.
   * Implemented in src/rvv.cpp using RVV vector validation.
   */
  simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace rvv
} // namespace simdjson

// The end.h header restores the previous namespace state and cleans up macros.
#include "simdjson/rvv/end.h"

#endif // SIMDJSON_RVV_IMPLEMENTATION_H