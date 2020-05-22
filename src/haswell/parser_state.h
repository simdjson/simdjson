#ifndef SIMDJSON_HASWELL_PARSER_STATE
#define SIMDJSON_HASWELL_PARSER_STATE

#include "simdjson.h"

TARGET_HASWELL
namespace simdjson {
namespace haswell {

#include "generic/stage1/stage1_state.h"
#include "generic/stage2/stage2_state.h"

struct parser_state : stage1::stage1_state, stage2::stage2_state {
  really_inline error_code allocate(parser &parser, size_t capacity, size_t max_depth);
}; // struct parser_state

really_inline error_code parser_state::allocate(parser &parser, size_t capacity, size_t max_depth) {
  error_code error = allocate_stage1(parser, capacity, max_depth);
  if (error) { return error; }
  return allocate_stage2(parser, capacity, max_depth);
}

} // namespace haswell
} // namespace simdjson
UNTARGET_REGION

#endif // SIMDJSON_HASWELL_PARSER_STATE
