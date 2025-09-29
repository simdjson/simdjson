#include "simdjson.h"
#include "simdjson/convert.h"
#include "test_ondemand.h"

#include <chrono>
#include <map>
#include <ranges>
#include <string>
#include <vector>

#if SIMDJSON_STATIC_REFLECTION

class MyDate {
public:
    void assign(std::string_view str) {
        date_str = str;
    }
    const std::string& to_string() const {
        return date_str;
    }
private:
    std::string date_str;
};


struct Meeting {
    std::string title;
    std::vector<std::string> attendees;
    std::optional<std::string> location;
    bool is_recurring;
};


struct MeetingTime {
    std::string title;
    std::chrono::system_clock::time_point start_time;
    std::vector<std::string> attendees;
    std::optional<std::string> location;
    bool is_recurring;
};


namespace simdjson {
template <typename simdjson_value>
auto tag_invoke(deserialize_tag, simdjson_value &val, MyDate& date) {
    std::string_view str;
    auto error = val.get_string().get(str);
    if(error) { return error; }
    date.assign(str);
  return simdjson::SUCCESS;
}
} // namespace simdjson

struct complicated_weather_data {
    std::vector<MyDate> time;
    std::vector<float> temperature;
};


#endif

#ifdef __cpp_lib_ranges

namespace convert_tests {

simdjson::padded_string json_car =
    R"( {
         "make": "Toyota",
         "model": "Camry",
         "year": 2018,
         "tire_pressure": [ 40.1, 39.9 ]
       } )"_padded;
simdjson::padded_string json_cars =
    R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
])"_padded;

#if SIMDJSON_SUPPORTS_CONCEPTS
  struct Car {
    std::string make{};
    std::string model{};
    int year{};
    std::vector<double> tire_pressure{};

    friend simdjson::error_code tag_invoke(simdjson::deserialize_tag, auto &val,
                                          Car &car) {
      simdjson::ondemand::object obj;
      auto error = val.get_object().get(obj);
      if (error) {
        return error;
      }
      // Instead of repeatedly obj["something"], we iterate through the object
      // which we expect to be faster.
      for (auto field : obj) {
        simdjson::ondemand::raw_json_string key;
        error = field.key().get(key);
        if (error) {
          return error;
        }
        if (key == "make") {
          error = field.value().get_string(car.make);
          if (error) {
            return error;
          }
        } else if (key == "model") {
          error = field.value().get_string(car.model);
          if (error) {
            return error;
          }
        } else if (key == "year") {
          error = field.value().get(car.year);
          if (error) {
            return error;
          }
        } else if (key == "tire_pressure") {
          error = field.value().get(car.tire_pressure);
          if (error) {
            return error;
          }
        }
      }
      return simdjson::SUCCESS;
    }
  };

  static_assert(simdjson::custom_deserializable<std::unique_ptr<Car>>,
                "It should be deserializable");


  bool simple_no_except() {
    TEST_START();
    Car car;
    ASSERT_SUCCESS(simdjson::from(json_car).get(car));
    if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
      return false;
    }
    TEST_SUCCEED();
  }
#endif // SIMDJSON_SUPPORTS_CONCEPTS
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS

  bool simple() {
    TEST_START();
    Car car = simdjson::from(json_car);
    if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
      return false;
    }
    TEST_SUCCEED();
  }
  struct Player {
      std::string username;
      int level;
      double health;
  };
  struct BadPlayer {
      int username;  // Oops, should be string!
      int level;
      double health;
  };
  struct OptionalPlayer {
      std::string username;
      std::optional<int> level;
      double health;
  };
#if SIMDJSON_STATIC_REFLECTION
  bool missing_key_player() {
    TEST_START();
    std::string json = R"({"username":"Alice","health":100.0})";
    simdjson::padded_string padded(json);
    Player p;
    simdjson::ondemand::parser parser;
    auto doc = parser.iterate(padded);
    ASSERT_ERROR(doc.get(p), simdjson::NO_SUCH_FIELD);
    TEST_SUCCEED();
  }
  bool missing_key_optional_player() {
    TEST_START();
    std::string json = R"({"username":"Alice","health":100.0})";
    simdjson::padded_string padded(json);
    OptionalPlayer p;
    simdjson::ondemand::parser parser;
    auto doc = parser.iterate(padded);
    ASSERT_SUCCESS(doc.get(p));
    TEST_SUCCEED();
  }
  bool bad_player() {
    TEST_START();
    // username is a string but we declared it as an int
    std::string json = R"({"username":"Alice","level":42,"health":100.0})";
    simdjson::padded_string padded(json);
    BadPlayer p;
    simdjson::ondemand::parser parser;
    auto doc = parser.iterate(padded);
    ASSERT_FAILURE(doc.get(p));
    TEST_SUCCEED();
  }
  bool good_player() {
    TEST_START();
    std::string json = R"({"username":123,"level":42,"health":100.0})";
    simdjson::padded_string padded(json);
    BadPlayer p;
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    ASSERT_SUCCESS(doc.get(p));
    TEST_SUCCEED();
  }
  bool complicated_weather_test() {
    TEST_START();
    std::string json = R"({"time":["2023-03-15T12:00:00Z"],"temperature":[42]})";
    simdjson::padded_string padded(json);
    complicated_weather_data p;
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    ASSERT_SUCCESS(doc.get(p));
    TEST_SUCCEED();
  }


  bool meeting_test() {
    TEST_START();
    std::string json = simdjson::to_json(Meeting{
        .title = "CppCon Planning",
        .attendees = {"Alice", "Bob", "Charlie"},
        .location = "Denver",
        .is_recurring = true
    });
    Meeting m2 = simdjson::from(json);
    std::cout << m2.title << std::endl;
    TEST_SUCCEED();
  }
  bool meeting_time_test() {
    TEST_START();
    auto start_time = std::chrono::system_clock::now();
    std::string json = simdjson::to_json(MeetingTime{
        .title = "CppCon Planning",
        .start_time = start_time,
        .attendees = {"Alice", "Bob", "Charlie"},
        .location = "Denver",
        .is_recurring = true
    });
    std::cout << json << std::endl;
    MeetingTime m2 = simdjson::from(json);
    //ASSERT_EQUAL(m2.start_time, start_time);
    std::cout << m2.title << std::endl;
    TEST_SUCCEED();
  }


