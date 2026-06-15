// Exercises the key-selector-based reflective deserializer, which is the
// default for reflective tag_invoke: a compile-time key_selector +
// object::for_each single pass instead of one obj[key] lookup per member.
// (It can be disabled with -DSIMDJSON_DISABLE_KEY_SELECTOR_REFLECTION=1.) The
// point of this path is to deserialize structs correctly even when the JSON
// keys are NOT in declaration order, so the tests below deliberately scramble
// the key order.
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

// A member name longer than the OLD 31-character key_selector limit but within
// the current 63-character limit (34 characters). This now goes through the
// key-selector path (no fallback) and must still deserialize correctly.
struct MediumKeyConfig {
  uint64_t number_of_simultaneous_connections = 0; // 34 characters
  uint64_t id = 0;
};

// A member name longer than the 63-character key_selector limit (69 characters).
// On the key-selector path this would be a static_assert; the deserializer must
// detect that the keys do not fit and silently fall back to the ordered
// per-member path, so this struct still compiles and deserializes correctly by
// default. (Regression test for the second review issue on PR #2774.)
struct OverLimitConfig {
  uint64_t a_very_long_member_name_that_exceeds_the_sixty_three_char_limit_padxx = 0; // 69 chars
  uint64_t id = 0;
};

// Field renaming can map a member to a JSON key that key_selector cannot accept:
// one containing a forbidden character (here a backslash and a double-quote). On
// the key-selector path these are rejected at compile time, so the deserializer
// must detect the unfit keys and silently take the ordered per-member fallback.
// The renamed members are optional and absent here, so the fallback leaves them
// empty; the plain member is present. (A null byte cannot reach this point: the
// key travels through the pipeline as a C string and is truncated at the null.)
struct ForbiddenKeyChars {
  [[= simdjson::rename<"back\\slash">]] std::optional<uint64_t> a{};
  [[= simdjson::rename<"qu\"ote">]]     std::optional<uint64_t> b{};
  uint64_t id = 0;
};

// Two members renamed to the same JSON key. The key-selector path rejects
// duplicate keys at compile time, so this struct compiling at all proves the
// ordered per-member fallback was selected.
struct DuplicateKeys {
  [[= simdjson::rename<"shared">]] uint64_t first = 0;
  [[= simdjson::rename<"shared">]] uint64_t second = 0;
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

// A 34-character member name fits the current 63-character key_selector limit, so
// it deserializes through the key-selector path (no fallback).
bool medium_member_name_uses_selector() {
  TEST_START();
  // Keys deliberately out of declaration order.
  auto json = R"({ "id": 99, "number_of_simultaneous_connections": 1234 })"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  MediumKeyConfig c;
  ASSERT_SUCCESS(doc.get(c));
  ASSERT_EQUAL(c.number_of_simultaneous_connections, 1234);
  ASSERT_EQUAL(c.id, 99);
  TEST_SUCCEED();
}

// A member name beyond the 63-character key_selector limit must make the
// deserializer fall back to the ordered per-member path rather than failing to
// compile.
bool long_member_name_falls_back() {
  TEST_START();
  // Keys deliberately out of declaration order to exercise the unordered lookups
  // of the fallback path.
  auto json = R"({ "id": 99, "a_very_long_member_name_that_exceeds_the_sixty_three_char_limit_padxx": 1234 })"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  OverLimitConfig c;
  ASSERT_SUCCESS(doc.get(c));
  ASSERT_EQUAL(c.a_very_long_member_name_that_exceeds_the_sixty_three_char_limit_padxx, 1234);
  ASSERT_EQUAL(c.id, 99);
  TEST_SUCCEED();
}

// A renamed key with a forbidden character must make the deserializer fall back
// to the ordered per-member path (compiling at all proves it; the key-selector
// path would reject the key at compile time).
bool forbidden_key_chars_fall_back() {
  TEST_START();
  auto json = R"({ "id": 7 })"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  ForbiddenKeyChars c;
  ASSERT_SUCCESS(doc.get(c));
  ASSERT_EQUAL(c.id, 7);
  ASSERT_FALSE(c.a.has_value());
  ASSERT_FALSE(c.b.has_value());
  TEST_SUCCEED();
}

// Duplicate renamed keys must make the deserializer fall back to the ordered
// per-member path. The fallback looks up "shared" once per member, so both
// resolve to the single field.
bool duplicate_keys_fall_back() {
  TEST_START();
  auto json = R"({ "shared": 42 })"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  DuplicateKeys c;
  ASSERT_SUCCESS(doc.get(c));
  ASSERT_EQUAL(c.first, 42);
  ASSERT_EQUAL(c.second, 42);
  TEST_SUCCEED();
}

bool run() {
  return flat_out_of_order() &&
         nested_out_of_order() &&
         optional_missing() &&
         medium_member_name_uses_selector() &&
         long_member_name_falls_back() &&
         forbidden_key_chars_fall_back() &&
         duplicate_keys_fall_back() &&
         true;
}

} // namespace

int main(int argc, char *argv[]) {
  return test_main(argc, argv, run);
}

#else

int main() { return 0; }

#endif
