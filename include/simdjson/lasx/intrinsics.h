#ifndef SIMDJSON_LASX_INTRINSICS_H
#define SIMDJSON_LASX_INTRINSICS_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/lasx/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <lsxintrin.h>
#if defined(__clang__) && !defined(__loongarch_asx)
// LLVM's lasxintrin.h only declares the LASX types and functions when
// __loongarch_asx is defined, that is, when the code is compiled with -mlasx.
// For runtime dispatching, we include it inside a target("lasx") region
// (see simdjson/lasx/begin.h) where the LASX builtins are available, so we
// define __loongarch_asx while including the header.
#define __loongarch_asx 1
#include <lasxintrin.h>
#undef __loongarch_asx
#else
#include <lasxintrin.h>
#endif

static_assert(sizeof(__m256i) <= simdjson::SIMDJSON_PADDING, "insufficient padding for LoongArch ASX");

#endif //  SIMDJSON_LASX_INTRINSICS_H
