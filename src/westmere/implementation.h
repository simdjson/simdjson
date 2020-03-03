#ifndef __SIMDJSON_WESTMERE_IMPLEMENTATION_H
#define __SIMDJSON_WESTMERE_IMPLEMENTATION_H

#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "simdjson/implementation.h"
#include "isadetection.h"

namespace simdjson::westmere {

class implementation final : public simdjson::implementation {
public:
  really_inline implementation() : simdjson::implementation("westmere", "Intel/AMD SSE4.2", instruction_set::SSE42 | instruction_set::PCLMULQDQ) {}
  WARN_UNUSED error_code parse(const uint8_t *buf, size_t len, document::parser &parser) const noexcept final;
  WARN_UNUSED error_code stage1(const uint8_t *buf, size_t len, document::parser &parser, bool streaming) const noexcept final;
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, document::parser &parser) const noexcept final;
  WARN_UNUSED error_code stage2(const uint8_t *buf, size_t len, document::parser &parser, size_t &next_json) const noexcept final;
};

} // namespace simdjson::westmere

#endif // IS_X86_64

#endif // __SIMDJSON_WESTMERE_IMPLEMENTATION_H