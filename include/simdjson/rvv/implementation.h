#ifndef SIMDJSON_RVV_IMPLEMENTATION_H
#define SIMDJSON_RVV_IMPLEMENTATION_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#include "simdjson/implementation.h"
#include "simdjson/internal/instruction_set.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace rvv {

class implementation final: public simdjson::implementation {
public:
  simdjson_inline implementation() : simdjson::implementation("rvv", "RISC-V Vector Extension", 0) {}
  simdjson_warn_unused error_code create_dom_parser_implementation(
    size_t capacity,
    size_t max_depth,
    std::unique_ptr<internal::dom_parser_implementation>& dst
  ) const noexcept final;
  simdjson_warn_unused error_code minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept final;
  simdjson_warn_unused bool validate_utf8(const char *buf, size_t len) const noexcept final;
};

} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_IMPLEMENTATION_H