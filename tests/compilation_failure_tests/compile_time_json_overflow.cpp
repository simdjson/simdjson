// Verifies that a number too large for binary64 is rejected at compile time.
//
// The runtime parser reports NUMBER_ERROR for a value that overflows to
// infinity, because JSON has no way to spell one. A constant expression has no
// error channel, so the only way to report it is to refuse to compile.
//
// The CMake harness compiles this twice: with COMPILATION_TEST_USE_FAILING_CODE
// unset (the largest finite binary64, which must compile) and set to 1 (1e400,
// which must not).
#include "simdjson.h"

#if SIMDJSON_STATIC_REFLECTION

#if COMPILATION_TEST_USE_FAILING_CODE
// 1e400 is far past DBL_MAX and rounds to infinity.
constexpr auto doc = simdjson::compile_time::parse_json<R"({"value": 1e400})">();
#else
// The largest finite binary64, which must round to exactly DBL_MAX.
constexpr auto doc =
    simdjson::compile_time::parse_json<R"({"value": 1.7976931348623157e308})">();
#endif

int main() { return doc.value > 0 ? 0 : 1; }

#else

// Compile-time JSON parsing needs static reflection; without it there is
// nothing to check. (CMake only registers this test when reflection is on, so
// this branch exists only to keep the file compilable on its own.)
int main() { return 0; }

#endif
