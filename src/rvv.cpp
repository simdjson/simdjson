#include "simdjson/rvv.h"

// -------------------------------------------------------------------------
// Compilation Guard
// -------------------------------------------------------------------------
// This file is compiled as part of the main library.
// We must only generate code if the compiler has been set up for RVV.
// (e.g. -march=rv64gcv or equivalent flags passed by CMake).
// -------------------------------------------------------------------------

#if SIMDJSON_IS_RVV

#include "simdjson/rvv/implementation.h"

namespace simdjson {
namespace rvv {

// -------------------------------------------------------------------------
// Factory Method: DOM Parser
// -------------------------------------------------------------------------
// This creates the parser instance that users actually interact with.
// It instantiates the 'dom_parser_implementation' template which was
// specialized for RVV in 'generic/amalgamated.h' (via begin.h).
// -------------------------------------------------------------------------
simdjson_warn_unused error_code implementation::create_dom_parser_implementation(
    size_t capacity,
    size_t max_depth,
    std::unique_ptr<internal::dom_parser_implementation>& dst
) const noexcept {
  // Allocate the RVV-specialized parser
  dst.reset(new (std::nothrow) dom_parser_implementation());
  
  if (!dst) {
    return MEMALLOC;
  }
  
  // Forward configuration
  if (auto err = dst->set_capacity(capacity)) {
    return err;
  }
  if (auto err = dst->set_max_depth(max_depth)) {
    return err;
  }
  
  return SUCCESS;
}

// -------------------------------------------------------------------------
// Utility: Minify
// -------------------------------------------------------------------------
// Milestone 1: We delegate to the generic (scalar/autovectorized) implementation
// generated within the simdjson::rvv namespace.
// Milestone 2 TODO: Replace with custom RVV whitespace skipping loop.
// -------------------------------------------------------------------------
simdjson_warn_unused error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  return simdjson::rvv::minify(buf, len, dst, dst_len);
}

// -------------------------------------------------------------------------
// Utility: UTF-8 Validation
// -------------------------------------------------------------------------
// Milestone 1: We delegate to the generic implementation.
// Milestone 2 TODO: Replace with RVV vector algorithm (vsetvl + lookup).
// -------------------------------------------------------------------------
simdjson_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  return simdjson::rvv::validate_utf8(buf, len);
}

} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_IS_RVV