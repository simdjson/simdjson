#ifndef SIMDJSON_AMALGAMATED_H
#define SIMDJSON_AMALGAMATED_H

#define SIMDJSON_AMALGAMATED

// This provides the public API for simdjson.
// DOM and ondemand are amalgamated separately, in simdjson.h
#include "simdjson/simdjson_version.h"

#include "simdjson/base.h"

#include "simdjson/error.h"
#include "simdjson/error-inl.h"
#include "simdjson/implementation.h"
#include "simdjson/minify.h"
#include "simdjson/padded_string.h"
#include "simdjson/padded_string-inl.h"
#include "simdjson/padded_string_view.h"
#include "simdjson/padded_string_view-inl.h"

#include "simdjson/internal/simdprune_tables.h" // TODO this is only for simd supporting platforms

#endif // SIMDJSON_AMALGAMATED_H