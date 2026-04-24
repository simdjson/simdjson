#include "simdjson.h"
#include "test_macros.h"

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

using simdjson::allocator;
using simdjson::padded_string;

namespace {

// Counts bytes outstanding and total allocations. Uses malloc/free internally
// so we can interleave with the default allocator in the same test binary.
struct tracking_allocator : allocator {
  std::atomic<size_t> bytes_outstanding{0};
  std::atomic<size_t> total_allocs{0};
  std::atomic<size_t> total_frees{0};

  simdjson::allocation_result allocate(size_t size) override {
    if (size == 0) {
      ++total_allocs;
      return { std::malloc(1), 0 };
    }
    void* p = std::malloc(size);
    if (p == nullptr) { return { nullptr, 0 }; }
    bytes_outstanding += size;
    ++total_allocs;
    return { p, size };
  }

  void deallocate(void* ptr, size_t size) noexcept override {
    if (ptr == nullptr) { return; }
    bytes_outstanding -= size;
    ++total_frees;
    std::free(ptr);
  }
};

// An allocator that always returns nullptr so we can exercise MEMALLOC paths.
struct null_allocator : allocator {
  std::atomic<size_t> attempted{0};
  simdjson::allocation_result allocate(size_t) override {
    ++attempted;
    return { nullptr, 0 };
  }
  void deallocate(void*, size_t) noexcept override {}
};

// An allocator that returns successfully for the first `n_successes` calls and
// throws `std::bad_alloc` afterwards. Tracks deallocation counts so we can
// verify there is no leak on the exception path.
struct throwing_allocator : allocator {
  size_t n_successes;
  std::atomic<size_t> allocs{0};
  std::atomic<size_t> frees{0};
  std::atomic<size_t> bytes_outstanding{0};
  explicit throwing_allocator(size_t n) : n_successes(n) {}
  simdjson::allocation_result allocate(size_t size) override {
    if (allocs.fetch_add(1) >= n_successes) {
      throw std::bad_alloc{};
    }
    void* p = std::malloc(size == 0 ? 1 : size);
    if (p == nullptr) { return { nullptr, 0 }; }
    bytes_outstanding += size;
    return { p, size };
  }
  void deallocate(void* ptr, size_t size) noexcept override {
    if (!ptr) { return; }
    bytes_outstanding -= size;
    ++frees;
    std::free(ptr);
  }
};

// Always rounds the requested size up to the nearest 4 KiB so callers see
// real over-allocation through allocate_at_least semantics.
struct rounding_allocator : allocator {
  static constexpr size_t CHUNK = 4096;
  std::atomic<size_t> total_allocs{0};
  std::atomic<size_t> bytes_outstanding{0};

  simdjson::allocation_result allocate(size_t size) override {
    size_t rounded = (size + CHUNK - 1) & ~(CHUNK - 1);
    if (rounded == 0) { rounded = CHUNK; }
    void* p = std::malloc(rounded);
    if (p == nullptr) { return { nullptr, 0 }; }
    ++total_allocs;
    bytes_outstanding += rounded;
    return { p, rounded };
  }
  void deallocate(void* ptr, size_t size) noexcept override {
    if (!ptr) { return; }
    bytes_outstanding -= size;
    std::free(ptr);
  }
};

static const char *const SAMPLE_JSON = R"({
  "name": "simdjson",
  "stars": 42,
  "tags": ["json", "parser", "simd"],
  "nested": { "inner": true, "depth": 3 }
})";

// ---- dom tests ----

bool test_dom_default_allocator_matches() {
  TEST_START();
  simdjson::dom::parser parser;
  simdjson::dom::element elem;
  ASSERT_SUCCESS(parser.parse(SAMPLE_JSON, std::strlen(SAMPLE_JSON)).get(elem));
  int64_t stars;
  ASSERT_SUCCESS(elem["stars"].get(stars));
  ASSERT_EQUAL(stars, 42);
  TEST_SUCCEED();
}

