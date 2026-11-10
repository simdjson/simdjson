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

static const int8_t signed_byte_index[64] = {
    -32, -31, -30, -29, -28, -27, -26, -25, -24, -23, -22, -21, -20, -19, -18, -17,
    -16, -15, -14, -13, -12, -11, -10,  -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1,
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
     16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31};
static_assert(width <= sizeof(signed_byte_index), "a register is at most 64 bytes");

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

  ASSERT_FALSE((~(low.min_val(high) == low)).any());
  ASSERT_FALSE((~(high.min_val(low) == low)).any());
  ASSERT_FALSE((~(low.max_val(high) == high)).any());
  ASSERT_FALSE((~(high.max_val(low) == high)).any());
  ASSERT_FALSE((~(low.min_val(low) == low)).any());
  ASSERT_FALSE((~(low.max_val(low) == low)).any());

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

static bool test_simd8_int8_order() {
  TEST_START();
  const simd8<int8_t> low(-20);
  const simd8<int8_t> high(10);
  ASSERT_TRUE((low < high).any());
  ASSERT_FALSE((high < low).any());
  ASSERT_TRUE((high > low).any());
  ASSERT_FALSE((low > high).any());
  ASSERT_FALSE((low < low).any());
  ASSERT_FALSE((low > low).any());

  ASSERT_FALSE((~(low.min_val(high) == low)).any());
  ASSERT_FALSE((~(high.min_val(low) == low)).any());
  ASSERT_FALSE((~(low.max_val(high) == high)).any());
  ASSERT_FALSE((~(high.max_val(low) == high)).any());
  ASSERT_FALSE((~(low.min_val(low) == low)).any());
  ASSERT_FALSE((~(low.max_val(low) == low)).any());

  const simd8<int8_t> index(signed_byte_index);
  for (size_t i = 0; i < width; i++) {
    const simd8<int8_t> threshold = simd8<int8_t>(signed_byte_index[i]);
    const simd8<bool> lt = index < threshold;
    const simd8<bool> gt = index > threshold;
    const simd8<bool> eq = index == threshold;

    ASSERT_TRUE(eq.any());
    if (i == 0) {
      ASSERT_FALSE(lt.any());
    } else {
      ASSERT_TRUE(lt.any());
    }

    if (i == width - 1) {
      ASSERT_FALSE(gt.any());
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
  ASSERT_FALSE((~(t == t)).any());
  ASSERT_FALSE((t == f).any());
  ASSERT_FALSE((f == t).any());
  ASSERT_FALSE((~(f == f)).any());

  ASSERT_FALSE((~(t.bit_andnot(f) == t)).any());
  ASSERT_FALSE((~(f.bit_andnot(t) == f)).any());
  ASSERT_FALSE((~(t.bit_andnot(t) == f)).any());
  ASSERT_FALSE((~(f.bit_andnot(f) == f)).any());

  TEST_SUCCEED();
}

static bool test_simd8_is_ascii() {
  TEST_START();
  const simd8<uint8_t> index(byte_index);
  ASSERT_TRUE(index.is_ascii());
  ASSERT_TRUE(simd8<uint8_t>(simd8<uint8_t>::zero()).is_ascii());
  ASSERT_TRUE(simd8<uint8_t>(uint8_t(127)).is_ascii());
  ASSERT_FALSE(simd8<uint8_t>(uint8_t(128)).is_ascii());
  ASSERT_FALSE(simd8<uint8_t>(uint8_t(255)).is_ascii());

  uint8_t non_ascii[64];
  for (size_t i = 0; i < 64; i++) {
    non_ascii[i] = byte_index[i];
  }
  for (size_t i = 0; i < width; i++) {
    non_ascii[i] |= 0x80;
    const simd8<uint8_t> with_high_bit(non_ascii);
    ASSERT_FALSE(with_high_bit.is_ascii());
    non_ascii[i] &= ~0x80;
  }
  TEST_SUCCEED();
}

static bool test_simd8_bits_anywhere() {
  TEST_START();
  const simd8<uint8_t> zero = simd8<uint8_t>::zero();
  ASSERT_TRUE(zero.bits_not_set_anywhere());
  ASSERT_FALSE(zero.any_bits_set_anywhere());

  const simd8<uint8_t> mask_f0(uint8_t(0xF0));
  const simd8<uint8_t> mask_0f(uint8_t(0x0F));
  ASSERT_TRUE(mask_f0.bits_not_set_anywhere(mask_0f));
  ASSERT_FALSE(mask_f0.any_bits_set_anywhere(mask_0f));
  ASSERT_FALSE(mask_f0.bits_not_set_anywhere(mask_f0));
  ASSERT_TRUE(mask_f0.any_bits_set_anywhere(mask_f0));

  for (size_t i = 0; i < width; i++) {
    uint8_t single_byte[64] = {0};
    single_byte[i] = 1;
    const simd8<uint8_t> single(single_byte);
    ASSERT_FALSE(single.bits_not_set_anywhere());
    ASSERT_TRUE(single.any_bits_set_anywhere());
  }
  TEST_SUCCEED();
}

static bool test_simd8_bits_elementwise() {
  TEST_START();
  const simd8<uint8_t> zero = simd8<uint8_t>::zero();
  ASSERT_FALSE((~zero.bits_not_set()).any());
  ASSERT_FALSE(zero.any_bits_set().any());

  const simd8<uint8_t> all_ones = simd8<uint8_t>(uint8_t(0xFF));
  ASSERT_FALSE(all_ones.bits_not_set().any());
  ASSERT_FALSE((~all_ones.any_bits_set()).any());

  const simd8<uint8_t> mask_f0(uint8_t(0xF0));
  const simd8<uint8_t> mask_0f(uint8_t(0x0F));
  ASSERT_FALSE((~mask_f0.bits_not_set(mask_0f)).any());
  ASSERT_FALSE(mask_f0.any_bits_set(mask_0f).any());
  ASSERT_FALSE(mask_f0.bits_not_set(mask_f0).any());
  ASSERT_FALSE((~mask_f0.any_bits_set(mask_f0)).any());

  const simd8<uint8_t> index(byte_index);
  for (size_t i = 0; i < width; i++) {
    uint8_t single_byte[64] = {0};
    single_byte[i] = 1;
    const simd8<uint8_t> single(single_byte);
    const simd8<bool> not_set = single.bits_not_set();
    const simd8<bool> any_set = single.any_bits_set();

    ASSERT_TRUE(any_set.any());
    ASSERT_TRUE(not_set.any());

    const simd8<bool> is_i = index == simd8<uint8_t>(uint8_t(i));
    ASSERT_FALSE((~(any_set == is_i)).any());
    ASSERT_FALSE((~(not_set == ~is_i)).any());
    ASSERT_FALSE((~(single.any_bits_set(simd8<uint8_t>(1)) == is_i)).any());
    ASSERT_FALSE((~(single.bits_not_set(simd8<uint8_t>(1)) == ~is_i)).any());
    ASSERT_FALSE(single.any_bits_set(simd8<uint8_t>(2)).any());
    ASSERT_FALSE((~single.bits_not_set(simd8<uint8_t>(2))).any());
  }
  TEST_SUCCEED();
}
#endif // TEST_SIMD8

int main() {
#if TEST_SIMD8
  std::cout << "Testing the " << simdjson::builtin_implementation()->name()
            << " SIMD layer" << std::endl;
  if (!test_simd8_bool_any() || !test_simd8_uint8_order() || !test_simd8_int8_order() ||
      !test_simd8_bit_andnot() || !test_simd8_is_ascii() || !test_simd8_bits_anywhere() ||
      !test_simd8_bits_elementwise()) {
    return EXIT_FAILURE;
  }
#else
  std::cout << "No SIMD layer in the builtin implementation, skipping."
            << std::endl;
#endif
  std::cout << "All tests passed!" << std::endl;
  return EXIT_SUCCESS;
}
