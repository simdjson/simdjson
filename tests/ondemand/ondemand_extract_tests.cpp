#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace multi_get_tests {
#ifdef __cpp_concepts
struct Car {
  std::string_view make;
  std::string_view model;
  std::int64_t year = 0;

  static simdjson_result<Car> create(auto& value) {
    simdjson::ondemand::object obj;
    auto error = value.get_object().get(obj);
    if (error) {
      return error;
    }
    Car car{};
    // Instead of this:
    //     for (auto field : obj) {
    //       simdjson::ondemand::raw_json_string key;
    //       error = field.key().get(key);
    //       if (error) {
    //         return error;
    //       }
    //       if (key == "make") {
    //         error = field.value().get_string(car.make);
    //         if (error) {
    //           return error;
    //         }
    //       } else if (key == "model") {
    //         error = field.value().get_string(car.model);
    //         if (error) {
    //           return error;
    //         }
    //       } else if (key == "year") {
    //         error = field.value().get(car.year);
    //         if (error) {
    //           return error;
    //         }
    //     }
    //
    // we can do this now:
    obj.extract(
      ondemand::to{"make", car.make},
      ondemand::to{"model", car.model},
      ondemand::to{"year", [&car](auto val) {
        car.year = val;
      }});
    return car;
  }
};

bool car_example() {
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
    auto car = Car::create(val);
    ASSERT_EQUAL(car.error(), SUCCESS);
    Car const c(car.value());
    if (c.make != "Toyota" && c.make != "Kia") {
      return false;
    }
    if (c.model != "Camry" && c.model != "Soul" && c.model != "Tercel") {
      return false;
    }
    if (c.year != 2018 && c.year != 2012 && c.year != 1999) {
      return false;
    }
  }

  TEST_SUCCEED();
}

#endif
bool run() {
  return
#ifdef __cpp_concepts
      car_example() &&
#endif
      true;
}

} // namespace multi_get_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, multi_get_tests::run);
}
