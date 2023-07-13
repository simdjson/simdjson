#ifndef SIMDJSON_GENERIC_DEPENDENCIES_H
#define SIMDJSON_GENERIC_DEPENDENCIES_H

// Internal headers needed for generics.
// All includes referencing simdjson headers *not* under simdjson/generic must be here!
// Otherwise, amalgamation will fail.
#include "simdjson/dom/document.h"
#include "simdjson/internal/jsoncharutils_tables.h"
#include "simdjson/internal/numberparsing_tables.h"
#include "simdjson/padded_string_view.h"

#endif // SIMDJSON_GENERIC_DEPENDENCIES_H