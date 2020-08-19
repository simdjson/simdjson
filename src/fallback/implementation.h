#ifndef SIMDJSON_FALLBACK_IMPLEMENTATION_H
#define SIMDJSON_FALLBACK_IMPLEMENTATION_H

#include "simdjson.h"
#include "isadetection.h"

namespace {
namespace fallback {

using namespace simdjson;
using namespace simdjson::dom;

class implementation final : public simdjson::implementation {
public:
  simdjson_really_inline implementation() : simdjson::implementation(
      "fallback",
      "Generic fallback implementation",
      0
  ) {}
  SIMDJSON_WARN_UNUSED error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_length,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  SIMDJSON_WARN_UNUSED error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  SIMDJSON_WARN_UNUSED bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace fallback
} // unnamed namespace

#endif // SIMDJSON_FALLBACK_IMPLEMENTATION_H