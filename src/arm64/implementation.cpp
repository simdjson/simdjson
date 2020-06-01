#include "simdjson.h"
#include "arm64/implementation.h"
#include "arm64/dom_parser_implementation.h"

TARGET_HASWELL

namespace simdjson {
namespace arm64 {

WARN_UNUSED error_code implementation::create_dom_parser_implementation(
  size_t capacity,
  size_t max_depth,
  std::unique_ptr<internal::dom_parser_implementation>& dst
) const noexcept {
  dst.reset( new (std::nothrow) dom_parser_implementation() );
  if (!dst) { return MEMALLOC; }
  dst->set_capacity(capacity);
  dst->set_max_depth(max_depth);
  return SUCCESS;
}

} // namespace arm64
} // namespace simdjson

UNTARGET_REGION
