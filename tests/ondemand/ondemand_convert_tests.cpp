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
  for ([[maybe_unused]] auto val : parser) {
    Car car{};
    if (val.get(car)) {
      continue;
    }
    return false;
  }
  TEST_SUCCEED();
}

bool test_no_errors() {
  TEST_START();
  std::cout << "Running test_no_errors with ranges support" << std::endl;
  auto parser = simdjson::from(json_cars);
  for (auto val : parser.array()) {
    if (val.error() != simdjson::SUCCESS) {
      continue; // Skip errors - this is what no_errors would do
    }
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
  auto parser = simdjson::from(json_cars);
  for (auto val : parser.array()) {
    if (val.error() != simdjson::SUCCESS) {
      continue;
    }
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
  // Test 1: Direct conversion from padded_string_view
  auto parser = simdjson::to<Car>()(json_car);
  Car car = parser.get<Car>();
  if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
    return false;
  }
  TEST_SUCCEED();
}

bool test_to_adaptor_with_parser() {
  TEST_START();
  // Test 2: Using to<T> with explicit parser
  simdjson::ondemand::parser parser;
  auto result = simdjson::to<Car>()(parser, json_car);
  Car car = result.get<Car>();
  if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
    return false;
  }
  TEST_SUCCEED();
}

bool test_to_adaptor_with_value() {
  TEST_START();
  // Test 3: Using to<T> with simdjson_result<ondemand::value>
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json_car);
  simdjson::simdjson_result<simdjson::ondemand::value> val = doc.get_value();
  
  Car car = simdjson::to<Car>()(val);
  if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
    return false;
  }
  TEST_SUCCEED();
}

bool test_to_adaptor_with_range() {
  TEST_START();
  // Test 4: Using to<T> as a range adaptor
  auto cars = simdjson::from(json_cars) | simdjson::to<Car>();
  
  std::vector<Car> car_vec;
  for (auto car : cars) {
    car_vec.push_back(car);
  }
  
  if (car_vec.size() != 3) {
    return false;
  }
  if (car_vec[0].make != "Toyota" || car_vec[1].make != "Kia" || car_vec[2].make != "Toyota") {
    return false;
  }
  TEST_SUCCEED();
}

bool test_to_adaptor_pipe_syntax() {
  TEST_START();
  // Test 5: Using pipe syntax with to<T>
  auto parser = simdjson::from(json_cars);
  auto cars = parser | simdjson::no_errors | simdjson::to<Car>();
  
  int count = 0;
  for (auto car : cars) {
    count++;
    if (car.year < 1998) {
      return false;
    }
  }
  
  if (count != 3) {
    return false;
  }
  TEST_SUCCEED();
}

bool test_to_adaptor_different_types() {
  TEST_START();
  // Test 6: Using to<T> with different types
  simdjson::padded_string json_numbers = R"([1, 2, 3, 4, 5])"_padded;
  auto numbers = simdjson::from(json_numbers) | simdjson::to<int64_t>();
  
  std::vector<int64_t> num_vec;
  for (auto num : numbers) {
    num_vec.push_back(num);
  }
  
  if (num_vec.size() != 5 || num_vec[0] != 1 || num_vec[4] != 5) {
    return false;
  }
  
  // Test with strings
  simdjson::padded_string json_strings = R"(["hello", "world", "test"])"_padded;
  auto strings = simdjson::from(json_strings) | simdjson::to<std::string>();
  
  std::vector<std::string> str_vec;
  for (auto str : strings) {
    str_vec.push_back(str);
  }
  
  if (str_vec.size() != 3 || str_vec[0] != "hello" || str_vec[2] != "test") {
    return false;
  }
  
  TEST_SUCCEED();
}

bool test_to_vs_from_equivalence() {
  TEST_START();
  // Test 7: Verify that simdjson::to and simdjson::from behave equivalently
  // when used as adaptors
  
  // Using from (which is an alias for to<>)
  auto parser1 = simdjson::from(json_car);
  Car car1 = parser1.get<Car>();
  
  // Using to<> directly (same as from)
  auto parser2 = simdjson::to<>()(json_car);
  Car car2 = parser2.get<Car>();
  
  // Both should produce the same result
  if (car1.make != car2.make || car1.model != car2.model || car1.year != car2.year) {
    return false;
  }
  
  // Test with arrays
  auto cars_from = simdjson::from(json_cars) | simdjson::to<Car>();
  auto cars_to = simdjson::to<>()(json_cars) | simdjson::to<Car>();
  
  std::vector<Car> vec_from, vec_to;
  for (auto car : cars_from) {
    vec_from.push_back(car);
  }
  for (auto car : cars_to) {
    vec_to.push_back(car);
  }
  
  if (vec_from.size() != vec_to.size() || vec_from.size() != 3) {
    return false;
  }
  
  for (size_t i = 0; i < vec_from.size(); ++i) {
    if (vec_from[i].make != vec_to[i].make || 
        vec_from[i].model != vec_to[i].model ||
        vec_from[i].year != vec_to[i].year) {
      return false;
    }
  }
  
  TEST_SUCCEED();
}

#endif // SIMDJSON_EXCEPTIONS
bool run() {
  return
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_DESERIALIZATION
      simple() && simple_optional() && with_parser() && to_array() &&
      to_array_shortcut() && to_bad_array() && test_no_errors() &&
      to_clean_array() && test_to_adaptor_basic() && 
      test_to_adaptor_with_parser() && test_to_adaptor_with_value() &&
      test_to_adaptor_with_range() && test_to_adaptor_pipe_syntax() &&
      test_to_adaptor_different_types() && test_to_vs_from_equivalence() &&
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
