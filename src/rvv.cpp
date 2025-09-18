#ifndef SIMDJSON_SRC_RVV_CPP
#define SIMDJSON_SRC_RVV_CPP

#include "simdjson/rvv.h"
#include "simdjson/rvv/implementation.h"

#include "simdjson/rvv/begin.h"
#include <generic/amalgamated.h>
#include <generic/stage1/amalgamated.h>
#include <generic/stage2/amalgamated.h>

//
// Stage 1
//
namespace simdjson {
namespace rvv {

simdjson_warn_unused error_code implementation::create_dom_parser_implementation(
  size_t capacity,
  size_t max_depth,
  std::unique_ptr<internal::dom_parser_implementation>& dst
) const noexcept {
  dst.reset( new (std::nothrow) dom_parser_implementation() );
  if (!dst) { return MEMALLOC; }
  if (auto err = dst->set_capacity(capacity))
    return err;
  if (auto err = dst->set_max_depth(max_depth))
    return err;
  return SUCCESS;
}



} // namespace rvv
} // namespace simdjson
#endif // SIMDJSON_SRC_RVV_CPP