#ifndef SIMDJSON_H
#define SIMDJSON_H

/**
 * @mainpage
 *
 * Check the [README.md](https://github.com/lemire/simdjson/blob/master/README.md#simdjson--parsing-gigabytes-of-json-per-second).
 */

#include "simdjson/compiler_check.h"
#include "simdjson/common_defs.h"

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING

// Public API
#include "simdjson/simdjson_version.h"
#include "simdjson/error.h"
#include "simdjson/padded_string.h"
#include "simdjson/implementation.h"
#include "simdjson/document.h"
#include "simdjson/document_stream.h"

// // Deprecated API
#include "simdjson/jsonparser.h"
#include "simdjson/parsedjson.h"
#include "simdjson/parsedjson_iterator.h"

// // Inline functions
#include "simdjson/inline/document.h"
#include "simdjson/inline/document_stream.h"
#include "simdjson/inline/error.h"
#include "simdjson/inline/padded_string.h"
#include "simdjson/inline/parsedjson_iterator.h"

SIMDJSON_POP_DISABLE_WARNINGS

#endif // SIMDJSON_H
