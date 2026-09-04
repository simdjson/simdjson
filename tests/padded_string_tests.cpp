
#include "simdjson.h"
#include "test_macros.h"

#include <cstdlib>
#include <limits>

// this test is needed, because memcpy may be invoked on a null pointer
// otherwise
static bool testNullString() {
  TEST_START();
  std::string_view empty;
  simdjson::padded_string blah(empty);
  TEST_SUCCEED();
}

static bool testFailedAllocationHasEmptyState() {
  TEST_START();
  // This always fails the length + SIMDJSON_PADDING overflow check without
  // relying on the machine actually being out of memory.
  simdjson::padded_string oversized(std::numeric_limits<size_t>::max());
  ASSERT_EQUAL(oversized.size(), 0);
  ASSERT_TRUE(oversized.data() == nullptr);
  TEST_SUCCEED();
}

static bool testPaddedStringBuilder() {
  TEST_START();

  // Test empty builder
  {
    simdjson::padded_string_builder builder;
    ASSERT_EQUAL(builder.length(), 0);
    auto result = builder.build();
    ASSERT_EQUAL(result.length(), 0);
    ASSERT_EQUAL(std::string_view(result), "");
  }

  // Test appending data
  {
    simdjson::padded_string_builder builder;
    const char* data = "Hello";
    bool success = builder.append(data, 5);
    ASSERT_TRUE(success);
    ASSERT_EQUAL(builder.length(), 5);

    auto result = builder.build();
    ASSERT_EQUAL(result.length(), 5);
    ASSERT_EQUAL(std::string_view(result), "Hello");

    // Builder should still have content
    ASSERT_EQUAL(builder.length(), 5);
  }

  // Test appending string_view
  {
    simdjson::padded_string_builder builder;
    std::string_view sv = " World";
    bool success = builder.append(sv);
    ASSERT_TRUE(success);
    ASSERT_EQUAL(builder.length(), 6);

    auto result = builder.build();
    ASSERT_EQUAL(result.size(), 6);
    ASSERT_EQUAL(std::string_view(result), " World");
  }

  // Test multiple appends
  {
    simdjson::padded_string_builder builder;
    ASSERT_TRUE(builder.append("Hello", 5));
    ASSERT_TRUE(builder.append(" ", 1));
    ASSERT_TRUE(builder.append("World", 5));

    ASSERT_EQUAL(builder.length(), 11);

    auto result = builder.build();
    ASSERT_EQUAL(result.size(), 11);
    ASSERT_EQUAL(std::string_view(result), "Hello World");
  }

  // Test convert (move)
  {
    simdjson::padded_string_builder builder;
    ASSERT_TRUE(builder.append("Convert", 7));

    ASSERT_EQUAL(builder.length(), 7);

    auto result = builder.convert();
    ASSERT_EQUAL(result.size(), 7);
    ASSERT_EQUAL(std::string_view(result), "Convert");

    // Builder should be empty after convert
    ASSERT_EQUAL(builder.length(), 0);

    // Building again should give empty string
    auto empty_result = builder.build();
    ASSERT_EQUAL(empty_result.size(), 0);
  }

  // Test initial capacity
  {
    simdjson::padded_string_builder builder(100);
    ASSERT_EQUAL(builder.length(), 0);
    // Capacity is internal, but we can test appending large data
    std::string large_data(50, 'A');
    bool success = builder.append(large_data.data(), large_data.size());
    ASSERT_TRUE(success);
    ASSERT_EQUAL(builder.length(), 50);
  }

  // Test move constructor
  {
    simdjson::padded_string_builder builder1;
    ASSERT_TRUE(builder1.append("Move", 4));

    simdjson::padded_string_builder builder2(std::move(builder1));
    ASSERT_EQUAL(builder2.length(), 4);
    ASSERT_EQUAL(builder1.length(), 0); // moved from

    auto result = builder2.build();
    ASSERT_EQUAL(std::string_view(result), "Move");
  }

  // Test move assignment
  {
    simdjson::padded_string_builder builder1;
    ASSERT_TRUE(builder1.append("Assign", 6));

    simdjson::padded_string_builder builder2;
    builder2 = std::move(builder1);

    ASSERT_EQUAL(builder2.length(), 6);
    ASSERT_EQUAL(builder1.length(), 0); // moved from

    auto result = builder2.convert();
    ASSERT_EQUAL(std::string_view(result), "Assign");
  }

  TEST_SUCCEED();
}


bool testSliceAt() {
  TEST_START();
  simdjson::padded_string json = "{\"a\":1}\n{\"a\":2}\n{\"a\":3}\n"_padded;
  auto view = simdjson::padded_string_view(json);

  // A block size larger than the input yields the whole input, then nothing.
  {
    auto all = simdjson::slice_at(view, '\n', 1 << 20, 0);
    ASSERT_EQUAL(all.size(), view.size());
    ASSERT_TRUE(simdjson::slice_at(view, '\n', 1 << 20, 1).empty());
  }

  // Slices are contiguous, non-overlapping, and reassemble the input. A block
  // that falls inside one document yields an empty slice, so iteration is
  // bounded by the input size rather than by the first empty slice.
  for (size_t block : {size_t(1), size_t(4), size_t(8), size_t(9), size_t(64)}) {
    std::string rebuilt;
    size_t previous_end = 0;
    for (size_t i = 0; i * block < view.size(); i++) {
      auto piece = simdjson::slice_at(view, '\n', block, i);
      if (piece.empty()) { continue; }
      size_t begin = size_t(piece.data() - view.data());
      ASSERT_EQUAL(begin, previous_end);
      previous_end = begin + piece.size();
      ASSERT_TRUE(piece.capacity() >= piece.size() + simdjson::SIMDJSON_PADDING);
      rebuilt.append(piece.data(), piece.size());
    }
    ASSERT_EQUAL(previous_end, view.size());
    ASSERT_EQUAL(rebuilt, std::string(view.data(), view.size()));
  }

  // Every slice ends on a document boundary, so each parses on its own.
  {
    size_t documents = 0;
    uint64_t sum = 0;
    simdjson::ondemand::parser parser;
    for (size_t i = 0;; i++) {
      if (i * 8 >= view.size()) { break; }
      auto piece = simdjson::slice_at(view, '\n', 8, i);
      if (piece.empty()) { continue; }
      simdjson::ondemand::document_stream docs;
      ASSERT_SUCCESS(parser.iterate_many(piece, piece.size()).get(docs));
      for (auto doc : docs) {
        uint64_t a;
        ASSERT_SUCCESS(doc["a"].get_uint64().get(a));
        sum += a;
        documents++;
      }
    }
    ASSERT_EQUAL(documents, 3);
    ASSERT_EQUAL(sum, 6);
  }

  // Degenerate inputs.
  ASSERT_TRUE(simdjson::slice_at(view, '\n', 0, 0).empty());
  ASSERT_TRUE(simdjson::slice_at(view, '\n', 8, 1000).empty());
  TEST_SUCCEED();
}

int main() {
  if (!testNullString()) {
    return EXIT_FAILURE;
  }
  if (!testFailedAllocationHasEmptyState()) {
    return EXIT_FAILURE;
  }
  if (!testPaddedStringBuilder()) {
    return EXIT_FAILURE;
  }
  if (!testSliceAt()) {
    return EXIT_FAILURE;
  }
  std::cout << "All tests passed!" << std::endl;
  return EXIT_SUCCESS;
}
