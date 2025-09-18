#ifdef SIMDJSON_CONDITIONAL_INCLUDE

#ifndef SIMDJSON_RVV_INTRINSICS_H
#define SIMDJSON_RVV_INTRINSICS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/rvv/base.h"
#endif

#if defined(__riscv_vector)
#include <riscv_vector.h>
#else
#error "RVV intrinsics header included but -march=riscv_v not enabled"
#endif

// Get vector length in bytes at run-time (VLENB CSR)
static inline size_t rvv_vlenb() {
    size_t vlenb;
    asm volatile("csrr %0, vlenb" : "=r"(vlenb));
    return vlenb;
}

// Ensure padding is large enough for the largest vector chunk we may use
static_assert(rvv_vlenb() * 4 <= simdjson::SIMDJSON_PADDING,
              "SIMDJSON_PADDING too small for RVV");

#endif // SIMDJSON_RVV_INTRINSICS_H

#endif // SIMDJSON_CONDITIONAL_INCLUDE