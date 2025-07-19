#include "simdjson.h"
#include "test_builder.h"
#include <string>

using namespace simdjson;

namespace builder_tests {

  bool test_simple_convert_api() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct Person {
      std::string name;
      int age;
      enum class Status { Active, Inactive } status;
    };
    
    Person person{"Alice", 30, Person::Status::Active};
    
    // Test simple serialization
    std::string json = convert(person);
    ASSERT_TRUE(json.find("\"name\":\"Alice\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"age\":30") != std::string::npos);
    ASSERT_TRUE(json.find("\"status\":\"Active\"") != std::string::npos);
    
    // Test simple deserialization
    Person restored = convert<Person>(json);
    ASSERT_EQUAL(restored.name, "Alice");
    ASSERT_EQUAL(restored.age, 30);
    ASSERT_TRUE(restored.status == Person::Status::Active);
    
    // Test custom JSON
    std::string custom_json = R"({"name":"Bob","age":25,"status":"Inactive"})";
    Person bob = convert<Person>(custom_json);
    ASSERT_EQUAL(bob.name, "Bob");
    ASSERT_EQUAL(bob.age, 25);
    ASSERT_TRUE(bob.status == Person::Status::Inactive);
#endif
    TEST_SUCCEED();
  }

  bool test_simple_try_convert_api() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct Car {
      std::string brand;
      int year;
    };
    
    Car car{"Tesla", 2024};
    
    // Test no-throw serialization
    auto json_result = try_convert(car);
    ASSERT_SUCCESS(json_result);
    std::string json = json_result.value();
    ASSERT_TRUE(json.find("\"brand\":\"Tesla\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"year\":2024") != std::string::npos);
    
    // Test no-throw deserialization
    auto car_result = try_convert<Car>(json);
    ASSERT_SUCCESS(car_result);
    Car restored = car_result.value();
    ASSERT_EQUAL(restored.brand, "Tesla");
    ASSERT_EQUAL(restored.year, 2024);
    
    // Test error handling
    auto bad_result = try_convert<Car>("{invalid json}");
    ASSERT_TRUE(bad_result.error() != SUCCESS);
#endif
    TEST_SUCCEED();
  }

  bool test_complex_types_simple_api() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    struct Config {
      std::string name;
      std::optional<int> max_value;
      std::vector<std::string> tags;
      enum class Mode { Debug, Release } mode;
    };
    
    Config config{
      "MyApp",
      std::optional<int>(100),
      {"web", "api", "backend"},
      Config::Mode::Release
    };
    
    // Test serialization of complex types
    std::string json = convert(config);
    ASSERT_TRUE(json.find("\"name\":\"MyApp\"") != std::string::npos);
    ASSERT_TRUE(json.find("\"max_value\":100") != std::string::npos);
    ASSERT_TRUE(json.find("\"tags\":[\"web\",\"api\",\"backend\"]") != std::string::npos);
    ASSERT_TRUE(json.find("\"mode\":\"Release\"") != std::string::npos);
    
    // Test deserialization of complex types
    Config restored = convert<Config>(json);
    ASSERT_EQUAL(restored.name, "MyApp");
    ASSERT_TRUE(restored.max_value.has_value());
    ASSERT_EQUAL(restored.max_value.value(), 100);
    ASSERT_EQUAL(restored.tags.size(), 3);
    ASSERT_EQUAL(restored.tags[0], "web");
    ASSERT_EQUAL(restored.tags[1], "api");
    ASSERT_EQUAL(restored.tags[2], "backend");
    ASSERT_TRUE(restored.mode == Config::Mode::Release);
#endif
    TEST_SUCCEED();
  }

  bool run() {
    return test_simple_convert_api() &&
           test_simple_try_convert_api() &&
           test_complex_types_simple_api();
  }

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, builder_tests::run);
}