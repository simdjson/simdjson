#include "simdjson.h"
#include "test_builder.h"
#include <chrono>
#include <optional>
#include <string>
#include <vector>

using namespace simdjson;

namespace builder_tests {

#if SIMDJSON_STATIC_REFLECTION

struct Meeting {
  std::string title;
  std::chrono::system_clock::time_point start_time;
  std::vector<std::string> attendees;
  std::optional<std::string> location;
  bool is_recurring;
};

bool test_meeting_roundtrip() {
  TEST_START();
  // Truncate to whole seconds so ISO-8601 second precision round-trips exactly.
  const auto start = std::chrono::time_point_cast<std::chrono::seconds>(
      std::chrono::system_clock::from_time_t(1705314600)); // 2024-01-15T10:30:00Z
  Meeting original{.title = "CppCon Planning",
                   .start_time = start,
                   .attendees = {"Alice", "Bob", "Charlie"},
                   .location = "Denver",
                   .is_recurring = true};

  std::string json;
  ASSERT_SUCCESS(to_json(original).get(json));
  ASSERT_TRUE(json.find("\"2024-01-15T10:30:00Z\"") != std::string::npos);

  ondemand::parser parser;
  auto doc = parser.iterate(pad(json));
  ASSERT_SUCCESS(doc);
  Meeting deserialized;
  ASSERT_SUCCESS(doc.get(deserialized));

  ASSERT_EQUAL(deserialized.title, original.title);
  ASSERT_EQUAL(deserialized.is_recurring, original.is_recurring);
  ASSERT_TRUE(deserialized.location.has_value());
  ASSERT_EQUAL(deserialized.location.value(), original.location.value());
  ASSERT_EQUAL(deserialized.attendees.size(), original.attendees.size());
  for (size_t i = 0; i < original.attendees.size(); i++) {
    ASSERT_EQUAL(deserialized.attendees[i], original.attendees[i]);
  }
  ASSERT_TRUE(deserialized.start_time == original.start_time);
  TEST_SUCCEED();
}

bool test_time_point_alone() {
  TEST_START();
  const auto tp = std::chrono::time_point_cast<std::chrono::seconds>(
      std::chrono::system_clock::from_time_t(0)); // 1970-01-01T00:00:00Z
  std::string json;
  ASSERT_SUCCESS(to_json(tp).get(json));
  ASSERT_EQUAL(json, "\"1970-01-01T00:00:00Z\"");

  ondemand::parser parser;
  auto doc = parser.iterate(pad(json));
  ASSERT_SUCCESS(doc);
  std::chrono::system_clock::time_point out;
  ASSERT_SUCCESS(doc.get(out));
  ASSERT_TRUE(out == tp);
  TEST_SUCCEED();
}

bool test_reject_bad_timestamp() {
  TEST_START();
  auto json = R"("not-a-timestamp")"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(json);
  ASSERT_SUCCESS(doc);
  std::chrono::system_clock::time_point out;
  ASSERT_ERROR(doc.get(out), INCORRECT_TYPE);
  TEST_SUCCEED();
}

#endif // SIMDJSON_STATIC_REFLECTION

bool run() {
#if SIMDJSON_STATIC_REFLECTION
  return test_meeting_roundtrip() && test_time_point_alone() &&
         test_reject_bad_timestamp();
#else
  return true;
#endif
}

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, builder_tests::run);
}
