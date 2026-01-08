#ifndef SIMDJSON_RVV_BUILDER_H
#define SIMDJSON_RVV_BUILDER_H

#include "simdjson/rvv/base.h"
#include "simdjson/generic/dom_parser_implementation.h"

namespace simdjson {
namespace rvv {

/**
 * RVV-specialized DOM parser implementation.
 *
 * This class serves as the bridge between the high-level DOM API and the
 * RVV-optimized kernels.
 *
 * Status (M0): Fully delegates to simdjson::internal::dom_parser_implementation.
 * Future (M3): Will override stage1() to use RVV structural indexing.
 * Future (M4): Will override stage2() for any backend-specific DOM construction optimizations.
 */
class dom_parser_implementation final : public internal::dom_parser_implementation {
public:
  /**
   * @brief Default constructor.
   *
   * Initializes the generic DOM parser base.
   */
  simdjson_inline dom_parser_implementation() noexcept;

  // -------------------------------------------------------------------------
  // Future Overrides (Milestones M3/M4)
  // -------------------------------------------------------------------------
  // simdjson_warn_unused error_code stage1(const uint8_t *buf, size_t len, bool partial) noexcept final;
  // simdjson_warn_unused error_code stage2(const uint8_t *buf, size_t len, bool partial) noexcept final;
};

simdjson_inline dom_parser_implementation::dom_parser_implementation() noexcept : internal::dom_parser_implementation() {}

} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_BUILDER_H