bool test_dom_custom_allocator_basic() {
  TEST_START();
  tracking_allocator alloc;
  {
    simdjson::dom::parser parser(alloc);
    simdjson::dom::element elem;
    ASSERT_SUCCESS(parser.parse(SAMPLE_JSON, std::strlen(SAMPLE_JSON)).get(elem));
    int64_t stars;
    ASSERT_SUCCESS(elem["stars"].get(stars));
    ASSERT_EQUAL(stars, 42);
    ASSERT_TRUE(alloc.bytes_outstanding.load() > 0);
    ASSERT_TRUE(alloc.total_allocs.load() > 0);
  }
  ASSERT_EQUAL(alloc.bytes_outstanding.load(), size_t(0));
  ASSERT_EQUAL(alloc.total_allocs.load(), alloc.total_frees.load());
  TEST_SUCCEED();
}

bool test_dom_two_parsers_two_allocators() {
  TEST_START();
  tracking_allocator a1;
  tracking_allocator a2;
  {
    simdjson::dom::parser p1(a1);
    simdjson::dom::parser p2(a2);
    simdjson::dom::element e1, e2;
    ASSERT_SUCCESS(p1.parse(SAMPLE_JSON, std::strlen(SAMPLE_JSON)).get(e1));
    ASSERT_SUCCESS(p2.parse(SAMPLE_JSON, std::strlen(SAMPLE_JSON)).get(e2));
    // Both allocators must have non-zero traffic.
    ASSERT_TRUE(a1.total_allocs.load() > 0);
    ASSERT_TRUE(a2.total_allocs.load() > 0);
    // And neither should touch the other: if they swapped we'd see a delta
    // in one while the other stays zero, which can't happen when both run.
  }
  ASSERT_EQUAL(a1.bytes_outstanding.load(), size_t(0));
  ASSERT_EQUAL(a2.bytes_outstanding.load(), size_t(0));
  TEST_SUCCEED();
}

bool test_dom_allocator_memalloc_on_null() {
  TEST_START();
  null_allocator alloc;
  simdjson::dom::parser parser(alloc);
  simdjson::dom::element elem;
  auto err = parser.parse(SAMPLE_JSON, std::strlen(SAMPLE_JSON)).get(elem);
  ASSERT_EQUAL(err, simdjson::MEMALLOC);
  ASSERT_TRUE(alloc.attempted.load() > 0);
  TEST_SUCCEED();
}

bool test_dom_allocator_exception_propagates() {
  TEST_START();
  // Allow exactly one successful allocation, then throw. This will trigger the
  // exception inside parse; we verify that nothing leaks and the exception
  // escapes.
  throwing_allocator alloc(1);
  bool caught = false;
  try {
    simdjson::dom::parser parser(alloc);
    simdjson::dom::element elem;
    (void)parser.parse(SAMPLE_JSON, std::strlen(SAMPLE_JSON)).get(elem);
  } catch (const std::bad_alloc &) {
    caught = true;
  }
  ASSERT_TRUE(caught);
  // After the parser's destructor runs, no bytes should be outstanding.
  ASSERT_EQUAL(alloc.bytes_outstanding.load(), size_t(0));
  TEST_SUCCEED();
}

