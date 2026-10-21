// string_builder::move_to() copies a small result out. This test makes that
// copy fail, by replacing the global allocation functions, and checks that
// move_to() reports MEMALLOC instead of terminating the program.
#include "simdjson.h"
#include "test_builder.h"
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>

#if SIMDJSON_EXCEPTIONS
// When set, the next allocation fails with std::bad_alloc.
static bool fail_next_allocation = false;

static void *test_allocate(std::size_t size) noexcept {
  if (fail_next_allocation) {
    fail_next_allocation = false;
    return nullptr;
  }
  return std::malloc(size == 0 ? 1 : size);
}

#if defined(__GNUC__) && !defined(__clang__)
// GCC pairs the library's new and delete calls with these definitions after
// inlining and reports malloc/free as mismatched.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif
void *operator new(std::size_t size) {
  if (void *p = test_allocate(size)) {
    return p;
  }
  throw std::bad_alloc();
}
void *operator new[](std::size_t size) {
  if (void *p = test_allocate(size)) {
    return p;
  }
  throw std::bad_alloc();
}
void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
  return test_allocate(size);
}
void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
  return test_allocate(size);
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
void operator delete(void *p, const std::nothrow_t &) noexcept { std::free(p); }
void operator delete[](void *p, const std::nothrow_t &) noexcept { std::free(p); }
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
#endif // SIMDJSON_EXCEPTIONS

namespace builder_tests {

#if SIMDJSON_EXCEPTIONS
// Whether the replaced operator new also serves std::string. It does not
// when the standard library allocates from its own DLL (MinGW).
// The probe writes to a global so that the allocation cannot be elided.
static std::string probe;
static bool string_allocations_are_replaced() {
  fail_next_allocation = true;
  bool replaced = false;
  try {
    probe.assign(100, 'x');
  } catch (const std::bad_alloc &) {
    replaced = true;
  }
  fail_next_allocation = false;
  std::string().swap(probe);
  return replaced;
}

bool move_to_small_result_allocation_failure() {
  TEST_START();
  if (!string_allocations_are_replaced()) {
    std::cout << "  skipped: std::string does not use the replaced operator new" << std::endl;
    TEST_SUCCEED();
  }
  // A failed copy leaves out and the builder unchanged.
  simdjson::builder::string_builder sb;
  const std::string expected(100, 'x'); // too long for the small-string buffer
  sb.append_raw(expected);
  std::string out;
  fail_next_allocation = true;
  simdjson::error_code e = sb.move_to(out);
  fail_next_allocation = false;
  ASSERT_EQUAL(e, simdjson::MEMALLOC);
  ASSERT_TRUE(out.empty());
  ASSERT_EQUAL(sb.size(), expected.size());
  ASSERT_SUCCESS(sb.move_to(out));
  ASSERT_EQUAL(out, expected);
  TEST_SUCCEED();
}
#endif // SIMDJSON_EXCEPTIONS

bool run() {
  return
#if SIMDJSON_EXCEPTIONS
         move_to_small_result_allocation_failure() &&
#endif
         true;
}

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, builder_tests::run);
}
