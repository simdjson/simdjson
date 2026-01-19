#ifndef SIMDJSON_RVV_VLS_INTRINSICS_H
#define SIMDJSON_RVV_VLS_INTRINSICS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/rvv-vls/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <riscv_vector.h>

#define simdutf_vrgather_u8m1x2(tbl, idx)                                      \
  __riscv_vcreate_v_u8m1_u8m2(                                                 \
      __riscv_vrgather_vv_u8m1(tbl, __riscv_vget_v_u8m2_u8m1(idx, 0),          \
                               __riscv_vsetvlmax_e8m1()),                      \
      __riscv_vrgather_vv_u8m1(tbl, __riscv_vget_v_u8m2_u8m1(idx, 1),          \
                               __riscv_vsetvlmax_e8m1()))

#define simdutf_vrgather_u8m1x4(tbl, idx)                                      \
  __riscv_vcreate_v_u8m1_u8m4(                                                 \
      __riscv_vrgather_vv_u8m1(tbl, __riscv_vget_v_u8m4_u8m1(idx, 0),          \
                               __riscv_vsetvlmax_e8m1()),                      \
      __riscv_vrgather_vv_u8m1(tbl, __riscv_vget_v_u8m4_u8m1(idx, 1),          \
                               __riscv_vsetvlmax_e8m1()),                      \
      __riscv_vrgather_vv_u8m1(tbl, __riscv_vget_v_u8m4_u8m1(idx, 2),          \
                               __riscv_vsetvlmax_e8m1()),                      \
      __riscv_vrgather_vv_u8m1(tbl, __riscv_vget_v_u8m4_u8m1(idx, 3),          \
                               __riscv_vsetvlmax_e8m1()))

#if __riscv_zbc
#include <riscv_bitmanip.h>
#endif

#endif // SIMDJSON_RVV_VLS_INTRINSICS_H
