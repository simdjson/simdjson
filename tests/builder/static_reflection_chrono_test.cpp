#include "simdjson.h"
#include "test_builder.h"
#include <chrono>
#include <string>
#include <vector>
#include <optional>
#include <iostream>

using namespace simdjson;

namespace builder_tests {

struct Meeting {
    std::string title;
    std::chrono::system_clock::time_point start_time;
    std::vector<std::string> attendees;
    std::optional<std::string> location;
    bool is_recurring;
};

bool test_meeting_roundtrip() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    // Create a Meeting instance with current time
    auto now = std::chrono::system_clock::now();
    Meeting original{
        .title = "CppCon Planning",
        .start_time = now,
        .attendees = {"Alice", "Bob", "Charlie"},
        .location = "Denver",
        .is_recurring = true
    };

    // Serialize to JSON
    auto json_result = builder::to_json_string(original);
    ASSERT_SUCCESS(json_result);
    std::string json = json_result.value();

    std::cout << "Serialized JSON: " << json << std::endl;

    // Parse back from JSON
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<Meeting>();
    ASSERT_SUCCESS(get_result);

    Meeting deserialized = std::move(get_result.value());

    // Verify round-trip
    ASSERT_EQUAL(deserialized.title, original.title);
    ASSERT_EQUAL(deserialized.is_recurring, original.is_recurring);
    ASSERT_TRUE(deserialized.location.has_value());
    ASSERT_EQUAL(deserialized.location.value(), original.location.value());
    ASSERT_EQUAL(deserialized.attendees.size(), original.attendees.size());
    for(size_t i = 0; i < original.attendees.size(); i++) {
        ASSERT_EQUAL(deserialized.attendees[i], original.attendees[i]);
    }

    // Check that the time point is correctly deserialized with millisecond precision
    auto original_ms = std::chrono::duration_cast<std::chrono::milliseconds>(original.start_time.time_since_epoch()).count();
    auto deserialized_ms = std::chrono::duration_cast<std::chrono::milliseconds>(deserialized.start_time.time_since_epoch()).count();

    std::cout << "Original time: " << original_ms << "ms since epoch" << std::endl;
    std::cout << "Deserialized time: " << deserialized_ms << "ms since epoch" << std::endl;

    // Verify exact equality at millisecond precision
    ASSERT_EQUAL(deserialized_ms, original_ms);

#endif
    TEST_SUCCEED();
}

bool test_duration_roundtrip() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct Task {
        std::string name;
        std::chrono::milliseconds duration_ms;
        std::chrono::seconds duration_s;
        std::chrono::minutes duration_min;
    };

    Task original{
        .name = "Build Project",
        .duration_ms = std::chrono::milliseconds(5432),
        .duration_s = std::chrono::seconds(300),
        .duration_min = std::chrono::minutes(45)
    };

    // Serialize to JSON
    auto json_result = builder::to_json_string(original);
    ASSERT_SUCCESS(json_result);
    std::string json = json_result.value();

    std::cout << "Duration test JSON: " << json << std::endl;

    // Parse back from JSON
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<Task>();
    ASSERT_SUCCESS(get_result);

    Task deserialized = std::move(get_result.value());

    // Verify round-trip
    ASSERT_EQUAL(deserialized.name, original.name);
    ASSERT_EQUAL(deserialized.duration_ms.count(), original.duration_ms.count());
    ASSERT_EQUAL(deserialized.duration_s.count(), original.duration_s.count());
    ASSERT_EQUAL(deserialized.duration_min.count(), original.duration_min.count());

#endif
    TEST_SUCCEED();
}

bool test_different_clock_types() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct Schedule {
        std::string event;
        std::chrono::system_clock::time_point system_time;
        std::chrono::steady_clock::time_point steady_time;
    };

    Schedule original{
        .event = "Conference",
        .system_time = std::chrono::system_clock::now(),
        .steady_time = std::chrono::steady_clock::now()
    };

    // Serialize to JSON
    auto json_result = builder::to_json_string(original);
    ASSERT_SUCCESS(json_result);
    std::string json = json_result.value();

    std::cout << "Clock types test JSON: " << json << std::endl;

    // Parse back from JSON
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<Schedule>();
    ASSERT_SUCCESS(get_result);

    Schedule deserialized = std::move(get_result.value());

    // Verify round-trip - check millisecond precision
    ASSERT_EQUAL(deserialized.event, original.event);

    auto orig_sys_ms = std::chrono::duration_cast<std::chrono::milliseconds>(original.system_time.time_since_epoch()).count();
    auto deser_sys_ms = std::chrono::duration_cast<std::chrono::milliseconds>(deserialized.system_time.time_since_epoch()).count();
    ASSERT_EQUAL(deser_sys_ms, orig_sys_ms);

    auto orig_steady_ms = std::chrono::duration_cast<std::chrono::milliseconds>(original.steady_time.time_since_epoch()).count();
    auto deser_steady_ms = std::chrono::duration_cast<std::chrono::milliseconds>(deserialized.steady_time.time_since_epoch()).count();
    ASSERT_EQUAL(deser_steady_ms, orig_steady_ms);

#endif
    TEST_SUCCEED();
}

bool run() {
    return test_meeting_roundtrip() &&
           test_duration_roundtrip() &&
           test_different_clock_types();
}

} // namespace builder_tests

int main(int argc, char *argv[]) {
    return builder_tests::run() ? EXIT_SUCCESS : EXIT_FAILURE;
}