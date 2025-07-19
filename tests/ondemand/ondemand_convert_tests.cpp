#include "simdjson.h"
#include "simdjson/convert.h"
#include "test_ondemand.h"

#include <string>
#include <vector>

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
  Car car = to(json_car);
  if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
    return false;
  }
  TEST_SUCCEED();
}

bool simple_optional() {
  TEST_START();
  auto car = to(json_car).optional<Car>();
  if (!car.has_value() || car->make != "Toyota" || car->model != "Camry" ||
      car->year != 2018) {
    return false;
  }
  TEST_SUCCEED();
}

bool with_parser() {
  TEST_START();
  simdjson::ondemand::parser parser;
  Car car = to(parser, json_car);
  if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
    return false;
  }
  TEST_SUCCEED();
}

bool to_array() {
  TEST_START();
  simdjson::ondemand::parser parser;
  for (auto val : to(json_cars).array()) {
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

#endif // SIMDJSON_EXCEPTIONS
bool run() {
  return
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_DESERIALIZATION
      simple() && simple_optional() && with_parser() && to_array() &&
#endif // SIMDJSON_EXCEPTIONS
      true;
}

} // namespace convert_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, convert_tests::run);
}
