#ifdef SIMDJSON_CONDITIONAL_INCLUDE
#error simdjson/generic/dependencies.h must be included before defining SIMDJSON_CONDITIONAL_INCLUDE!
#endif

#ifndef SIMDJSON_GENERIC_DEPENDENCIES_H
#define SIMDJSON_GENERIC_DEPENDENCIES_H

// Internal headers needed for generics.
// All includes referencing simdjson headers *not* under simdjson/generic must be here!
// Otherwise, amalgamation will fail.
#include "simdjson/base.h"
#include "simdjson/implementation.h"
#include "simdjson/implementation_detection.h"
#include "simdjson/internal/instruction_set.h"
#include "simdjson/internal/dom_parser_implementation.h"
#include "simdjson/internal/jsoncharutils_tables.h"
#include "simdjson/internal/numberparsing_tables.h"
#include "simdjson/internal/simdprune_tables.h"
#endif // SIMDJSON_GENERIC_DEPENDENCIES_H