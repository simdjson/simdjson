#include "simdjson/generic_include_defs.h"
#if SIMDJSON_GENERIC_ONCE(SIMDJSON_GENERIC_BASE_H)
#define SIMDJSON_GENERIC_BASE_H SIMDJSON_GENERIC_INCLUDED(SIMDJSON_GENERIC_BASE_H)

#include "simdjson/base.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

struct open_container;
class dom_parser_implementation;

/// @private
namespace numberparsing {

enum class number_type;

} // namespace numberparsing
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONCE