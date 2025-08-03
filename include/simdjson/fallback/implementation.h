#ifndef SIMDJSON_FALLBACK_IMPLEMENTATION_H
#define SIMDJSON_FALLBACK_IMPLEMENTATION_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/fallback/base.h"
#include "simdjson/implementation.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace fallback {

/**
 * @private
 */
class implementation final : public simdjson::implementation {
public:
  simdjson_inline implementation() : simdjson::implementation(
      "fallback",
      "Generic fallback implementation",
      0
  ) {}
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<simdjson::internal::dom_parser_implementation>& dst
  ) const noexcept final;
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace fallback
} // namespace simdjson

#endif // SIMDJSON_FALLBACK_IMPLEMENTATION_H
