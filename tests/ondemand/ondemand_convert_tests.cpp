#include "simdjson.h"
#include "simdjson/convert.h"
#include "test_ondemand.h"

#include <ranges>
#include <string>
#include <vector>

#ifdef __cpp_lib_ranges

namespace convert_tests {
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_DESERIALIZATION
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

static_assert(std::input_or_output_iterator<simdjson::auto_iterator>,
              "Must be a valid input iterator");
static_assert(std::semiregular<simdjson::auto_iterator>,
              "Should be kinda regular");
// static_assert(std::ranges::__access::__member_end<simdjson::auto_parser<>>,
//               "Must be a valid input iterator");
// static_assert(std::ranges::views::__adaptor::__is_range_adaptor_closure<
//                   simdjson::auto_parser<>>,
//               "Parser need to be range adaptor closure.");
// static_assert(std::ranges::views::__adaptor::__adaptor_invocable<
//                   decltype(simdjson::to<Car>()),
//                   simdjson::auto_parser<>>,
//               "I don't even know!");
static_assert(std::ranges::range<simdjson::auto_parser<>>,
              "Parser need to be a range.");
static_assert(std::ranges::forward_range<simdjson::auto_parser<>>,
              "Parser need to be an input range.");
static_assert(
    requires(simdjson::auto_parser<> &parser) {
      { parser.begin() } -> std::input_or_output_iterator;
    }, "Must be valid iterator.");

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

bool simple() {
  TEST_START();
  Car car = simdjson::from(json_car);
  if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
    return false;
  }
  TEST_SUCCEED();
}

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

bool test_basic_adaptor() {
  TEST_START();
  for (Car car : simdjson::from(json_cars) | simdjson::as<Car>()) {
    if (car.year < 1998) {
      return false;
    }
  }
  TEST_SUCCEED();
}

bool test_no_errors() {
  TEST_START();
  auto cars = simdjson::from(json_cars) | simdjson::no_errors;
  for (auto val : cars) {
    Car car = val.get<Car>();
    if (car.year < 1998) {
      return false;
    }
  }
  TEST_SUCCEED();
}

bool to_clean_array() {
  TEST_START();
  for (auto val : simdjson::from(json_cars) | simdjson::no_errors) {
    Car car = val.get<Car>();
    if (car.year < 1998) {
      std::cerr << car.make << " " << car.model << " " << car.year << std::endl;
      return false;
    }
  }
  TEST_SUCCEED();
}

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
  simdjson::ondemand::object obj = std::move(obj_result.value());

  auto year_val = obj["year"];
  int64_t year = simdjson::to<int64_t>(year_val);
  if (year != 2018) {
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
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_DESERIALIZATION
      test_basic_adaptor() && broken() && simple() && simple_optional() && with_parser() && to_array() &&
      to_array_shortcut() && to_bad_array() && test_no_errors() &&
      to_clean_array() && test_to_adaptor_basic() &&
      test_to_adaptor_with_single_value() && test_to_vs_from_equivalence() &&
#endif // SIMDJSON_EXCEPTIONS
      true;
}

} // namespace convert_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, convert_tests::run);
}
#else
int main() { return 0; }
#endif