#ifndef SIMDJSON_FALLBACK_IMPLEMENTATION_H
#define SIMDJSON_FALLBACK_IMPLEMENTATION_H

#include "simdjson.h"
#include "isadetection.h"

namespace simdjson::fallback {

class implementation final : public simdjson::implementation {
public:
  really_inline implementation() : simdjson::implementation(
      "fallback",
      "Generic fallback implementation",
      0
  ) {}
  WARN_UNUSED error_code parse(const uint8_t *buf, size_t len, document::parser &parser) const noexcept final;
  WARN_UNUSED error_code stage1(const uint8_t *buf, size_t len, document::parser &parser, bool streaming) const noexcept final;
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, document::parser &parser) const noexcept final;
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, document::parser &parser, size_t &next_json) const noexcept final;
};

} // namespace simdjson::fallback

#endif // SIMDJSON_FALLBACK_IMPLEMENTATION_H