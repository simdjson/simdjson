#include "simdjson.h"
#include "test_macros.h"
#include "test_main.h"

#include <limits>

namespace arithmetic_overflow_tests {

using simdjson::internal::add_overflow;
using simdjson::internal::dom_string_capacity;
using simdjson::internal::multiply_overflow;
using simdjson::internal::roundup_64_overflow;

bool helper_add_overflow_boundaries() {
  TEST_START();
  size_t out = 0;
  ASSERT_FALSE(add_overflow(size_t(10), size_t(20), out));
  ASSERT_EQUAL(out, size_t(30));

  const size_t max = (std::numeric_limits<size_t>::max)();
  ASSERT_FALSE(add_overflow(max - size_t(3), size_t(3), out));
  ASSERT_EQUAL(out, max);
  ASSERT_TRUE(add_overflow(max - size_t(2), size_t(3), out));
  TEST_SUCCEED();
}

bool helper_multiply_overflow_boundaries() {
  TEST_START();
  size_t out = 0;
  ASSERT_FALSE(multiply_overflow(size_t(123), size_t(456), out));
  ASSERT_EQUAL(out, size_t(56088));

  const size_t max = (std::numeric_limits<size_t>::max)();
  ASSERT_FALSE(multiply_overflow(max / size_t(5), size_t(5), out));
  ASSERT_EQUAL(out, (max / size_t(5)) * size_t(5));
  ASSERT_TRUE(multiply_overflow((max / size_t(5)) + size_t(1), size_t(5), out));
  TEST_SUCCEED();
}

bool helper_roundup_overflow_and_compatibility() {
  TEST_START();
  size_t out = 0;

  // Safe boundary and exact compatibility with the historical formula.
  const size_t safe_boundary = (std::numeric_limits<size_t>::max)() - size_t(63);
  ASSERT_FALSE(roundup_64_overflow(safe_boundary, out));
  ASSERT_EQUAL(out, safe_boundary);

  // Overflow boundary: this was previously wrapped by unsigned arithmetic.
  ASSERT_TRUE(roundup_64_overflow(safe_boundary + size_t(1), out));

  // Prove old expression would wrap for this input.
  const size_t old_wrapped = (safe_boundary + size_t(1) + size_t(63)) & ~size_t(63);
  ASSERT_EQUAL(old_wrapped, size_t(0));
  TEST_SUCCEED();
}

bool helper_dom_string_capacity_boundaries() {
  TEST_START();
  size_t out = 0;

  // Preserves historical formula when safe.
  const size_t safe_cap = size_t(1024);
  ASSERT_FALSE(dom_string_capacity(safe_cap, out));
  const size_t expected = ((size_t(5) * (safe_cap / size_t(3)) + simdjson::SIMDJSON_PADDING + size_t(63)) & ~size_t(63));
  ASSERT_EQUAL(out, expected);

  // Exact overflow boundary:
  // dom_string_capacity computes roundup_64(5 * (capacity / 3) + SIMDJSON_PADDING).
  // Let q = capacity / 3. For the rounded value to stay in range:
  //   5*q + SIMDJSON_PADDING + 63 <= SIZE_MAX
  // so:
  //   q <= floor((SIZE_MAX - SIMDJSON_PADDING - 63) / 5)
  const size_t max = (std::numeric_limits<size_t>::max)();
  const size_t qmax = (max - simdjson::SIMDJSON_PADDING - size_t(63)) / size_t(5);
  const size_t largest_safe_capacity = qmax * size_t(3) + size_t(2);
  const size_t first_overflow_capacity = largest_safe_capacity + size_t(1);

  ASSERT_FALSE(dom_string_capacity(largest_safe_capacity, out));
  ASSERT_TRUE(dom_string_capacity(first_overflow_capacity, out));
  TEST_SUCCEED();
}

bool run() {
  return helper_add_overflow_boundaries() &&
         helper_multiply_overflow_boundaries() &&
         helper_roundup_overflow_and_compatibility() &&
         helper_dom_string_capacity_boundaries();
}

} // namespace arithmetic_overflow_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, arithmetic_overflow_tests::run);
}
