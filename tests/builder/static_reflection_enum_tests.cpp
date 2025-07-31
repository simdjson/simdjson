#include "simdjson.h"
#include "test_builder.h"
#include <string>

using namespace simdjson;

namespace builder_tests {

  bool test_enum_serialization() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    enum class Color {
      Red,
      Green,
      Blue
    };

    struct EnumStruct {
      Color color;
      int value;
    };

    EnumStruct test{Color::Red, 42};

    auto result = builder::to_json_string(test);
    ASSERT_SUCCESS(result);

    std::string json = result.value();
    // Enum should be serialized as string (Red)
    ASSERT_TRUE(json.find("\"color\":\"Red\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"value\":42") != std::string::npos);

    // Test different enum values
    test.color = Color::Green;
    auto result2 = builder::to_json_string(test);
    ASSERT_SUCCESS(result2);
    std::string json2 = result2.value();
    ASSERT_TRUE(json2.find("\"color\":\"Green\"") != std::string::npos);

    test.color = Color::Blue;
    auto result3 = builder::to_json_string(test);
    ASSERT_SUCCESS(result3);
    std::string json3 = result3.value();
    ASSERT_TRUE(json3.find("\"color\":\"Blue\"") != std::string::npos);
#endif
    TEST_SUCCEED();
  }

  bool test_enum_deserialization() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    enum class Status {
      Active,
      Inactive,
      Pending
    };

    struct StatusStruct {
      Status status;
      std::string name;
    };

    // Test deserialization of different enum values with string representation
    std::string json1 = "{\"status\":\"Active\",\"name\":\"test1\"}";
    ondemand::parser parser1;
    auto doc_result1 = parser1.iterate(pad(json1));
    ASSERT_SUCCESS(doc_result1);

    auto get_result1 = doc_result1.value().get<StatusStruct>();
    ASSERT_SUCCESS(get_result1);

    StatusStruct deserialized1 = std::move(get_result1.value());
    ASSERT_TRUE(deserialized1.status == Status::Active);
    ASSERT_EQUAL(deserialized1.name, "test1");

    // Test Status::Inactive
    std::string json2 = "{\"status\":\"Inactive\",\"name\":\"test2\"}";
    ondemand::parser parser2;
    auto doc_result2 = parser2.iterate(pad(json2));
    ASSERT_SUCCESS(doc_result2);

    auto get_result2 = doc_result2.value().get<StatusStruct>();
    ASSERT_SUCCESS(get_result2);

    StatusStruct deserialized2 = std::move(get_result2.value());
    ASSERT_TRUE(deserialized2.status == Status::Inactive);
    ASSERT_EQUAL(deserialized2.name, "test2");

    // Test Status::Pending
    std::string json3 = "{\"status\":\"Pending\",\"name\":\"test3\"}";
    ondemand::parser parser3;
    auto doc_result3 = parser3.iterate(pad(json3));
    ASSERT_SUCCESS(doc_result3);

    auto get_result3 = doc_result3.value().get<StatusStruct>();
    ASSERT_SUCCESS(get_result3);

    StatusStruct deserialized3 = std::move(get_result3.value());
    ASSERT_TRUE(deserialized3.status == Status::Pending);
    ASSERT_EQUAL(deserialized3.name, "test3");
#endif
    TEST_SUCCEED();
  }

  bool test_enum_round_trip() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    enum class Priority {
      Low,
      Medium,
      High,
      Critical
    };

    struct Task {
      Priority priority;
      std::string description;
      int id;
    };

    Task original{Priority::High, "Important task", 123};

    // Serialize
    auto serialize_result = builder::to_json_string(original);
    ASSERT_SUCCESS(serialize_result);

    std::string json = serialize_result.value();
    ASSERT_TRUE(json.find("\"priority\":\"High\"") != std::string::npos); // High as string
    ASSERT_TRUE(json.find("\"description\":\"Important task\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"id\":123") != std::string::npos);

    // Deserialize
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<Task>();
    ASSERT_SUCCESS(get_result);

    Task deserialized = std::move(get_result.value());
    ASSERT_TRUE(deserialized.priority == Priority::High);
    ASSERT_EQUAL(deserialized.description, "Important task");
    ASSERT_EQUAL(deserialized.id, 123);
#endif
    TEST_SUCCEED();
  }

  bool test_enum_with_underlying_type() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    enum class ErrorCode : int {
      Success = 0,
      NotFound = 404,
      ServerError = 500
    };

    struct Response {
      ErrorCode error;
      std::string message;
    };

    Response test{ErrorCode::NotFound, "Resource not found"};

    auto result = builder::to_json_string(test);
    ASSERT_SUCCESS(result);

    std::string json = result.value();
    ASSERT_TRUE(json.find("\"error\":\"NotFound\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"message\":\"Resource not found\"") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<Response>();
    ASSERT_SUCCESS(get_result);

    Response deserialized = std::move(get_result.value());
    ASSERT_TRUE(deserialized.error == ErrorCode::NotFound);
    ASSERT_EQUAL(deserialized.message, "Resource not found");
#endif
    TEST_SUCCEED();
  }

  bool test_multiple_enums() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    enum class Day {
      Monday,
      Tuesday,
      Wednesday,
      Thursday,
      Friday,
      Saturday,
      Sunday
    };

    enum class Month {
      January,
      February,
      March,
      April,
      May,
      June,
      July,
      August,
      September,
      October,
      November,
      December
    };

    struct Date {
      Day day;
      Month month;
      int year;
    };

    Date test{Day::Friday, Month::July, 2024};

    auto result = builder::to_json_string(test);
    ASSERT_SUCCESS(result);

    std::string json = result.value();
    ASSERT_TRUE(json.find("\"day\":\"Friday\"") != std::string::npos);      // Friday as string
    ASSERT_TRUE(json.find("\"month\":\"July\"") != std::string::npos);    // July as string
    ASSERT_TRUE(json.find("\"year\":2024") != std::string::npos);

    // Test round-trip
    ondemand::parser parser;
    auto doc_result = parser.iterate(pad(json));
    ASSERT_SUCCESS(doc_result);

    auto get_result = doc_result.value().get<Date>();
    ASSERT_SUCCESS(get_result);

    Date deserialized = std::move(get_result.value());
    ASSERT_TRUE(deserialized.day == Day::Friday);
    ASSERT_TRUE(deserialized.month == Month::July);
    ASSERT_EQUAL(deserialized.year, 2024);
#endif
    TEST_SUCCEED();
  }

  bool run() {
    return test_enum_serialization() &&
           test_enum_deserialization() &&
           test_enum_round_trip() &&
           test_enum_with_underlying_type() &&
           test_multiple_enums();
  }

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, builder_tests::run);
}