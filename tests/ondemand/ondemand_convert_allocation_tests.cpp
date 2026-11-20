// simdjson::from<T> parses some types without On Demand. An allocation failure
// there must reach the caller as std::bad_alloc, as it does through On Demand,
// instead of terminating the program. Large allocations are made to fail by
// replacing the global allocation functions.
#include "simdjson.h"
#include "test_ondemand.h"

#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include <vector>

#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
// Allocations larger than this many bytes fail. Volatile: GCC otherwise
// propagates it through the inlined operator new into false -Warray-bounds.
static volatile std::size_t allocation_limit = SIZE_MAX;

static void *test_allocate(std::size_t size) noexcept {
  if (size > allocation_limit) {
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

namespace convert_allocation_tests {

constexpr std::size_t LIMIT = 1 << 16;

// Whether the replaced operator new also serves the standard containers. It
// does not when the standard library allocates from its own DLL (MinGW).
// The probe writes to a global so that the allocation cannot be elided.
static std::vector<char> probe;
static bool allocations_are_replaced() {
  allocation_limit = LIMIT;
  bool replaced = false;
  try {
    probe.resize(2 * LIMIT);
  } catch (const std::bad_alloc &) {
    replaced = true;
  }
  allocation_limit = SIZE_MAX;
  std::vector<char>().swap(probe);
  return replaced;
}

// Parses json as T with allocations above LIMIT failing, and reports whether
// std::bad_alloc reached the caller.
template <typename T>
static bool throws_bad_alloc(const simdjson::padded_string &json) {
  bool thrown = false;
  allocation_limit = LIMIT;
  try {
    T value;
    simdjson::error_code error = simdjson::from(json).get(value);
    (void)error;
  } catch (const std::bad_alloc &) {
    thrown = true;
  }
  allocation_limit = SIZE_MAX;
  return thrown;
}

bool direct_parse_allocation_failure() {
  TEST_START();
  if (!allocations_are_replaced()) {
    std::cout << "  skipped: the standard library does not use the replaced operator new" << std::endl;
    TEST_SUCCEED();
  }
  std::string ints = "[";
  std::string doubles = "[";
  std::string strings = "[";
  for (std::size_t i = 0; i < 4 * LIMIT / 8; i++) {
    if (i) { ints += ","; doubles += ","; strings += ","; }
    ints += std::to_string(i % 10);
    doubles += "1.5";
    strings += "\"ab\"";
  }
  ints += "]";
  doubles += "]";
  strings += "]";
  // The On Demand parser itself is allocated up front so that only the
  // container fails.
  simdjson::padded_string ints_json(ints), doubles_json(doubles), strings_json(strings);
  std::string long_string = "\"";
  long_string.append(2 * LIMIT, 'a');
  long_string.append("\"");
  simdjson::padded_string long_string_json(long_string);
  ASSERT_SUCCESS(simdjson::ondemand::parser::get_parser().allocate(strings_json.size()));
  ASSERT_TRUE(throws_bad_alloc<std::vector<int64_t>>(ints_json));
  ASSERT_TRUE(throws_bad_alloc<std::vector<double>>(doubles_json));
  ASSERT_TRUE(throws_bad_alloc<std::vector<std::string>>(strings_json));
  ASSERT_TRUE(throws_bad_alloc<std::string>(long_string_json));
  TEST_SUCCEED();
}

bool run() {
  return direct_parse_allocation_failure() &&
         true;
}

} // namespace convert_allocation_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, convert_allocation_tests::run);
}
#else
int main() { return 0; }
#endif // SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
