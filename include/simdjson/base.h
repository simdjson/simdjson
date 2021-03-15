#ifndef SIMDJSON_BASE_H
#define SIMDJSON_BASE_H

#include "simdjson/compiler_check.h"
#include "simdjson/common_defs.h"
#include "simdjson/portability.h"

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

// Public API
#include "simdjson/simdjson_version.h"
#include "simdjson/error.h"
#include "simdjson/minify.h"
#include "simdjson/padded_string.h"
#include "simdjson/padded_string_view.h"
#include "simdjson/implementation.h"

// Inline functions
#include "simdjson/error-inl.h"
#include "simdjson/padded_string-inl.h"
#include "simdjson/padded_string_view-inl.h"

SIMDJSON_POP_DISABLE_WARNINGS

#endif // SIMDJSON_BASE_H