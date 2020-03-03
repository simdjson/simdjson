#ifndef __SIMDJSON_HASWELL_IMPLEMENTATION_H
#define __SIMDJSON_HASWELL_IMPLEMENTATION_H

#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "simdjson/implementation.h"
#include "isadetection.h"

namespace simdjson::haswell {

class implementation final : public simdjson::implementation {
public:
  really_inline implementation() : simdjson::implementation(
      "haswell",
      "Intel/AMD AVX2",
      instruction_set::AVX2 | instruction_set::PCLMULQDQ | instruction_set::BMI1 | instruction_set::BMI2
  ) {}
  WARN_UNUSED error_code parse(const uint8_t *buf, size_t len, document::parser &parser) const noexcept final;
  WARN_UNUSED error_code stage1(const uint8_t *buf, size_t len, document::parser &parser, bool streaming) const noexcept final;
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, document::parser &parser) const noexcept final;
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, document::parser &parser, size_t &next_json) const noexcept final;
};

} // namespace simdjson::haswell

#endif // IS_X86_64

#endif // __SIMDJSON_HASWELL_IMPLEMENTATION_H