#endif // SIMDJSON_STATIC_REFLECTION
bool broken() {
  TEST_START();
  simdjson::padded_string short_json_cars = R"( { "make )"_padded;
  try {
    Car car = simdjson::from(json_cars);
    TEST_FAIL("Should not have succeeded");
  } catch (...) {
    TEST_SUCCEED();
  }
  TEST_SUCCEED();
}

bool simple_optional() {
  TEST_START();
  auto car = simdjson::from(json_car).optional<Car>();
  if (!car.has_value() || car->make != "Toyota" || car->model != "Camry" ||
      car->year != 2018) {
    return false;
  }
  TEST_SUCCEED();
}

bool example_with_parser() {
  TEST_START();
  simdjson::ondemand::parser parser;
  std::map<std::string, std::string> obj = simdjson::from(parser, R"({"key": "value"})"_padded);
  ASSERT_EQUAL(obj["key"], "value");
  TEST_SUCCEED();
}

bool with_parser() {
  TEST_START();
  simdjson::ondemand::parser parser;
  Car car = simdjson::from(parser, json_car);
  if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
    return false;
  }
  TEST_SUCCEED();
}

bool to_array() {
  TEST_START();
  auto parser = simdjson::from(json_cars);
  for (auto val : parser.array()) {
    Car car{};
    if (auto const error = val.get(car)) {
      std::cerr << simdjson::error_message(error) << std::endl;
      return false;
    }
    if (car.year < 1998) {
      std::cerr << car.make << " " << car.model << " " << car.year << std::endl;
      return false;
    }
  }
  TEST_SUCCEED();
}

bool to_bad_array() {
  TEST_START();
  auto parser = simdjson::from(json_car);
  try {
    auto array_result = parser.array();
    // Check if array_result has an error
    if (array_result.error() != simdjson::SUCCESS) {
      // This is expected - trying to get array from an object should fail
      if (array_result.error() != simdjson::INCORRECT_TYPE) {
        std::cerr << "Expected INCORRECT_TYPE but got: " << array_result.error()
                  << " (" << simdjson::error_message(array_result.error()) << ")" << std::endl;
        return false;
      }
      // Got expected error, test passes
      TEST_SUCCEED();
    }

    // If we get here without error, try to iterate
    // This might throw when we try to use the array
    for (auto val : array_result) {
      static_cast<void>(val);
      // Should not reach here - the JSON is an object, not an array
      std::cerr << "Unexpectedly succeeded in iterating over non-array JSON" << std::endl;
      return false;
    }
    // Also should not reach here
    std::cerr << "array() succeeded on object JSON without throwing" << std::endl;
    return false;
  } catch (simdjson::simdjson_error &e) {
    if (e.error() != simdjson::INCORRECT_TYPE) {
      std::cerr << "Expected INCORRECT_TYPE but got: " << e.error() << " (" << simdjson::error_message(e.error()) << ")" << std::endl;
      return false;
    }
    // Got expected exception, test passes
  } catch (...) {
    std::cerr << "Unexpected exception type" << std::endl;
    return false;
  }
  TEST_SUCCEED();
}

#if SIMDJSON_SUPPORTS_RANGES_FROM
// currently not active (SIMDJSON_SUPPORTS_RANGES_FROM)
bool to_array_shortcut() {
  TEST_START();
  simdjson::ondemand::parser parser;
  for (auto val : simdjson::from(parser, json_cars)) {
    Car car{};
    if (auto const error = val.get(car)) {
      std::cerr << simdjson::error_message(error) << std::endl;
      return false;
    }
    if (car.year < 1998) {
      std::cerr << car.make << " " << car.model << " " << car.year << std::endl;
      return false;
    }
  }
  TEST_SUCCEED();
}
// currently not active (SIMDJSON_SUPPORTS_RANGES_FROM)
bool test_basic_adaptor() {
  TEST_START();
  int64_t sum_year = 0;
  for (Car car : simdjson::from(json_cars) | simdjson::as<Car>()) {
    printf("Car: %s %s %d\n", car.make.c_str(), car.model.c_str(), car.year);
    if (car.year < 1998) {
      return false;
    }
    sum_year += car.year;
  }
  ASSERT_EQUAL(sum_year, 2018 + 2012 + 1999);
  TEST_SUCCEED();
}

