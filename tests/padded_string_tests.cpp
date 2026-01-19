
#include "simdjson.h"
#include "test_macros.h"

#include <cstdlib>

// this test is needed, because memcpy may be invoked on a null pointer
// otherwise
static bool testNullString() {
  TEST_START();
  std::string_view empty;
  simdjson::padded_string blah(empty);
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

int main() {
  if (!testNullString()) {
    return EXIT_FAILURE;
  }
  if (!testPaddedStringBuilder()) {
    return EXIT_FAILURE;
  }
  std::cout << "All tests passed!" << std::endl;
  return EXIT_SUCCESS;
}
