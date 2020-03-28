#ifndef SIMDJSON_ARM64_IMPLEMENTATION_H
#define SIMDJSON_ARM64_IMPLEMENTATION_H

#include "simdjson.h"
#include "isadetection.h"

namespace simdjson::arm64 {

using namespace simdjson::dom;

class implementation final : public simdjson::implementation {
public:
  really_inline implementation() : simdjson::implementation("arm64", "ARM NEON", instruction_set::NEON) {}
  WARN_UNUSED error_code parse(const uint8_t *buf, size_t len, parser &parser) const noexcept final;
  WARN_UNUSED error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  WARN_UNUSED error_code stage1(const uint8_t *buf, size_t len, parser &parser, bool streaming) const noexcept final;
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, parser &parser) const noexcept final;
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, parser &parser, size_t &next_json) const noexcept final;
};

} // namespace simdjson::arm64

#endif // SIMDJSON_ARM64_IMPLEMENTATION_H
