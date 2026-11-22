// Checks parser::set_allocator(): the buffers that grow with the document go
// through the buffer_allocator, are freed through it with the size that was
// requested, and a failed allocation leaves a usable parser.
#include "simdjson.h"
#include "test_macros.h"
#include "test_main.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

using namespace simdjson;

namespace buffer_allocator_tests {

struct counting_allocator {
  std::map<void *, size_t> live{};
  size_t allocations{0};
  size_t deallocations{0};
  size_t bad_frees{0};
  // When non-zero, the allocation with this number (counting from 1) fails.
  size_t fail_at{0};
  size_t attempts{0};
  buffer_allocator hook{};

  counting_allocator() noexcept {
    hook.allocate = &allocate;
    hook.deallocate = &deallocate;
    hook.context = this;
  }
  size_t live_bytes() const {
    size_t total = 0;
    for (const auto &entry : live) { total += entry.second; }
    return total;
  }
  static void *allocate(void *context, size_t size) noexcept {
    counting_allocator *self = static_cast<counting_allocator *>(context);
    self->attempts++;
    if (self->fail_at != 0 && self->attempts == self->fail_at) {
      return nullptr;
    }
    void *p = std::malloc(size == 0 ? 1 : size);
    if (p == nullptr) { return nullptr; }
    self->live[p] = size;
    self->allocations++;
    return p;
  }
  static void deallocate(void *context, void *pointer, size_t size) noexcept {
    counting_allocator *self = static_cast<counting_allocator *>(context);
    auto it = self->live.find(pointer);
    if (it == self->live.end() || it->second != size) {
      self->bad_frees++;
    } else {
      self->live.erase(it);
    }
    self->deallocations++;
    std::free(pointer);
  }
};

padded_string array_of_ints(size_t count) {
  std::string json = "[";
  for (size_t i = 0; i < count; i++) {
    if (i > 0) { json.append(","); }
    json.append("1");
  }
  json.append("]");
  return padded_string(json);
}

bool sum_ondemand(ondemand::parser &parser, const padded_string &json, int64_t &sum) {
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

bool sum_dom(dom::parser &parser, const padded_string &json, int64_t &sum) {
  dom::array array;
  ASSERT_SUCCESS( parser.parse(json).get_array().get(array) );
  sum = 0;
  for (dom::element value : array) {
    int64_t x;
    ASSERT_SUCCESS( value.get_int64().get(x) );
    sum += x;
  }
  return true;
}

bool ondemand_counting() {
  TEST_START();
  counting_allocator a;
  {
    ondemand::parser parser;
    parser.set_allocator(&a.hook);
    ASSERT_TRUE( parser.get_allocator() == &a.hook );
    int64_t sum;
    padded_string big = array_of_ints(1000);
    if (!sum_ondemand(parser, big, sum)) { return false; }
    ASSERT_EQUAL( sum, 1000 );
    // The string buffer and the structural indexes.
    ASSERT_EQUAL( a.allocations, 2 );
    size_t live = a.live_bytes();
    ASSERT_TRUE( live > 0 );
    // A reused parser does not allocate for smaller documents.
    padded_string small_doc = array_of_ints(10);
    for (int i = 0; i < 10; i++) {
      if (!sum_ondemand(parser, small_doc, sum)) { return false; }
      ASSERT_EQUAL( sum, 10 );
    }
    ASSERT_EQUAL( a.allocations, 2 );
    // Growing replaces both buffers.
    padded_string bigger = array_of_ints(10000);
    if (!sum_ondemand(parser, bigger, sum)) { return false; }
    ASSERT_EQUAL( sum, 10000 );
    ASSERT_EQUAL( a.allocations, 4 );
    ASSERT_EQUAL( a.deallocations, 2 );
    ASSERT_TRUE( a.live_bytes() > live );
  }
  ASSERT_EQUAL( a.live.size(), 0 );
  ASSERT_EQUAL( a.bad_frees, 0 );
  TEST_SUCCEED();
}

bool ondemand_failure_leaves_parser_usable() {
  TEST_START();
  for (size_t fail_at = 1; fail_at <= 2; fail_at++) {
    counting_allocator a;
    {
      ondemand::parser parser;
      parser.set_allocator(&a.hook);
      ASSERT_SUCCESS( parser.allocate(1024, 64) );
      a.fail_at = a.attempts + fail_at;
      ASSERT_ERROR( parser.allocate(65536, 64), MEMALLOC );
      ASSERT_EQUAL( parser.capacity(), 1024 );
      ASSERT_EQUAL( parser.max_depth(), 64 );
      int64_t sum;
      padded_string small_doc = array_of_ints(10);
      if (!sum_ondemand(parser, small_doc, sum)) { return false; }
      ASSERT_EQUAL( sum, 10 );
      a.fail_at = 0;
      ASSERT_SUCCESS( parser.allocate(65536, 64) );
      ASSERT_EQUAL( parser.capacity(), 65536 );
    }
    ASSERT_EQUAL( a.live.size(), 0 );
    ASSERT_EQUAL( a.bad_frees, 0 );
  }
  TEST_SUCCEED();
}

bool ondemand_fresh_parser_failure() {
  TEST_START();
  counting_allocator a;
  {
    ondemand::parser parser;
    parser.set_allocator(&a.hook);
    a.fail_at = 1;
    ondemand::document doc;
    padded_string json = array_of_ints(10);
    ASSERT_ERROR( parser.iterate(json).get(doc), MEMALLOC );
    ASSERT_EQUAL( parser.capacity(), 0 );
    a.fail_at = 0;
    int64_t sum;
    if (!sum_ondemand(parser, json, sum)) { return false; }
    ASSERT_EQUAL( sum, 10 );
  }
  ASSERT_EQUAL( a.live.size(), 0 );
  ASSERT_EQUAL( a.bad_frees, 0 );
  TEST_SUCCEED();
}

bool ondemand_set_after_first_allocation() {
  TEST_START();
  counting_allocator a;
  {
    ondemand::parser parser;
    ASSERT_SUCCESS( parser.allocate(1024, 64) );
    parser.set_allocator(&a.hook);
    int64_t sum;
    padded_string small_doc = array_of_ints(10);
    if (!sum_ondemand(parser, small_doc, sum)) { return false; }
    // The buffers allocated with new[] are kept.
    ASSERT_EQUAL( a.allocations, 0 );
    padded_string big = array_of_ints(10000);
    if (!sum_ondemand(parser, big, sum)) { return false; }
    ASSERT_EQUAL( sum, 10000 );
    ASSERT_EQUAL( a.allocations, 2 );
    // The new[] buffers were not handed to the allocator.
    ASSERT_EQUAL( a.deallocations, 0 );
  }
  ASSERT_EQUAL( a.live.size(), 0 );
  ASSERT_EQUAL( a.bad_frees, 0 );
  TEST_SUCCEED();
}

bool ondemand_move() {
  TEST_START();
  counting_allocator a;
  {
    ondemand::parser moved_to;
    {
      ondemand::parser parser;
      parser.set_allocator(&a.hook);
      ASSERT_SUCCESS( parser.allocate(4096, 64) );
      moved_to = std::move(parser);
    }
    ASSERT_EQUAL( a.live.size(), 2 );
    ASSERT_TRUE( moved_to.get_allocator() == &a.hook );
    int64_t sum;
    padded_string json = array_of_ints(100);
    if (!sum_ondemand(moved_to, json, sum)) { return false; }
    ASSERT_EQUAL( sum, 100 );
    ASSERT_EQUAL( a.allocations, 2 );
  }
  ASSERT_EQUAL( a.live.size(), 0 );
  ASSERT_EQUAL( a.bad_frees, 0 );
  TEST_SUCCEED();
}

bool ondemand_stream() {
  TEST_START();
  counting_allocator a;
  {
    ondemand::parser parser;
    parser.set_allocator(&a.hook);
    std::string ndjson;
    for (int i = 0; i < 1000; i++) { ndjson.append("[1,2,3]\n"); }
    padded_string json(ndjson);
    ondemand::document_stream stream;
    ASSERT_SUCCESS( parser.iterate_many(json, 1024).get(stream) );
    size_t count = 0;
    for (auto doc : stream) {
      ondemand::array array;
      ASSERT_SUCCESS( doc.get_array().get(array) );
      count++;
    }
    ASSERT_EQUAL( count, 1000 );
    ASSERT_TRUE( parser.get_allocator() == &a.hook );
    ASSERT_TRUE( a.allocations >= 2 );
  }
  ASSERT_EQUAL( a.live.size(), 0 );
  ASSERT_EQUAL( a.bad_frees, 0 );
  TEST_SUCCEED();
}

bool dom_counting() {
  TEST_START();
  counting_allocator a;
  {
    dom::parser parser;
    parser.set_allocator(&a.hook);
    ASSERT_TRUE( parser.get_allocator() == &a.hook );
    int64_t sum;
    padded_string big = array_of_ints(1000);
    if (!sum_dom(parser, big, sum)) { return false; }
    ASSERT_EQUAL( sum, 1000 );
    // The tape, the string buffer and the structural indexes.
    ASSERT_EQUAL( a.allocations, 3 );
    padded_string small_doc = array_of_ints(10);
    for (int i = 0; i < 10; i++) {
      if (!sum_dom(parser, small_doc, sum)) { return false; }
    }
    ASSERT_EQUAL( a.allocations, 3 );
    // A document passed by the user gets its buffers from the parser allocator.
    dom::document doc;
    dom::element root;
    ASSERT_SUCCESS( parser.parse_into_document(doc, big).get(root) );
    ASSERT_EQUAL( a.allocations, 5 );
  }
  ASSERT_EQUAL( a.live.size(), 0 );
  ASSERT_EQUAL( a.bad_frees, 0 );
  TEST_SUCCEED();
}

bool dom_failure_leaves_parser_usable() {
  TEST_START();
  for (size_t fail_at = 1; fail_at <= 3; fail_at++) {
    counting_allocator a;
    {
      dom::parser parser;
      parser.set_allocator(&a.hook);
      int64_t sum;
      padded_string small_doc = array_of_ints(10);
      if (!sum_dom(parser, small_doc, sum)) { return false; }
      a.fail_at = a.attempts + fail_at;
      padded_string big = array_of_ints(100000);
      dom::element root;
      ASSERT_ERROR( parser.parse(big).get(root), MEMALLOC );
      a.fail_at = 0;
      if (!sum_dom(parser, small_doc, sum)) { return false; }
      ASSERT_EQUAL( sum, 10 );
      if (!sum_dom(parser, big, sum)) { return false; }
      ASSERT_EQUAL( sum, 100000 );
    }
    ASSERT_EQUAL( a.live.size(), 0 );
    ASSERT_EQUAL( a.bad_frees, 0 );
  }
  TEST_SUCCEED();
}

bool dom_stream() {
  TEST_START();
  counting_allocator a;
  {
    dom::parser parser;
    parser.set_allocator(&a.hook);
    std::string ndjson;
    for (int i = 0; i < 1000; i++) { ndjson.append("[1,2,3]\n"); }
    padded_string json(ndjson);
    dom::document_stream stream;
    ASSERT_SUCCESS( parser.parse_many(json, 1024).get(stream) );
    size_t count = 0;
    for (auto doc : stream) {
      dom::array array;
      ASSERT_SUCCESS( doc.get_array().get(array) );
      count++;
    }
    ASSERT_EQUAL( count, 1000 );
    ASSERT_TRUE( parser.get_allocator() == &a.hook );
    ASSERT_TRUE( a.allocations >= 3 );
  }
  ASSERT_EQUAL( a.live.size(), 0 );
  ASSERT_EQUAL( a.bad_frees, 0 );
  TEST_SUCCEED();
}

bool run() {
  return ondemand_counting() &&
         ondemand_failure_leaves_parser_usable() &&
         ondemand_fresh_parser_failure() &&
         ondemand_set_after_first_allocation() &&
         ondemand_move() &&
         ondemand_stream() &&
         dom_counting() &&
         dom_failure_leaves_parser_usable() &&
         dom_stream();
}

} // namespace buffer_allocator_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, buffer_allocator_tests::run);
}
