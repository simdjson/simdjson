#include "simdjson/haswell/begin.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

simdjson_warn_unused error_code implementation::create_dom_parser_implementation(
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

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#include "simdjson/haswell/end.h"

