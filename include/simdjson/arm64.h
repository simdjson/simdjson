#ifndef SIMDJSON_ARM64_H
#define SIMDJSON_ARM64_H

#include "simdjson/portability.h"

#if SIMDJSON_IMPLEMENTATION_ARM64

#include "simdjson/arm64/implementation.h"

#include "simdjson/arm64/begin.h"
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/arm64/intrinsics.h"
#include "simdjson/arm64/bitmanipulation.h"
#include "simdjson/arm64/bitmask.h"
#include "simdjson/arm64/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/arm64/stringparsing.h"
#include "simdjson/arm64/numberparsing.h"
#include "simdjson/arm64/end.h"

#endif // SIMDJSON_IMPLEMENTATION_ARM64

#endif // SIMDJSON_ARM64_H
