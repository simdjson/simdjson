#if defined(SIMDJSON_CONDITIONAL_INCLUDE) && !defined(SIMDJSON_GENERIC_BUILDER_DEPENDENCIES_H)
#error simdjson/generic/builder/dependencies.h must be included before simdjson/generic/builder/amalgamated.h!
#endif

#include "simdjson/generic/builder/json_string_builder.h"
#include "simdjson/generic/builder/json_builder.h"
#include "simdjson/generic/builder/fractured_json_builder.h"



// JSON builder inline definitions
#include "simdjson/generic/builder/json_string_builder-inl.h"

