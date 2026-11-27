// Code below from Jsonifier's batched-drain architecture:
// https://nihilai-collective.net/batched-drain

#if SIMDJSON_CPLUSPLUS20

#include <generic/jsonifier/config.h>
#include <generic/jsonifier/avx.h>
#include <generic/jsonifier/avx_stage1.h>
#include <generic/jsonifier/neon.h>
#include <generic/jsonifier/neon_stage1.h>
#include <generic/jsonifier/add_tape_values.h>
#include <generic/jsonifier/simd_string_reader.h>
#include <generic/jsonifier/stage1.h>

#undef SIMDJSON_JSONIFIER_AVX512
#undef SIMDJSON_JSONIFIER_ANY_AVX
#undef SIMDJSON_JSONIFIER_NEON
#undef SIMDJSON_JSONIFIER_CLANG
#undef SIMDJSON_JSONIFIER_GCC
#undef SIMDJSON_JSONIFIER_MSVC
#undef SIMDJSON_JSONIFIER_ASAN

#endif // SIMDJSON_CPLUSPLUS20
