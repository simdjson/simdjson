// Exercises the experimental key-selector-based reflective deserializer.
//
// We enable the option here (before including simdjson) so the reflective
// tag_invoke uses a compile-time key_selector + object::for_each single pass
// instead of one obj[key] lookup per member. The point of the option is to
// deserialize structs correctly even when the JSON keys are NOT in declaration
// order, so the tests below deliberately scramble the key order.
#define SIMDJSON_USE_KEY_SELECTOR_REFLECTION 1
#include "simdjson.h"
#include <optional>
#include <string>
#include "test_ondemand.h"

#if SIMDJSON_STATIC_REFLECTION

namespace {

using namespace simdjson;

// Default member initializers keep -Weffc++ happy; reflective deserialization
// overwrites these fields from the JSON.
struct User {
  uint64_t id = 0;
  std::string screen_name{};
  std::optional<std::string> location{};
  bool verified = false;
};

struct Tweet {
  uint64_t id = 0;
  std::string text{};
  uint64_t retweet_count = 0;
  User user{};
};

struct Flat {
  uint64_t id = 0;
  std::string text{};
  uint64_t retweet_count = 0;
};

bool flat_out_of_order() {
  TEST_START();
  // Keys deliberately out of declaration order.
  auto json = R"({ "retweet_count": 7, "id": 12345, "text": "hello world" })"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  Flat f;
  ASSERT_SUCCESS(doc.get(f));
  ASSERT_EQUAL(f.id, 12345);
  ASSERT_EQUAL(f.text, "hello world");
  ASSERT_EQUAL(f.retweet_count, 7);
  TEST_SUCCEED();
}

bool nested_out_of_order() {
  TEST_START();
  // Both the outer and the nested object have scrambled key order, and the
  // optional "location" is present here.
  auto json = R"({
    "user": {
      "verified": true,
      "location": "Montreal",
      "screen_name": "ayuu0123",
      "id": 1186275104
    },
    "retweet_count": 3,
    "text": "nested",
    "id": 99
  })"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  Tweet t;
  ASSERT_SUCCESS(doc.get(t));
  ASSERT_EQUAL(t.id, 99);
  ASSERT_EQUAL(t.text, "nested");
  ASSERT_EQUAL(t.retweet_count, 3);
  ASSERT_EQUAL(t.user.id, 1186275104);
  ASSERT_EQUAL(t.user.screen_name, "ayuu0123");
  ASSERT_TRUE(t.user.location.has_value());
  ASSERT_EQUAL(t.user.location.value(), "Montreal");
  ASSERT_TRUE(t.user.verified);
  TEST_SUCCEED();
}

bool optional_missing() {
  TEST_START();
  // "location" is absent: the optional member must be left empty, not error.
  auto json = R"({
    "user": { "id": 7, "screen_name": "nobody", "verified": false },
    "id": 1, "text": "t", "retweet_count": 0
  })"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  Tweet t;
  ASSERT_SUCCESS(doc.get(t));
  ASSERT_EQUAL(t.user.id, 7);
  ASSERT_EQUAL(t.user.screen_name, "nobody");
  ASSERT_FALSE(t.user.location.has_value());
  ASSERT_FALSE(t.user.verified);
  TEST_SUCCEED();
}

bool run() {
  return flat_out_of_order() &&
         nested_out_of_order() &&
         optional_missing() &&
         true;
}

} // namespace

int main(int argc, char *argv[]) {
  return test_main(argc, argv, run);
}

#else

int main() { return 0; }

#endif