bool test_dom_move_assign_preserves_allocator() {
  TEST_START();
  tracking_allocator a1;
  tracking_allocator a2;
  {
    simdjson::dom::parser p1(a1);
    simdjson::dom::element e1;
    ASSERT_SUCCESS(p1.parse(SAMPLE_JSON, std::strlen(SAMPLE_JSON)).get(e1));
    size_t a1_bytes_before = a1.bytes_outstanding.load();
    ASSERT_TRUE(a1_bytes_before > 0);

    simdjson::dom::parser p2(a2);
    simdjson::dom::element e2;
    ASSERT_SUCCESS(p2.parse(SAMPLE_JSON, std::strlen(SAMPLE_JSON)).get(e2));
    size_t a2_bytes_before = a2.bytes_outstanding.load();
    ASSERT_TRUE(a2_bytes_before > 0);

    p1 = std::move(p2);
    // After move, p1 now owns the buffers that were allocated by a2. a1 must
    // have been fully drained of the bytes it lent to p1 previously.
    ASSERT_EQUAL(a1.bytes_outstanding.load(), size_t(0));
    // a2 still has its bytes outstanding, now owned by p1.
    ASSERT_EQUAL(a2.bytes_outstanding.load(), a2_bytes_before);
  }
  ASSERT_EQUAL(a1.bytes_outstanding.load(), size_t(0));
  ASSERT_EQUAL(a2.bytes_outstanding.load(), size_t(0));
  TEST_SUCCEED();
}

bool test_allocator_adapter_std_allocator() {
  TEST_START();
  simdjson::allocator_adapter<std::allocator<std::byte>> a;
  simdjson::dom::parser parser(a);
  simdjson::dom::element elem;
  ASSERT_SUCCESS(parser.parse(SAMPLE_JSON, std::strlen(SAMPLE_JSON)).get(elem));
  int64_t stars;
  ASSERT_SUCCESS(elem["stars"].get(stars));
  ASSERT_EQUAL(stars, 42);
  TEST_SUCCEED();
}

// ---- ondemand tests ----

static padded_string make_padded(const char* s) {
  return padded_string(s, std::strlen(s));
}

bool test_ondemand_custom_allocator_basic() {
  TEST_START();
  tracking_allocator alloc;
  {
    simdjson::ondemand::parser parser(alloc);
    auto padded = make_padded(SAMPLE_JSON);
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    int64_t stars;
    ASSERT_SUCCESS(doc["stars"].get(stars));
    ASSERT_EQUAL(stars, 42);
    ASSERT_TRUE(alloc.total_allocs.load() > 0);
  }
  ASSERT_EQUAL(alloc.bytes_outstanding.load(), size_t(0));
  TEST_SUCCEED();
}

bool test_ondemand_allocator_memalloc_on_null() {
  TEST_START();
  null_allocator alloc;
  simdjson::ondemand::parser parser(alloc);
  auto padded = make_padded(SAMPLE_JSON);
  simdjson::ondemand::document doc;
  auto err = parser.iterate(padded).get(doc);
  ASSERT_EQUAL(err, simdjson::MEMALLOC);
  ASSERT_TRUE(alloc.attempted.load() > 0);
  TEST_SUCCEED();
}

bool test_ondemand_allocator_exception_propagates() {
  TEST_START();
  throwing_allocator alloc(1);
  bool caught = false;
  try {
    simdjson::ondemand::parser parser(alloc);
    auto padded = make_padded(SAMPLE_JSON);
    simdjson::ondemand::document doc;
    (void)parser.iterate(padded).get(doc);
  } catch (const std::bad_alloc &) {
    caught = true;
  }
  ASSERT_TRUE(caught);
  ASSERT_EQUAL(alloc.bytes_outstanding.load(), size_t(0));
  TEST_SUCCEED();
}

// ---- document_stream tests ----
//
// parse_many / iterate_many own a `document_stream` that allocates its own
// scratch buffers on top of the parser's; make sure those allocations also
// flow through the user-supplied allocator.

static const char *const STREAMED_JSON =
    R"({"i":0}{"i":1}{"i":2}{"i":3}{"i":4})";

bool test_dom_parse_many_custom_allocator() {
  TEST_START();
  tracking_allocator alloc;
  {
    simdjson::dom::parser parser(alloc);
    simdjson::dom::document_stream stream;
    auto padded = make_padded(STREAMED_JSON);
    ASSERT_SUCCESS(parser.parse_many(padded).get(stream));
    int64_t expected = 0;
    for (auto doc : stream) {
      simdjson::dom::element e;
      ASSERT_SUCCESS(doc.get(e));
      int64_t i;
      ASSERT_SUCCESS(e["i"].get(i));
      ASSERT_EQUAL(i, expected);
      ++expected;
    }
    ASSERT_EQUAL(expected, int64_t(5));
    ASSERT_TRUE(alloc.total_allocs.load() > 0);
  }
  ASSERT_EQUAL(alloc.bytes_outstanding.load(), size_t(0));
  ASSERT_EQUAL(alloc.total_allocs.load(), alloc.total_frees.load());
  TEST_SUCCEED();
}

