#ifndef SIMDJSON_RVV_ONDEMAND_H
#define SIMDJSON_RVV_ONDEMAND_H

#include "simdjson/rvv/base.h"
#include "simdjson/rvv/simd.h"

namespace simdjson {
namespace rvv {
namespace ondemand {

// -------------------------------------------------------------------------
// On-Demand Backend Logic
// -------------------------------------------------------------------------
// This namespace will eventually contain specialized RVV kernels for:
// 1. skip_string (Scanning past string values without parsing them)
// 2. find_next_structural (Jumping between JSON tokens)
//
// For Milestone 1 (Skeleton), we leave this empty.
// The generic ondemand::parser will utilize the basic SIMD primitives
// defined in rvv/simd.h and rvv/stringparsing_defs.h via the
// generic implementation templates.
// -------------------------------------------------------------------------

} // namespace ondemand
} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_ONDEMAND_H