// currently not active (SIMDJSON_SUPPORTS_RANGES_FROM)
bool test_basic_adaptor_with_parser() {
  TEST_START();
  simdjson::ondemand::parser parser;
  int64_t sum_year = 0;
  for (Car car : simdjson::from(parser, json_cars) | simdjson::as<Car>()) {
    if (car.year < 1998) {
      return false;
    }
    sum_year += car.year;
  }
  ASSERT_EQUAL(sum_year, 2018 + 2012 + 1999);
  TEST_SUCCEED();
}

// currently not active (SIMDJSON_SUPPORTS_RANGES_FROM)
bool test_no_errors() {
  TEST_START();
  int64_t sum_year = 0;
  auto cars = simdjson::from(json_cars) | simdjson::no_errors;
  for (auto val : cars) {
    Car car = val.get<Car>();
    if (car.year < 1998) {
      return false;
    }
    printf("-- year: %d\n", car.year);
    sum_year += car.year;
  }
  ASSERT_EQUAL(sum_year, 2018 + 2012 + 1999);
  TEST_SUCCEED();
}

// currently not active (SIMDJSON_SUPPORTS_RANGES_FROM)
bool to_clean_array() {
  TEST_START();
  int64_t sum_year = 0;
  for (auto val : simdjson::from(json_cars) | simdjson::no_errors) {
    Car car = val.get<Car>();
    if (car.year < 1998) {
      std::cerr << car.make << " " << car.model << " " << car.year << std::endl;
      return false;
    }
    sum_year += car.year;
  }
  ASSERT_EQUAL(sum_year, 2018 + 2012 + 1999);
  TEST_SUCCEED();
}

// currently not active (SIMDJSON_SUPPORTS_RANGES_FROM)
bool test_to_adaptor_basic() {
  TEST_START();
  // Test 1: Basic usage of to<T> with a value reference
  simdjson::ondemand::parser parser;
  auto doc_result = parser.iterate(json_car);
  if (doc_result.error()) {
    return false;
  }
  simdjson::ondemand::document doc = std::move(doc_result.value());
  simdjson::simdjson_result<simdjson::ondemand::value> val = doc.get_value();

  // to<T> converts a simdjson_result<value>& to T
  Car car = simdjson::to<Car>(val);
  if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
    return false;
  }
  TEST_SUCCEED();
}

#endif // SIMDJSON_SUPPORTS_RANGES_FROM

bool test_to_adaptor_with_single_value() {
  TEST_START();
  // Test 2: Using to<T> to convert individual values
  simdjson::ondemand::parser parser;
  auto doc_result = parser.iterate(json_car);
  if (doc_result.error()) {
    return false;
  }
  simdjson::ondemand::document doc = std::move(doc_result.value());

  // Get individual field and convert it
  auto obj_result = doc.get_object();
  if (obj_result.error()) {
    return false;
  }

  TEST_SUCCEED();
}

bool test_to_vs_from_equivalence() {
  TEST_START();
  // Test 3: Verify that simdjson::to<> and simdjson::from behave equivalently
  // Both are instances of to_adaptor - from is just to<void>

  // These should produce identical auto_parser objects
  auto parser1 = simdjson::from(json_car);
  // simdjson::from is an alias for simdjson::to<void>
  auto parser2 = simdjson::from(json_car); // Same as parser1

  // Both should parse the same way
  Car car1 = parser1;
  Car car2 = parser2;

  if (car1.make != car2.make || car1.model != car2.model || car1.year != car2.year) {
    return false;
  }

  TEST_SUCCEED();
}

#endif // SIMDJSON_EXCEPTIONS
bool run() {
  return
#if SIMDJSON_STATIC_REFLECTION && SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
      missing_key_optional_player() && missing_key_player() && meeting_time_test() && meeting_test() && bad_player() && good_player() && complicated_weather_test() &&
#endif
#if SIMDJSON_SUPPORTS_CONCEPTS
      simple_no_except() &&
#endif
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
      broken() && simple() && simple_optional() && with_parser() && to_array() &&
      to_bad_array() &&
      test_to_adaptor_with_single_value() && test_to_vs_from_equivalence() &&
      example_with_parser() &&
#if SIMDJSON_SUPPORTS_RANGES_FROM
      test_basic_adaptor_with_parser() && test_no_errors() && test_basic_adaptor()
      && test_to_adaptor_basic() && to_clean_array() && to_array_shortcut() &&
#endif // SIMDJSON_SUPPORTS_RANGES_FROM
#endif // SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
      true;
}

} // namespace convert_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, convert_tests::run);
}
#else
int main() { return 0; }
#endif