bool test_ondemand_iterate_many_custom_allocator() {
  TEST_START();
  tracking_allocator alloc;
  {
    simdjson::ondemand::parser parser(alloc);
    auto padded = make_padded(STREAMED_JSON);
    simdjson::ondemand::document_stream stream;
    ASSERT_SUCCESS(parser.iterate_many(padded).get(stream));
    int64_t expected = 0;
    for (auto doc : stream) {
      int64_t i;
      ASSERT_SUCCESS(doc["i"].get(i));
      ASSERT_EQUAL(i, expected);
      ++expected;
    }
    ASSERT_EQUAL(expected, int64_t(5));
    ASSERT_TRUE(alloc.total_allocs.load() > 0);
  }
  ASSERT_EQUAL(alloc.bytes_outstanding.load(), size_t(0));
  ASSERT_EQUAL(alloc.total_allocs.load(), alloc.total_frees.load());
  TEST_SUCCEED();
}

// ---- builder::string_builder tests ----

bool test_builder_default_allocator() {
  TEST_START();
  simdjson::builder::string_builder b;
  b.start_object();
  b.append_key_value("k", int64_t(7));
  b.end_object();
  std::string_view sv;
  ASSERT_SUCCESS(b.view().get(sv));
  ASSERT_EQUAL(std::string(sv), std::string("{\"k\":7}"));
  TEST_SUCCEED();
}

bool test_builder_custom_allocator() {
  TEST_START();
  tracking_allocator alloc;
  {
    simdjson::builder::string_builder b(alloc, 16);
    // Force a re-grow at least once.
    std::string big(1000, 'x');
    b.escape_and_append_with_quotes(big);
    std::string_view sv;
    ASSERT_SUCCESS(b.view().get(sv));
    ASSERT_EQUAL(sv.size(), big.size() + 2);
    ASSERT_TRUE(alloc.total_allocs.load() >= 2);
  }
  ASSERT_EQUAL(alloc.bytes_outstanding.load(), size_t(0));
  TEST_SUCCEED();
}

bool test_builder_allocator_exception_propagates() {
  TEST_START();
  // Builder succeeds on the first (ctor) allocation but throws when growing.
  throwing_allocator alloc(1);
  bool caught = false;
  try {
    simdjson::builder::string_builder b(alloc, 16);
    std::string big(1024, 'x');
    b.escape_and_append_with_quotes(big);
  } catch (const std::bad_alloc &) {
    caught = true;
  }
  ASSERT_TRUE(caught);
  ASSERT_EQUAL(alloc.bytes_outstanding.load(), size_t(0));
  TEST_SUCCEED();
}

