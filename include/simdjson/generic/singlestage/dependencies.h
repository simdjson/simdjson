#ifdef SIMDJSON_AMALGAMATED
#error simdjson/generic/singlestage/dependencies.h must be included before defining SIMDJSON_AMALGAMATED!
#endif

#ifndef SIMDJSON_GENERIC_SINGLESTAGE_DEPENDENCIES_H
#define SIMDJSON_GENERIC_SINGLESTAGE_DEPENDENCIES_H

// Internal headers needed for singlestage generics.
// All includes not under simdjson/generic/singlestage must be here!
// Otherwise, amalgamation will fail.
#include "simdjson/dom/base.h" // for MINIMAL_DOCUMENT_CAPACITY
#include "simdjson/implementation.h"
#include "simdjson/padded_string.h"
#include "simdjson/padded_string_view.h"
#include "simdjson/internal/dom_parser_implementation.h"

#endif // SIMDJSON_GENERIC_SINGLESTAGE_DEPENDENCIES_H