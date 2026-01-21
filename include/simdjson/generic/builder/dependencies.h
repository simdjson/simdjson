#ifdef SIMDJSON_CONDITIONAL_INCLUDE
#error simdjson/generic/builder/dependencies.h must be included before defining SIMDJSON_CONDITIONAL_INCLUDE!
#endif

#ifndef SIMDJSON_GENERIC_BUILDER_DEPENDENCIES_H
#define SIMDJSON_GENERIC_BUILDER_DEPENDENCIES_H

// Internal headers needed for builder generics.
// All includes not under simdjson/generic/builder must be here!
// Otherwise, amalgamation will fail.
#include "simdjson/concepts.h"
#include "simdjson/dom/fractured_json.h"

#endif // SIMDJSON_GENERIC_BUILDER_DEPENDENCIES_H