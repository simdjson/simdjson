#ifndef __SIMDJSON_ARM64_IMPLEMENTATION_H
#define __SIMDJSON_ARM64_IMPLEMENTATION_H

#include "simdjson/portability.h"

#ifdef IS_ARM64

#include "simdjson/implementation.h"
#include "simdjson/isadetection.h"

namespace simdjson::arm64 {

class implementation final : public simdjson::implementation {
public:
  really_inline implementation() : simdjson::implementation("arm64", "ARM NEON", instruction_set::NEON) {}
  WARN_UNUSED error_code parse(const uint8_t *buf, size_t len, document::parser &parser) const noexcept final;
  WARN_UNUSED error_code stage1(const uint8_t *buf, size_t len, document::parser &parser, bool streaming) const noexcept final;
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, document::parser &parser) const noexcept final;
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, document::parser &parser, size_t &next_json) const noexcept final;
};

} // namespace simdjson::arm64

#endif // IS_ARM64

#endif // __SIMDJSON_ARM64_IMPLEMENTATION_H
