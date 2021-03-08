#ifndef SIMDJSON_IMPLEMENTATIONS_H
#define SIMDJSON_IMPLEMENTATIONS_H

#include "simdjson/implementation-base.h"

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_UNDESIRED_WARNINGS

// Implementations
#include "simdjson/arm64.h"
#include "simdjson/haswell.h"
#include "simdjson/westmere.h"
#include "simdjson/ppc64.h"
#include "simdjson/fallback.h"
#include "simdjson/builtin.h"

SIMDJSON_POP_DISABLE_WARNINGS

#endif // SIMDJSON_IMPLEMENTATIONS_H