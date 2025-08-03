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
  for (auto val : simdjson::from(json_cars).array()) {
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
  for ([[maybe_unused]] auto val : simdjson::from(json_car)) {
    Car car{};
    if (val.get(car)) {
      continue;
    }
    return false;
  }
  TEST_SUCCEED();
}

// These tests require C++23 ranges support which is currently disabled
#if 0
bool test_no_errors() {
  TEST_START();
  for (auto val : simdjson::from(json_cars) | simdjson::no_errors) {
    Car car{};
    val.get(car);
    if (car.year < 1998) {
      std::cerr << car.make << " " << car.model << " " << car.year << std::endl;
      return false;
    }
  }
  TEST_SUCCEED();
}

bool to_clean_array() {
  TEST_START();
  for (Car const car : simdjson::from(json_cars) | simdjson::to<Car>) {
    if (car.year < 1998) {
      std::cerr << car.make << " " << car.model << " " << car.year << std::endl;
      return false;
    }
  }
  TEST_SUCCEED();
}
#else
// Placeholder functions when ranges support is disabled
bool test_no_errors() { return true; }
bool to_clean_array() { return true; }
#endif

#endif // SIMDJSON_EXCEPTIONS
bool run() {
  return
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_DESERIALIZATION
      simple() && simple_optional() && with_parser() && to_array() &&
      to_array_shortcut() && to_bad_array() && test_no_errors() &&
      to_clean_array() &&
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
