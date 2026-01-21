#ifndef SIMDJSON_RVV_STAGE2_H
#define SIMDJSON_RVV_STAGE2_H

#include "simdjson/rvv/base.h"
#include "simdjson/rvv/intrinsics.h"
#include "simdjson/rvv/rvv_config.h"

namespace simdjson {
namespace rvv {
namespace numberparsing {

/**
 * @brief Architecture-specific Number Parsing Kernels.
 *
 * Current Status (M0/M1):
 * - We rely on the robust scalar fallback provided by the generic implementation
 * for integer and floating-point parsing.
 * - Unlike Stage 1, Stage 2 is branch-heavy and less amenable to simple
 * data-parallelism without specialized gather/scatter support.
 *
 * Future (M4 - Stage 2 Optimization):
 * - We may implement vectorized 8-digit decimal-to-integer conversion here
 * using RVV multiply-add instructions if benchmarks justify the complexity.
 * - Potential for `vslide` based parsing of short integers.
 */

// Placeholder for M4:
// simdjson_inline uint32_t parse_eight_digits_unrolled(const uint8_t *chars);

} // namespace numberparsing
} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_STAGE2_H
