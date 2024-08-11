#include "simdjson.h"
#include "test_ondemand.h"

#include <string>
#include <vector>

#ifdef __cpp_concepts

template <typename T>
struct is_unique_ptr : std::false_type {
};


template <typename T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {
};

template <typename T>
concept is_unique_ptr_v = is_unique_ptr<T>::value;


namespace simdjson {

// this is to demonstrate that we can use concept; otherwise a simple
// type_identity<unique_ptr<T>> as the second argument would have done the job.
//
// This tag_invoke MUST be inside simdjson namespace
template <typename T>
  requires is_unique_ptr_v<T>
auto tag_invoke(deserialize_tag, std::type_identity<T>, ondemand::value &val) {
  using type = typename T::element_type;
  return simdjson_result{std::make_unique<type>(val.template get<type>())};
}

} // namespace simdjson


#endif // __cpp_concepts


namespace custom_types_tests {
#if SIMDJSON_EXCEPTIONS && defined(__cpp_concepts)
struct Car {
  std::string make{};
  std::string model{};
  int year{};
  std::vector<double> tire_pressure{};

  friend simdjson_result<Car>
  tag_invoke(simdjson::deserialize_tag, std::type_identity<Car>, auto &val) {
    simdjson::ondemand::object obj;
    auto error = val.get_object().get(obj);
    if (error) {
      return error;
    }
    Car car{};
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
    return car;
  }
};

static_assert(simdjson::tag_invocable<simdjson::deserialize_tag,
                            std::type_identity<std::unique_ptr<Car>>, simdjson::ondemand::value &>,
              "It should be invocable");

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
#if SIMDJSON_EXCEPTIONS && defined(__cpp_concepts)
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
