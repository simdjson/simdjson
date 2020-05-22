#include "arm64/implementation.h"
#include "arm64/parser_state.h"

namespace simdjson {
namespace arm64 {

WARN_UNUSED error_code implementation::allocate(parser &parser, size_t max_depth, size_t capacity) const noexcept {
  return parser.implementation_state<parser_state>().allocate(parser, max_depth, capacity);
}

} // namespace arm64
} // namespace simdjson
