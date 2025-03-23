#include "simdjson.h"
#include "test_ondemand.h"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <list>
#include <optional>
#include <map>
#include <unordered_map>

using namespace simdjson;

#if !SIMDJSON_SUPPORTS_DESERIALIZATION

int main(void) {
  printf("This test is only relevant when SIMDJSON_SUPPORTS_DESERIALIZATION is true (C++20)\n");
  return EXIT_SUCCESS;
}
#else

/**
 * A custom type that we want to parse.
 */
struct Car {
  std::string make{};
  std::string model{};
  int year{};
  std::vector<float> tire_pressure{};
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



namespace simdjson {
// This tag_invoke MUST be inside simdjson namespace
template <typename simdjson_value>
auto tag_invoke(deserialize_tag, simdjson_value &val, Car& car) {
  ondemand::object obj;
  auto error = val.get_object().get(obj);
  if (error) {
    return error;
  }
  if ((error = obj["make"].get_string(car.make))) {
    return error;
  }
  if ((error = obj["model"].get_string(car.model))) {
    return error;
  }
  if ((error = obj["year"].get(car.year))) {
    return error;
  }
  if ((error = obj["tire_pressure"].get<std::vector<float>>().get(
           car.tire_pressure))) {
    return error;
  }
  return simdjson::SUCCESS;
}

// suppose we want to filter out all Toyotas
template <typename simdjson_value>
auto tag_invoke(deserialize_tag, simdjson_value &val, std::list<Car>& car) {
  ondemand::array arr;
  auto error = val.get_array().get(arr);
  if (error) {
    return error;
  }
  for (auto v : arr) {
    Car c;
    if ((error = v.get<Car>().get(c))) {
      return error;
    }
    if(c.make != "Toyota") {
      car.push_back(c);
    }
  }
  return simdjson::SUCCESS;
}
} // namespace simdjson


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
  std::vector<Car> expected = {{"Toyota", "Camry", 2018, {40.1f, 39.9f}},
                               {"Kia", "Soul", 2012, {30.1f, 31.0f}},
                               {"Toyota", "Tercel", 1999, {29.8f, 30.0f}}};
  ASSERT_EQUAL(cars.size(), expected.size());
  for (size_t i = 0; i < cars.size(); i++) {
    ASSERT_EQUAL(cars[i], expected[i]);
  }
  TEST_SUCCEED();
}

bool vector_car_deserialize() {
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
#if SIMDJSON_EXCEPTIONS
#if SIMDJSON_REGULAR_VISUAL_STUDIO
  std::vector<Car> cars = doc.get<std::vector<Car>>();  // an exception may be thrown
#else
  std::vector<Car> cars(doc); // an exception may be thrown
#endif
#else
  std::vector<Car> cars;
  if (auto error = doc.get<std::vector<Car>>().get(cars)) {
    std::cerr << error << std::endl;
    return EXIT_FAILURE;
  }
#endif
  std::vector<Car> expected = {{"Toyota", "Camry", 2018, {40.1f, 39.9f}},
                               {"Kia", "Soul", 2012, {30.1f, 31.0f}},
                               {"Toyota", "Tercel", 1999, {29.8f, 30.0f}}};
  ASSERT_EQUAL(cars.size(), expected.size());
  for (size_t i = 0; i < cars.size(); i++) {
    ASSERT_EQUAL(cars[i], expected[i]);
  }
  TEST_SUCCEED();
}


bool list_car_deserialize() {
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
#if SIMDJSON_EXCEPTIONS
#if SIMDJSON_REGULAR_VISUAL_STUDIO
  std::list<Car> cars = doc.get<std::list<Car>>();  // an exception may be thrown
#else
  std::list<Car> cars(doc); // an exception may be thrown
#endif
#else
  std::list<Car> cars;
  if (auto error = doc.get<std::list<Car>>().get(cars)) {
    std::cerr << error << std::endl;
    return EXIT_FAILURE;
  }
#endif
  std::list<Car> expected = {{"Kia", "Soul", 2012, {30.1f, 31.0f}}};
  ASSERT_EQUAL(cars.size(), expected.size());
  ASSERT_EQUAL(cars.front(), expected.front());
  TEST_SUCCEED();
}

