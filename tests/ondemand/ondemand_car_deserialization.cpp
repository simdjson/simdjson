#include "simdjson.h"
#include "test_ondemand.h"
#include <iostream>
#include <vector>

using namespace simdjson;

/**
 * A custom type that we want to parse.
 */
struct Car {
  std::string make{};
  std::string model{};
  int64_t year{};
  std::vector<double> tire_pressure{};
  bool operator==(const Car &other) const {
    return make == other.make && model == other.model && year == other.year &&
           tire_pressure == other.tire_pressure;
  }
};

std::ostream &operator<<(std::ostream &os, const Car &c) {
  os << c.make << " " << c.model << " " << c.year << " ";
  for (auto p : c.tire_pressure) {
    os << p << " ";
  }
  return os;
}

#if !SIMDJSON_SUPPORTS_DESERIALIZATION
// This code is not necessary if you have a C++20 compliant system:
template <>
simdjson_inline simdjson_result<std::vector<double>>
simdjson::ondemand::value::get() noexcept {
  ondemand::array array;
  auto error = get_array().get(array);
  if (error) {
    return error;
  }
  std::vector<double> vec;
  for (auto v : array) {
    double val;
    error = v.get_double().get(val);
    if (error) {
      return error;
    }
    vec.push_back(val);
  }
  return vec;
}
#endif

template <>
simdjson_inline simdjson_result<Car> simdjson::ondemand::value::get() noexcept {
  ondemand::object obj;
  auto error = get_object().get(obj);
  if (error) {
    return error;
  }
  Car car;
  if ((error = obj["make"].get_string(car.make))) {
    return error;
  }
  if ((error = obj["model"].get_string(car.model))) {
    return error;
  }
  if ((error = obj["year"].get_int64().get(car.year))) {
    return error;
  }
  if ((error = obj["tire_pressure"].get<std::vector<double>>().get(
           car.tire_pressure))) {
    return error;
  }
  return car;
}

template <>
simdjson_inline simdjson_result<Car>
simdjson::ondemand::document::get() &noexcept {
  ondemand::object obj;
  auto error = get_object().get(obj);
  if (error) {
    return error;
  }
  Car car;
  if ((error = obj["make"].get_string(car.make))) {
    return error;
  }
  if ((error = obj["model"].get_string(car.model))) {
    return error;
  }
  if ((error = obj["year"].get_int64().get(car.year))) {
    return error;
  }
  if ((error = obj["tire_pressure"].get<std::vector<double>>().get(
           car.tire_pressure))) {
    return error;
  }
  return car;
}


template <>
simdjson_inline simdjson_result<Car>
simdjson::ondemand::document_reference::get() &noexcept {
  ondemand::object obj;
  auto error = get_object().get(obj);
  if (error) {
    return error;
  }
  Car car;
  if ((error = obj["make"].get_string(car.make))) {
    return error;
  }
  if ((error = obj["model"].get_string(car.model))) {
    return error;
  }
  if ((error = obj["year"].get_int64().get(car.year))) {
    return error;
  }
  if ((error = obj["tire_pressure"].get<std::vector<double>>().get(
           car.tire_pressure))) {
    return error;
  }
  return car;
}

int main_should_compile(void) {
  padded_string json =
      R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
])"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  [[maybe_unused]] auto doc_error = parser.iterate(json).get(doc);
  for (auto val : doc) {
#if SIMDJSON_EXCEPTIONS
    Car c(val); // an exception may be thrown
#else
    Car c;
    if (auto error = val.get<Car>().get(c)) {
      std::cerr << error << std::endl;
      return EXIT_FAILURE;
    }
#endif
  }
  return EXIT_SUCCESS;
}

namespace car_error_tests {
bool car_deserialize() {
  TEST_START();
  padded_string json =
      R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
])"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  [[maybe_unused]] auto doc_error = parser.iterate(json).get(doc);
  std::vector<Car> cars;
  for (auto val : doc) {
#if SIMDJSON_EXCEPTIONS
    Car c(val); // an exception may be thrown
#else
    Car c;
    if (auto error = val.get<Car>().get(c)) {
      std::cerr << error << std::endl;
      return EXIT_FAILURE;
    }
#endif
    cars.push_back(c);
    std::cout << c.make << std::endl;
  }
  std::vector<Car> expected = {{"Toyota", "Camry", 2018, {40.1, 39.9}},
                               {"Kia", "Soul", 2012, {30.1, 31.0}},
                               {"Toyota", "Tercel", 1999, {29.8, 30.0}}};
  ASSERT_EQUAL(cars.size(), expected.size());
  for (size_t i = 0; i < cars.size(); i++) {
    ASSERT_EQUAL(cars[i], expected[i]);
  }
  TEST_SUCCEED();
}

bool car_doc_deserialize() {
  TEST_START();
  padded_string json = R"(  { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] }
)"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  [[maybe_unused]] auto doc_error = parser.iterate(json).get(doc);
#if SIMDJSON_EXCEPTIONS
  Car c(doc);   // an exception may be thrown
#else
  Car c;
  if (auto error = doc.get<Car>().get(c)) {
    std::cerr << error << std::endl;
    return EXIT_FAILURE;
  }
#endif
  Car expected = {"Toyota", "Camry", 2018, {40.1, 39.9}};
  ASSERT_EQUAL(c, expected);
  TEST_SUCCEED();
}

bool car_stream_deserialize() {
  TEST_START();
  padded_string json =
      R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] }
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] }
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
)"_padded;
  ondemand::parser parser;
  ondemand::document_stream stream;
  auto error = parser.iterate_many(json).get(stream);
  if(error) {
    std::cerr << error << std::endl;
    return EXIT_FAILURE;
  }
  std::vector<Car> cars;

#if SIMDJSON_EXCEPTIONS
  for(ondemand::document_reference doc : stream) {
    //Car c(doc);   // an exception may be thrown
    cars.push_back(Car(doc)); // an exception may be thrown
  }
#else
  for(auto doc : stream) {
    Car c;
    if ((error = doc.get<Car>().get(c))) {
      std::cerr << error << std::endl;
      return EXIT_FAILURE;
    }
    cars.push_back(c);
  }
#endif
  std::vector<Car> expected = {{"Toyota", "Camry", 2018, {40.1, 39.9}},
                               {"Kia", "Soul", 2012, {30.1, 31.0}},
                               {"Toyota", "Tercel", 1999, {29.8, 30.0}}};
  ASSERT_EQUAL(cars.size(), expected.size());
  for (size_t i = 0; i < cars.size(); i++) {
    ASSERT_EQUAL(cars[i], expected[i]);
  }
  TEST_SUCCEED();
}

bool run() { return car_stream_deserialize() && car_doc_deserialize() && car_deserialize(); }
} // namespace car_error_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, car_error_tests::run);
}