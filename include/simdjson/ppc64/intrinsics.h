#ifndef SIMDJSON_PPC64_INTRINSICS_H
#define SIMDJSON_PPC64_INTRINSICS_H

#include "simdjson/base.h"

// This should be the correct header whether
// you use visual studio or other compilers.
#include <altivec.h>

// These are defined by altivec.h in GCC toolchain, it is safe to undef them.
#ifdef bool
#undef bool
#endif

#ifdef vector
#undef vector
#endif

static_assert(sizeof(__vector unsigned char) <= simdjson::SIMDJSON_PADDING, "insufficient padding for ppc64");

#endif //  SIMDJSON_PPC64_INTRINSICS_H