bool optional_car_deserialize() {
  TEST_START();
  padded_string json =
      R"( { "car1": { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] }
})"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  [[maybe_unused]] auto error = parser.iterate(json).get(doc);
  std::optional<Car> car;
  error = doc["key not found"].get<std::optional<Car>>().get(car);
  ASSERT_TRUE(!car);
  error = doc["car1"].get<std::optional<Car>>().get(car);
  std::optional<Car> expected = Car{"Toyota", "Camry", 2018, {40.1f, 39.9f}};
  ASSERT_TRUE(car == expected);
  TEST_SUCCEED();
}


bool car_deserialize_to_map() {
  TEST_START();
  std::map<std::string,std::string> expected =  { {"make", "Toyota"}, {"model", "Camry"}};
  padded_string json =
      R"( { "make": "Toyota", "model": "Camry" })"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  [[maybe_unused]] auto error = parser.iterate(json).get(doc);
  static_assert(simdjson::concepts::string_view_keyed_map<std::map<std::string, std::string>>, "should be custom deserializable");
  using map_type = typename std::map<std::string,std::string>;
  static_assert(simdjson::custom_deserializable<map_type,simdjson::ondemand::value>, "should be custom deserializable");
  map_type brands;
  error = doc.get<map_type>().get(brands);
  ASSERT_TRUE(brands == expected);
  TEST_SUCCEED();
}

bool car_deserialize_with_map() {
  TEST_START();
  padded_string json =
      R"( { "car1": { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] }
})"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  [[maybe_unused]] auto error = parser.iterate(json).get(doc);
  std::map<std::string,Car> car;
  error = doc.get<std::map<std::string,Car>>().get(car);
  std::optional<Car> expected = Car{"Toyota", "Camry", 2018, {40.1f, 39.9f}};
  ASSERT_TRUE(car["car1"] == expected);
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
  Car expected = {"Toyota", "Camry", 2018, {40.1f, 39.9f}};
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
  [[maybe_unused]] auto error = parser.iterate_many(json).get(stream);
  std::vector<Car> cars;

#if SIMDJSON_EXCEPTIONS
  for(auto doc : stream) {
    cars.push_back((Car)doc); // an exception may be thrown
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
  std::vector<Car> expected = {{"Toyota", "Camry", 2018, {40.1f, 39.9f}},
                               {"Kia", "Soul", 2012, {30.1f, 31.0f}},
                               {"Toyota", "Tercel", 1999, {29.8f, 30.0f}}};
  ASSERT_EQUAL(cars.size(), expected.size());
  for (size_t i = 0; i < cars.size(); i++) {
    ASSERT_EQUAL(cars[i], expected[i]);
  }
  TEST_SUCCEED();
}

bool car_unique_ptr_deserialize() {
  TEST_START();
  auto const json = R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] })"_padded;
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  [[maybe_unused]] auto error = parser.iterate(json).get(doc);
#if SIMDJSON_EXCEPTIONS
  std::unique_ptr<Car> c(doc);
#else
  std::unique_ptr<Car> c;
  if ((error = doc.get<std::unique_ptr<Car>>().get(c))) {
    std::cerr << error << std::endl;
    return EXIT_FAILURE;
  }
#endif
  ASSERT_EQUAL(c->make, "Toyota");
  TEST_SUCCEED();
}

bool run() { return car_deserialize_with_map()
                    && car_deserialize_to_map()
                    && list_car_deserialize()
                    && optional_car_deserialize()
                    && car_unique_ptr_deserialize()
                    && vector_car_deserialize()
                    && car_stream_deserialize()
                    && car_doc_deserialize()
                    && car_deserialize(); }
} // namespace car_error_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, car_error_tests::run);
}
#endif // SIMDJSON_SUPPORTS_DESERIALIZATION