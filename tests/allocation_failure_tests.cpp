// Checks that a parser survives a failed allocation: allocate() returns
// MEMALLOC, the parser keeps its previous capacity and depth, and it can still
// parse documents. Failures are injected by replacing the array forms of
// operator new and making one allocation of a chosen size return nullptr.
#include "simdjson.h"
#include "test_macros.h"
#include "test_main.h"

#include <cstdlib>
#include <iostream>
#include <new>

namespace {
// When non-zero, the next nothrow new[] of exactly this many bytes fails.
size_t fail_size = 0;
// Number of allocations made to fail so far.
size_t injected = 0;
} // namespace

void *operator new[](size_t n, const std::nothrow_t &) noexcept {
  if (fail_size != 0 && n == fail_size) {
    fail_size = 0;
    injected++;
    return nullptr;
  }
  return std::malloc(n == 0 ? 1 : n);
}
void *operator new[](size_t n) {
  void *p = std::malloc(n == 0 ? 1 : n);
  if (p == nullptr) {
#if __cpp_exceptions
    throw std::bad_alloc();
#else
    std::abort();
#endif
  }
  return p;
}
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete[](void *p, size_t) noexcept { std::free(p); }
void operator delete[](void *p, const std::nothrow_t &) noexcept { std::free(p); }

using namespace simdjson;

namespace allocation_failure_tests {

// Bytes requested for the stage 1 structural index buffer of a given capacity.
size_t structural_bytes(size_t capacity) {
  return (SIMDJSON_ROUNDUP_N(capacity, 64) + 9) * sizeof(uint32_t);
}
// Bytes requested for the On-Demand string buffer of a given capacity.
size_t ondemand_string_bytes(size_t capacity) {
  return SIMDJSON_ROUNDUP_N(5 * (capacity / 3) + SIMDJSON_PADDING, 64);
}

// Arms the injection and runs allocate(); if the allocation never reached our
// operator new (e.g. simdjson is a DLL with its own allocator), tells the
// caller to skip the check.
template <typename P>
bool allocate_with_failure(P &parser, size_t capacity, size_t depth, size_t bytes, error_code &err) {
  size_t before = injected;
  fail_size = bytes;
  err = parser.allocate(capacity, depth);
  fail_size = 0;
  if (injected == before) {
    std::cout << "  injection did not fire, skipping" << std::endl;
    return false;
  }
  return true;
}

bool sum_ondemand(ondemand::parser &parser, int64_t &sum) {
  auto json = "[1,2,3]"_padded;
  ondemand::document doc;
  ASSERT_SUCCESS( parser.iterate(json).get(doc) );
  ondemand::array array;
  ASSERT_SUCCESS( doc.get_array().get(array) );
  sum = 0;
  for (auto value : array) {
    int64_t x;
    ASSERT_SUCCESS( value.get_int64().get(x) );
    sum += x;
  }
  return true;
}

bool sum_dom(dom::parser &parser, int64_t &sum) {
  auto json = "[1,2,3]"_padded;
  dom::array array;
  ASSERT_SUCCESS( parser.parse(json).get_array().get(array) );
  sum = 0;
  for (dom::element val : array) {
    int64_t x;
    ASSERT_SUCCESS( val.get_int64().get(x) );
    sum += x;
  }
  return true;
}

bool ondemand_structural_indexes_failure() {
  TEST_START();
  ondemand::parser parser;
  ASSERT_SUCCESS( parser.allocate(1024, 64) );
  error_code err;
  if (allocate_with_failure(parser, 4096, 64, structural_bytes(4096), err)) {
    ASSERT_ERROR( err, MEMALLOC );
    ASSERT_EQUAL( parser.capacity(), 1024 );
    ASSERT_EQUAL( parser.max_depth(), 64 );
    int64_t sum;
    if (!sum_ondemand(parser, sum)) { return false; }
    ASSERT_EQUAL( sum, 6 );
  }
  ASSERT_SUCCESS( parser.allocate(4096, 64) );
  ASSERT_EQUAL( parser.capacity(), 4096 );
  TEST_SUCCEED();
}

bool ondemand_string_buffer_failure() {
  TEST_START();
  ondemand::parser parser;
  ASSERT_SUCCESS( parser.allocate(1024, 64) );
  error_code err;
  if (allocate_with_failure(parser, 4096, 64, ondemand_string_bytes(4096), err)) {
    ASSERT_ERROR( err, MEMALLOC );
    ASSERT_EQUAL( parser.capacity(), 1024 );
    ASSERT_EQUAL( parser.max_depth(), 64 );
    int64_t sum;
    if (!sum_ondemand(parser, sum)) { return false; }
    ASSERT_EQUAL( sum, 6 );
  }
  TEST_SUCCEED();
}

bool ondemand_fresh_parser_failure() {
  TEST_START();
  ondemand::parser parser;
  error_code err;
  if (allocate_with_failure(parser, 4096, 64, structural_bytes(4096), err)) {
    ASSERT_ERROR( err, MEMALLOC );
    ASSERT_EQUAL( parser.capacity(), 0 );
    int64_t sum;
    if (!sum_ondemand(parser, sum)) { return false; }
    ASSERT_EQUAL( sum, 6 );
  }
  TEST_SUCCEED();
}

bool ondemand_depth_failure() {
  TEST_START();
  ondemand::parser parser;
  ASSERT_SUCCESS( parser.allocate(1024, 64) );
  // 777 bools: the is_array stack is the only allocation of that size.
  error_code err;
  if (allocate_with_failure(parser, 1024, 777, 777 * sizeof(bool), err)) {
    ASSERT_ERROR( err, MEMALLOC );
    ASSERT_EQUAL( parser.capacity(), 1024 );
    ASSERT_EQUAL( parser.max_depth(), 64 );
    int64_t sum;
    if (!sum_ondemand(parser, sum)) { return false; }
    ASSERT_EQUAL( sum, 6 );
  }
  TEST_SUCCEED();
}

bool dom_structural_indexes_failure() {
  TEST_START();
  dom::parser parser;
  ASSERT_SUCCESS( parser.allocate(1024, 64) );
  error_code err;
  if (allocate_with_failure(parser, 4096, 64, structural_bytes(4096), err)) {
    ASSERT_ERROR( err, MEMALLOC );
    ASSERT_EQUAL( parser.capacity(), 1024 );
    ASSERT_EQUAL( parser.max_depth(), 64 );
    int64_t sum;
    if (!sum_dom(parser, sum)) { return false; }
    ASSERT_EQUAL( sum, 6 );
  }
  TEST_SUCCEED();
}

bool dom_depth_failure() {
  TEST_START();
  dom::parser parser;
  ASSERT_SUCCESS( parser.allocate(1024, 64) );
  error_code err;
  if (allocate_with_failure(parser, 1024, 777, 777 * sizeof(bool), err)) {
    ASSERT_ERROR( err, MEMALLOC );
    ASSERT_EQUAL( parser.capacity(), 1024 );
    ASSERT_EQUAL( parser.max_depth(), 64 );
    int64_t sum;
    if (!sum_dom(parser, sum)) { return false; }
    ASSERT_EQUAL( sum, 6 );
  }
  TEST_SUCCEED();
}

bool run() {
  return ondemand_structural_indexes_failure() &&
         ondemand_string_buffer_failure() &&
         ondemand_fresh_parser_failure() &&
         ondemand_depth_failure() &&
         dom_structural_indexes_failure() &&
         dom_depth_failure();
}

} // namespace allocation_failure_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, allocation_failure_tests::run);
}
