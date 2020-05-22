#include "fallback/implementation.h"
#include "fallback/parser_state.h"

namespace simdjson {
namespace fallback {

WARN_UNUSED error_code implementation::allocate(parser &parser, size_t max_depth, size_t capacity) const noexcept {
  return parser.implementation_state<parser_state>().allocate(parser, max_depth, capacity);
}

} // namespace fallback
} // namespace simdjson
