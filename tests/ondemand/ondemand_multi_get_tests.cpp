#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace multi_get_tests {
#ifdef __cpp_concepts
struct Car {
  std::string_view make;
  std::string_view model;
  std::uint64_t year = 0;

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
    auto [make, model, year] = obj.find_all("make", "model", "year");
    if (make.error() != SUCCESS) {
      return make.error();
    }
    if (model.error() != SUCCESS) {
      return model.error();
    }
    if (year.error() != SUCCESS) {
      return year.error();
    }
    make.get(car.make);
    model.get(car.model);
    year.get(car.year);
    return car;
  }
};

bool get_3() {
  TEST_START();
  auto json = R"(   {"one": 1, "two": 2, "three": 153 }  )"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ondemand::object obj = doc.get_object();
  auto [one, two, three] = obj.find_all("one", "two", "three");
  int long one_value, two_value, three_value;
  ASSERT_SUCCESS(one.get_int64().get(one_value));
  ASSERT_SUCCESS(two.get_int64().get(two_value));
  ASSERT_SUCCESS(three.get_int64().get(three_value));
  ASSERT_EQUAL(one_value, 1);
  ASSERT_EQUAL(two_value, 2);
  ASSERT_EQUAL(three_value, 153);
  TEST_SUCCEED();
}

bool one_key_not_found() {
  TEST_START();
  auto json = R"(   {"one": 1, "three": 153 }  )"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  auto [one, two, three] = doc.get_object().find_all("one", "two", "three");
  int long one_value, three_value;
  ASSERT_SUCCESS(one.get_int64().get(one_value));
  ASSERT_EQUAL(two.get_int64().error(), UNINITIALIZED);
  ASSERT_SUCCESS(three.get_int64().get(three_value));
  ASSERT_EQUAL(one_value, 1);
  ASSERT_EQUAL(three_value, 153);
  TEST_SUCCEED();
}

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
      get_3() &&
      one_key_not_found() &&
      car_example() &&
#endif
      true;
}

} // namespace multi_get_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, multi_get_tests::run);
}
