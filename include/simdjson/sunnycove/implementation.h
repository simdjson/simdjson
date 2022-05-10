#ifndef SIMDJSON_SUNNYCOVE_IMPLEMENTATION_H
#define SIMDJSON_SUNNYCOVE_IMPLEMENTATION_H

#include "simdjson/implementation.h"

// The constructor may be executed on any host, so we take care not to use SIMDJSON_TARGET_SUNNYCOVE
namespace simdjson {
namespace sunnycove {

using namespace simdjson;

class implementation final : public simdjson::implementation {
public:
  simdjson_really_inline implementation() : simdjson::implementation(
      "sunnycove",
      "Intel/AMD AVX512",
      internal::instruction_set::AVX512 | internal::instruction_set::AVX2 | internal::instruction_set::PCLMULQDQ | internal::instruction_set::BMI1 | internal::instruction_set::BMI2
  ) {}
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace sunnycove
} // namespace simdjson

#endif // SIMDJSON_SUNNYCOVE_IMPLEMENTATION_H