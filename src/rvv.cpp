#ifndef SIMDJSON_SRC_RVV_CPP
#define SIMDJSON_SRC_RVV_CPP

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include <base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE


#include <simdjson/rvv.h>
#include <simdjson/rvv/implementation.h>



namespace simdjson {
namespace rvv {
error_code implementation::create_dom_parser_implementation(
    size_t, size_t,
    std::unique_ptr<simdjson::internal::dom_parser_implementation>&) const noexcept {
  return error_code::UNSUPPORTED_ARCHITECTURE;
}
} // namespace rvv
} // namespace simdjson
#endif // SIMDJSON_SRC_RVV_CPP