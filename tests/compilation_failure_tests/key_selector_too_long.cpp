// Verifies the compile-time key-length limit of ondemand::key_selector.
//
// Keys are limited to 63 characters; a longer key must trigger the static_assert
// "key_selector keys must be at most 63 characters long". This is checked
// directly (it is orthogonal to the reflective-deserialization fallback, which
// instead silently switches to per-member lookups for over-limit member names).
//
// The CMake harness compiles this twice: with COMPILATION_TEST_USE_FAILING_CODE
// unset (a 63-character key, which must compile) and set to 1 (a 64-character
// key, which must fail to compile with the message above).
#include "simdjson.h"

#if SIMDJSON_SUPPORTS_CONCEPTS

using namespace simdjson;

#if COMPILATION_TEST_USE_FAILING_CODE
// 64 characters: one past the limit. Note 64 == SIMDJSON_PADDING, so the
// "longer than SIMDJSON_PADDING" assertion does not fire first; the 63-character
// assertion is the one we expect.
using fields = ondemand::key_selector<"key_with_sixty_four_chars_one_over_the_limitxxxxxxxxxxxxxxxxxxxx">;
#else
// 63 characters: exactly at the limit, must compile.
using fields = ondemand::key_selector<"key_with_exactly_sixty_three_charsxxxxxxxxxxxxxxxxxxxxxxxxxxxxx">;
#endif

int main() {
  return static_cast<int>(fields::size()) - 1; // 1 key -> returns 0
}

#else

// key_selector requires C++20 concepts; without them there is nothing to check.
int main() { return 0; }

#endif