bool test_dom_loaded_bytes_uses_actual_allocated_size() {
  TEST_START();
  // With allocate_at_least semantics the dom::parser's loaded_bytes buffer
  // must treat the allocator's over-allocation as usable capacity rather than
  // re-allocating as soon as the requested size grows slightly.
  rounding_allocator alloc;
  {
    simdjson::dom::parser parser(alloc);
    // Pre-allocate parser-state and document buffers so growth of `capacity()`
    // doesn't muddle the accounting; we're only interested in loaded_bytes.
    ASSERT_SUCCESS(parser.allocate(8 * rounding_allocator::CHUNK));
    ASSERT_SUCCESS(parser.doc.allocate(8 * rounding_allocator::CHUNK));

    simdjson::dom::element elem;
    // First parse: small doc. Triggers the initial loaded_bytes allocation
    // (requested small + padding, rounded up to CHUNK by the allocator).
    std::string small_doc = R"({"a":1})";
    ASSERT_SUCCESS(parser.parse(small_doc.c_str(), small_doc.size()).get(elem));
    size_t allocs_after_first = alloc.total_allocs.load();

    // Second parse: a doc noticeably larger than the first but still small
    // enough that, together with SIMDJSON_PADDING, it fits inside the
    // over-allocated CHUNK-sized loaded_bytes buffer. No re-allocation should
    // happen anywhere.
    std::string larger_doc = "[0";
    for (int i = 0; i < 100; ++i) { larger_doc += ",0"; }
    larger_doc += "]";
    ASSERT_TRUE(larger_doc.size() + simdjson::SIMDJSON_PADDING <=
                rounding_allocator::CHUNK);
    ASSERT_SUCCESS(parser.parse(larger_doc.c_str(), larger_doc.size()).get(elem));
    ASSERT_EQUAL(alloc.total_allocs.load(), allocs_after_first);
  }
  ASSERT_EQUAL(alloc.bytes_outstanding.load(), size_t(0));
  TEST_SUCCEED();
}

bool test_builder_uses_actual_allocated_size() {
  TEST_START();
  // With allocate_at_least semantics the string_builder must treat the
  // allocator's over-allocation as usable capacity rather than asking for a
  // second buffer as soon as the requested size is exceeded.
  rounding_allocator alloc;
  {
    simdjson::builder::string_builder b(alloc, 64);
    // We asked for 64 bytes, the allocator rounded up to CHUNK. Writing
    // CHUNK/2 bytes must NOT trigger a re-grow.
    std::string chunk(rounding_allocator::CHUNK / 2, 'x');
    b.append_raw(chunk);
    ASSERT_EQUAL(alloc.total_allocs.load(), size_t(1));
    std::string_view sv;
    ASSERT_SUCCESS(b.view().get(sv));
    ASSERT_EQUAL(sv.size(), chunk.size());
  }
  ASSERT_EQUAL(alloc.bytes_outstanding.load(), size_t(0));
  TEST_SUCCEED();
}

// ---- runner ----

int test_main() {
  std::cout << "Running allocator tests." << std::endl;
  if (!test_dom_default_allocator_matches()) { return EXIT_FAILURE; }
  if (!test_dom_custom_allocator_basic()) { return EXIT_FAILURE; }
  if (!test_dom_two_parsers_two_allocators()) { return EXIT_FAILURE; }
  if (!test_dom_allocator_memalloc_on_null()) { return EXIT_FAILURE; }
  if (!test_dom_allocator_exception_propagates()) { return EXIT_FAILURE; }
  if (!test_dom_move_assign_preserves_allocator()) { return EXIT_FAILURE; }
  if (!test_allocator_adapter_std_allocator()) { return EXIT_FAILURE; }
  if (!test_dom_loaded_bytes_uses_actual_allocated_size()) { return EXIT_FAILURE; }
  if (!test_ondemand_custom_allocator_basic()) { return EXIT_FAILURE; }
  if (!test_ondemand_allocator_memalloc_on_null()) { return EXIT_FAILURE; }
  if (!test_ondemand_allocator_exception_propagates()) { return EXIT_FAILURE; }
  if (!test_dom_parse_many_custom_allocator()) { return EXIT_FAILURE; }
  if (!test_ondemand_iterate_many_custom_allocator()) { return EXIT_FAILURE; }
  if (!test_builder_default_allocator()) { return EXIT_FAILURE; }
  if (!test_builder_custom_allocator()) { return EXIT_FAILURE; }
  if (!test_builder_allocator_exception_propagates()) { return EXIT_FAILURE; }
  if (!test_builder_uses_actual_allocated_size()) { return EXIT_FAILURE; }
  std::cout << "All allocator tests passed." << std::endl;
  return EXIT_SUCCESS;
}

} // namespace

int main() {
  return test_main();
}
