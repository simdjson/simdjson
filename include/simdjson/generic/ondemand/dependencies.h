#ifdef SIMDJSON_AMALGAMATED
#error simdjson/generic/ondemand/dependencies.h must be included before defining SIMDJSON_AMALGAMATED!
#endif

#ifndef SIMDJSON_GENERIC_ONDEMAND_DEPENDENCIES_H
#define SIMDJSON_GENERIC_ONDEMAND_DEPENDENCIES_H

// Internal headers needed for ondemand generics.
// All includes not under simdjson/generic/ondemand must be here!
// Otherwise, amalgamation will fail.
#include "simdjson/padded_string.h"
#include "simdjson/padded_string_view.h"
#include "simdjson/dom/base.h" // for MINIMAL_DOCUMENT_CAPACITY
#include "simdjson/internal/jsoncharutils_tables.h"
#include "simdjson/internal/numberparsing_tables.h"

#endif // SIMDJSON_GENERIC_ONDEMAND_DEPENDENCIES_H