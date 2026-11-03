#include "simdjson.h"
#include "test_macros.h"

#include <cstdint>
#include <cstdlib>

// Tests for the SIMD layer of the builtin implementation. The fallback kernel
// has no SIMD layer, so there is nothing to test.
#if SIMDJSON_BUILTIN_IMPLEMENTATION_IS(fallback)
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

static bool test_simd8_uint8_order() {
  TEST_START();
  const simd8<uint8_t> low(10);
  const simd8<uint8_t> high(20);
  ASSERT_TRUE((low < high).any());
  ASSERT_FALSE((high < low).any());
  ASSERT_TRUE((high > low).any());
  ASSERT_FALSE((low > high).any());
  ASSERT_TRUE((low <= high).any());
  ASSERT_FALSE((high <= low).any());
  ASSERT_TRUE((high >= low).any());
  ASSERT_FALSE((low >= high).any());
  ASSERT_TRUE((low <= low).any());
  ASSERT_TRUE((low >= low).any());
  ASSERT_FALSE((low < low).any());
  ASSERT_FALSE((low > low).any());

  const simd8<uint8_t> index(byte_index);
  for (size_t i = 0; i < width; i++) {
    const simd8<uint8_t> threshold = simd8<uint8_t>(uint8_t(i));
    const simd8<bool> lt = index < threshold;
    const simd8<bool> gt = index > threshold;
    const simd8<bool> le = index <= threshold;
    const simd8<bool> ge = index >= threshold;

    if (i == 0) {
      ASSERT_FALSE(lt.any());
      ASSERT_TRUE(ge.any());
    } else {
      ASSERT_TRUE(lt.any());
    }

    if (i == width - 1) {
      ASSERT_FALSE(gt.any());
      ASSERT_TRUE(le.any());
    } else {
      ASSERT_TRUE(gt.any());
    }
  }
  TEST_SUCCEED();
}

static bool test_simd8_bit_andnot() {
  TEST_START();
  const simd8<uint8_t> all_zeros(uint8_t(0));
  const simd8<uint8_t> all_ones(uint8_t(0xFF));
  const simd8<uint8_t> mask_f0(uint8_t(0xF0));
  const simd8<uint8_t> mask_0f(uint8_t(0x0F));

  ASSERT_FALSE((~(all_ones.bit_andnot(all_zeros) == all_ones)).any());
  ASSERT_FALSE((~(all_zeros.bit_andnot(all_ones) == all_zeros)).any());
  ASSERT_FALSE((~(all_ones.bit_andnot(all_ones) == all_zeros)).any());
  ASSERT_FALSE((~(all_zeros.bit_andnot(all_zeros) == all_zeros)).any());

  ASSERT_FALSE((~(mask_f0.bit_andnot(mask_0f) == mask_f0)).any());
  ASSERT_FALSE((~(mask_0f.bit_andnot(mask_f0) == mask_0f)).any());
  ASSERT_FALSE((~(mask_f0.bit_andnot(all_ones) == all_zeros)).any());
  ASSERT_FALSE((~(mask_f0.bit_andnot(all_zeros) == mask_f0)).any());

  const simd8<bool> t(true);
  const simd8<bool> f(false);
  ASSERT_FALSE((t.bit_andnot(f) ^ t).any());
  ASSERT_FALSE((f.bit_andnot(t) ^ f).any());
  ASSERT_FALSE((t.bit_andnot(t) ^ f).any());
  ASSERT_FALSE((f.bit_andnot(f) ^ f).any());

  TEST_SUCCEED();
}
#endif // TEST_SIMD8

int main() {
#if TEST_SIMD8
  std::cout << "Testing the " << simdjson::builtin_implementation()->name()
            << " SIMD layer" << std::endl;
  if (!test_simd8_bool_any() || !test_simd8_uint8_order() || !test_simd8_bit_andnot()) {
    return EXIT_FAILURE;
  }
#else
  std::cout << "No SIMD layer in the builtin implementation, skipping."
            << std::endl;
#endif
  std::cout << "All tests passed!" << std::endl;
  return EXIT_SUCCESS;
}
