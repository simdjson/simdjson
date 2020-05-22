#include "haswell/implementation.h"
#include "haswell/parser_state.h"

TARGET_HASWELL
namespace simdjson {
namespace haswell {

WARN_UNUSED error_code implementation::allocate(parser &parser, size_t max_depth, size_t capacity) const noexcept {
  return parser.implementation_state<parser_state>().allocate(parser, max_depth, capacity);
}

} // namespace haswell
} // namespace simdjson
UNTARGET_REGION