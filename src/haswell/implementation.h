#ifndef SIMDJSON_HASWELL_IMPLEMENTATION_H
#define SIMDJSON_HASWELL_IMPLEMENTATION_H

#include "simdjson.h"
#include "isadetection.h"

// The constructor may be executed on any host, so we take care not to use SIMDJSON_TARGET_REGION
namespace {
namespace haswell {

using namespace simdjson;

class implementation final : public simdjson::implementation {
public:
  simdjson_really_inline implementation() : simdjson::implementation(
      "haswell",
      "Intel/AMD AVX2",
      instruction_set::AVX2 | instruction_set::PCLMULQDQ | instruction_set::BMI1 | instruction_set::BMI2
  ) {}
  SIMDJSON_WARN_UNUSED error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  SIMDJSON_WARN_UNUSED error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  SIMDJSON_WARN_UNUSED bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace haswell
} // unnamed namespace

#endif // SIMDJSON_HASWELL_IMPLEMENTATION_H