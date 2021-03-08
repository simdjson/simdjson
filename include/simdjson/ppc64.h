#ifndef SIMDJSON_PPC64_H
#define SIMDJSON_PPC64_H

#include "simdjson/implementation-base.h"

#if SIMDJSON_IMPLEMENTATION_PPC64

namespace simdjson {
/**
 * Implementation for ALTIVEC (PPC64).
 */
namespace ppc64 {
} // namespace ppc64
} // namespace simdjson

#include "simdjson/ppc64/implementation.h"

#include "simdjson/ppc64/begin.h"

// Declarations
#include "simdjson/generic/dom_parser_implementation.h"
#include "simdjson/ppc64/intrinsics.h"
#include "simdjson/ppc64/bitmanipulation.h"
#include "simdjson/ppc64/bitmask.h"
#include "simdjson/ppc64/simd.h"
#include "simdjson/generic/jsoncharutils.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/ppc64/stringparsing.h"
#include "simdjson/ppc64/numberparsing.h"
#include "simdjson/ppc64/end.h"

#endif // SIMDJSON_IMPLEMENTATION_PPC64

#endif // SIMDJSON_PPC64_H
