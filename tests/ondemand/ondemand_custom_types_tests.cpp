#include "simdjson.h"
#include "test_ondemand.h"

#include <string>
#include <vector>


namespace custom_types_tests {
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_DESERIALIZATION
struct Car {
  std::string make{};
  std::string model{};
  int year{};
  std::vector<double> tire_pressure{};

  friend simdjson::error_code tag_invoke(simdjson::deserialize_tag, auto &val, Car& car) {
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

static_assert(simdjson::custom_deserializable<std::unique_ptr<Car>>, "It should be deserializable");

bool custom_uniqueptr_test() {
  TEST_START();
  simdjson::padded_string json =
      R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  for (auto val : doc) {
    std::unique_ptr<Car> c(val);
    if (c->make != "Toyota" && c->make != "Kia") {
      return false;
    }
    if (c->model != "Camry" && c->model != "Soul" && c->model != "Tercel") {
      return false;
    }
    if (c->year != 2018 && c->year != 2012 && c->year != 1999) {
      return false;
    }
    if (c->tire_pressure.size() != 2) {
      return false;
    }
  }

  TEST_SUCCEED();
}



bool custom_test() {
  TEST_START();
  simdjson::padded_string json =
      R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  for (auto val : doc) {
    Car c(val);
    if (c.make != "Toyota" && c.make != "Kia") {
      return false;
    }
    if (c.model != "Camry" && c.model != "Soul" && c.model != "Tercel") {
      return false;
    }
    if (c.year != 2018 && c.year != 2012 && c.year != 1999) {
      return false;
    }
    if (c.tire_pressure.size() != 2) {
      return false;
    }
  }

  TEST_SUCCEED();
}



bool custom_test_crazy() {
  TEST_START();
  simdjson::padded_string json =
      R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  for (auto val : doc) {
    Car c(val);
    std::cout << c.year << std::endl;
    if (c.make != "Toyota" && c.make != "Kia") {
      return false;
    }
    if (c.model != "Camry" && c.model != "Soul" && c.model != "Tercel") {
      return false;
    }
    if (c.year != 2018 && c.year != 2012 && c.year != 1999) {
      return false;
    }
    if (c.tire_pressure.size() != 2) {
      return false;
    }
  }

  TEST_SUCCEED();
}

bool custom_no_except() {
  TEST_START();
  simdjson::padded_string json =
      R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  for (auto val : doc) {
    Car c;
    auto error = val.get(c);
    if(error) { std::cerr << simdjson::error_message(error) << std::endl; return false; }
  }

  TEST_SUCCEED();
}
#endif // SIMDJSON_EXCEPTIONS
bool run() {
  return
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_DESERIALIZATION
      custom_test() &&
      custom_uniqueptr_test() &&
      custom_no_except() &&
#endif // SIMDJSON_EXCEPTIONS
      true;
}

} // namespace custom_types_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, custom_types_tests::run);
}
