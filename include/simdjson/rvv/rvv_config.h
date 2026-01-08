#ifndef SIMDJSON_RVV_RVV_CONFIG_H
#define SIMDJSON_RVV_RVV_CONFIG_H

/**
 * @file rvv_config.h
 * @brief Single Source of Truth for RVV Backend Configuration.
 *
 * This file defines the architectural decisions and constants for the
 * RISC-V Vector backend to prevent configuration drift across headers
 * and translation units.
 */

#include <cstddef> // size_t

namespace simdjson {
namespace rvv {

// -------------------------------------------------------------------------
// 1. Backend Identity & Policy
// -------------------------------------------------------------------------

/**
 * @brief The semantic block size for the parser.
 *
 * The generic simdjson algorithms (Stage 1) operate on 64-byte chunks.
 * We adhere strictly to this to reuse upstream logic.
 */
static constexpr size_t RVV_BLOCK_BYTES = 64;

/**
 * @brief Vector Length Multiplier (LMUL) Policy.
 *
 * Fixed to m1 (LMUL=1) for maximum register allocation stability and
 * compiler optimization potential.
 *
 * IMPORTANT: RVV backend code must not use LMUL > 1 unless this file is
 * updated and all call sites are made consistent.
 */
#define SIMDJSON_RVV_LMUL_M1 1

/**
 * @brief Mask Export Strategy.
 *
 * We generate predicate masks using vector comparisons, store them to a
 * local stack buffer and pack them into a scalar integer.
 *
 * We do NOT use vfirst/vcpop loops for the primary export path.
 */
#define SIMDJSON_RVV_MASK_EXPORT_STORE_AND_PACK 1

/**
 * @brief Tail Safety Policy.
 *
 * STRICT: We never issue a vector load that reads beyond
 * `buf + len + SIMDJSON_PADDING`.
 *
 * Loops must explicitly calculate `vl` (vector length) for the final
 * iteration. We do NOT assume VLEN <= SIMDJSON_PADDING.
 */
#define SIMDJSON_RVV_TAIL_SAFETY_STRICT 1

/**
 * @brief Lookup Table (LUT) Policy.
 *
 * RESTRICTED_WIDTH: Character-classification LUT microkernels should be
 * 16-byte or 32-byte slices. We do NOT rely on full-width vrgather.vv
 * across an entire register group as a baseline.
 */
#define SIMDJSON_RVV_LUT_RESTRICTED_WIDTH 1

// -------------------------------------------------------------------------
// 2. Feature Macros
// -------------------------------------------------------------------------

// Optional: Vector bitmanipulation (ZVBB)
// Defaults to 0 (disabled). Must be explicitly enabled by the build system.
#ifndef SIMDJSON_RVV_ENABLE_ZVBB
#define SIMDJSON_RVV_ENABLE_ZVBB 0
#endif

// -------------------------------------------------------------------------
// 3. Versioning & Constraints
// -------------------------------------------------------------------------

/**
 * @brief RVV intrinsics version requirements.
 *
 * We require RVV 1.0 explicit (non-overloaded) intrinsics.
 * Older 0.7.1 drafts are not supported.
 */
#define SIMDJSON_RVV_VERSION_1_0 1

} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_RVV_CONFIG_H
