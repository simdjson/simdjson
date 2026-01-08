#ifndef SIMDJSON_RVV_BEGIN_H
#define SIMDJSON_RVV_BEGIN_H

#include "simdjson/portability.h"

// -------------------------------------------------------------------------
// 1) Implementation Macros (fixed)
// -------------------------------------------------------------------------
// Drives generic kernel instantiation into namespace simdjson::rvv.
#ifndef SIMDJSON_IMPLEMENTATION
#define SIMDJSON_IMPLEMENTATION rvv
#endif

// Optional target macro (kept for symmetry with other backends).
#ifndef SIMDJSON_TARGET_RVV
#define SIMDJSON_TARGET_RVV
#endif

// -------------------------------------------------------------------------
// 2) RVV feature gate
// -------------------------------------------------------------------------
// We must not hard-reference RVV intrinsics/types when RVV is unavailable,
// otherwise including simdjson/rvv.h would break builds on x86/ARM.
// The canonical gate is SIMDJSON_IS_RVV (provided by simdjson).
// -------------------------------------------------------------------------

#if SIMDJSON_IS_RVV

// -------------------------------------------------------------------------
// 3) Include RVV building blocks
// -------------------------------------------------------------------------
// These must be visible before generic templates are instantiated.
#include "simdjson/rvv/base.h"
#include "simdjson/rvv/intrinsics.h"
#include "simdjson/rvv/bitmanipulation.h"
#include "simdjson/rvv/bitmask.h"
#include "simdjson/rvv/simd.h"
#include "simdjson/rvv/numberparsing_defs.h"
#include "simdjson/rvv/stringparsing_defs.h"

#endif // SIMDJSON_IS_RVV

// -------------------------------------------------------------------------
// NOTE: Do NOT include simdjson/generic/amalgamated.h here.
// It is included by include/simdjson/rvv.h to avoid double-including it.
// -------------------------------------------------------------------------

#endif // SIMDJSON_RVV_BEGIN_H
