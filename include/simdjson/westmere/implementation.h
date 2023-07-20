#ifndef SIMDJSON_WESTMERE_IMPLEMENTATION_H
#define SIMDJSON_WESTMERE_IMPLEMENTATION_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/westmere/base.h"
#include "simdjson/implementation.h"
#include "simdjson/internal/instruction_set.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

// The constructor may be executed on any host, so we take care not to use SIMDJSON_TARGET_WESTMERE
namespace simdjson {
namespace westmere {

/**
 * @private
 */
class implementation final : public simdjson::implementation {
public:
  simdjson_inline implementation() : simdjson::implementation("westmere", "Intel/AMD SSE4.2", internal::instruction_set::SSE42 | internal::instruction_set::PCLMULQDQ) {}
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace westmere
} // namespace simdjson

#endif // SIMDJSON_WESTMERE_IMPLEMENTATION_H
