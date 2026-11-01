#include "simdjson.h"
#include "test_macros.h"

#include <cstdint>
#include <cstdlib>

// Tests for the SIMD layer of the builtin implementation. The fallback and
// rvv-vls kernels have no simd8<bool>::any(), so there is nothing to test.
#if SIMDJSON_BUILTIN_IMPLEMENTATION_IS(fallback) ||                            \
    SIMDJSON_BUILTIN_IMPLEMENTATION_IS(rvv_vls)
#define TEST_SIMD8 0
#else
#define TEST_SIMD8 1
#endif

#if TEST_SIMD8
namespace simd = simdjson::builtin::simd;
using simd::simd8;
constexpr size_t width = sizeof(simd8<uint8_t>);

// Byte i holds i, so comparing with a splat of i selects byte i alone.
static const uint8_t byte_index[64] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};
static_assert(width <= sizeof(byte_index), "a register is at most 64 bytes");

static bool test_simd8_bool_any() {
  TEST_START();
  ASSERT_FALSE(simd8<bool>(false).any());
  ASSERT_TRUE(simd8<bool>(true).any());
  const simd8<uint8_t> index(byte_index);
  for (size_t i = 0; i < width; i++) {
    const simd8<bool> only_i = index == simd8<uint8_t>(uint8_t(i));
    ASSERT_TRUE(only_i.any());
    ASSERT_TRUE((~only_i).any());
  }
  TEST_SUCCEED();
}
#endif // TEST_SIMD8

int main() {
#if TEST_SIMD8
  std::cout << "Testing the " << simdjson::builtin_implementation()->name()
            << " SIMD layer" << std::endl;
  if (!test_simd8_bool_any()) {
    return EXIT_FAILURE;
  }
#else
  std::cout << "No SIMD layer in the builtin implementation, skipping."
            << std::endl;
#endif
  std::cout << "All tests passed!" << std::endl;
  return EXIT_SUCCESS;
}
