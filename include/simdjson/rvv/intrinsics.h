#ifdef SIMDJSON_CONDITIONAL_INCLUDE

#ifndef SIMDJSON_RVV_INTRINSICS_H
#define SIMDJSON_RVV_INTRINSICS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/rvv/base.h"

#if defined(__riscv_vector)
#include <riscv_vector.h>
#else
#error "RVV intrinsics header included but -march=rv64gcv (or rv32gcv) not enabled"
#endif

#endif // SIMDJSON_CONDITIONAL_INCLUDE


#endif // SIMDJSON_RVV_INTRINSICS_H
