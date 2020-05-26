#ifndef SIMDJSON_ARM64_PARSER_STATE
#define SIMDJSON_ARM64_PARSER_STATE

#include "simdjson.h"

namespace simdjson {
namespace arm64 {

#include "generic/stage2/stage2_state.h"

struct parser_state : stage2::stage2_state {
  really_inline error_code allocate(parser &parser, size_t capacity, size_t max_depth);
}; // struct parser_state

really_inline error_code parser_state::allocate(parser &parser, size_t capacity, size_t max_depth) {
  return allocate_stage2(parser, capacity, max_depth);
}

} // namespace arm64
} // namespace simdjson

#endif // SIMDJSON_ARM64_PARSER_STATE
