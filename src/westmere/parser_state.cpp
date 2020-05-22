#include "westmere/implementation.h"
#include "westmere/parser_state.h"

TARGET_WESTMERE
namespace simdjson {
namespace westmere {

WARN_UNUSED error_code implementation::allocate(parser &parser, size_t max_depth, size_t capacity) const noexcept {
  return parser.implementation_state<parser_state>().allocate(parser, max_depth, capacity);
}

} // namespace westmere
} // namespace simdjson
UNTARGET_REGION
