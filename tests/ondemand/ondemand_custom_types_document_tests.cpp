#include "simdjson.h"
#include "test_ondemand.h"

#include <string>
#include <vector>


namespace doc_custom_types_tests {
#if SIMDJSON_EXCEPTIONS && defined(__cpp_concepts)
struct Car {
  std::string make{};
  std::string model{};
  int64_t year{};
  std::vector<double> tire_pressure{};

  friend error_code tag_invoke(simdjson::deserialize_tag,
                                         auto &val, Car& car) {
    simdjson::ondemand::object obj;
    if (auto const error = val.get_object().get(obj)) {
      return error;
    }
    // Instead of repeatedly obj["something"], we iterate through the object
    // which we expect to be faster.
    for (auto field : obj) {
      simdjson::ondemand::raw_json_string key;
      if (auto const error = field.key().get(key)) {
        return error;
      }
      if (key == "make") {
        if (auto const error = field.value().get_string(car.make)) {
          return error;
        }
      } else if (key == "model") {
        if (auto const error = field.value().get_string(car.model)) {
          return error;
        }
      } else if (key == "year") {
        if (auto const error = field.value().get_int64().get(car.year)) {
          return error;
        }
      } else if (key == "tire_pressure") {
        if (auto const error = field.value().get<std::vector<double>>().get(
                car.tire_pressure)) {
          return error;
        }
      }
    }
    return error_code::SUCCESS;
  }
};

static_assert(simdjson::custom_deserializable<std::unique_ptr<Car>, simdjson::ondemand::value>, "It should be invocable");
static_assert(simdjson::custom_deserializable<std::unique_ptr<Car>, simdjson::ondemand::document>, "Tag_invoke should work with document as well.");
static_assert(simdjson::custom_deserializable<std::vector<Car>, simdjson::ondemand::value>, "It should be invocable");
static_assert(simdjson::custom_deserializable<std::vector<Car>, simdjson::ondemand::document>, "Tag_invoke should work with document as well.");

bool custom_test() {
  TEST_START();
  auto const json = R"( {
      "make": "Toyota",
      "model": "Camry",
      "year": 2018,
      "tire_pressure": [ 40.1, 39.9 ]
 } )"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);

  // creating car directly from "doc":
  const std::unique_ptr<Car> car(doc);
  if (car->make != "Toyota") {
    return false;
  }
  if (car->model != "Camry") {
    return false;
  }
  if (car->year != 2018) {
    return false;
  }
  if (car->tire_pressure.size() != 2) {
    return false;
  }
  TEST_SUCCEED();
}

bool uint8_t_test() {
  TEST_START();
  simdjson::ondemand::parser parser;
  const simdjson::padded_string json =
      R"({"data" : [1,2,3,4]})"_padded;
      simdjson::ondemand::document d = parser.iterate(json);
  std::vector<uint8_t> array = d["data"].get<std::vector<uint8_t>>();
  std::vector<uint8_t> expected = {1, 2, 3, 4};
  ASSERT_EQUAL(array, expected);
  TEST_SUCCEED();
}

bool append_test() {
  TEST_START();
  simdjson::ondemand::parser parser;
  const simdjson::padded_string json =
      R"({"data" : [1,2,3,4]})"_padded;
      simdjson::ondemand::document d = parser.iterate(json);
  std::vector<uint32_t> array = {0, 0};
  d["data"].get<std::vector<uint32_t>>(array);
  std::vector<uint32_t> expected = {0, 0, 1, 2, 3, 4};
  ASSERT_EQUAL(array, expected);
  TEST_SUCCEED();
}

bool readme_test() {
  TEST_START();
  auto const json = R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] })"_padded;
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  std::unique_ptr<Car> c(doc);
  std::cout << c->make << std::endl;
  TEST_SUCCEED();
}


bool simple_document_test() {
  TEST_START();
  simdjson::padded_string json =
      R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] })"_padded;
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  Car c(doc);
  std::cout << c.make << std::endl;
  ASSERT_EQUAL(c.make, "Toyota");
  TEST_SUCCEED();
}

bool custom_test_crazier() {
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
#if SIMDJSON_REGULAR_VISUAL_STUDIO
  std::vector<Car> cars = doc.get<std::vector<Car>>();
#else
  std::vector<Car> cars(doc);
#endif
  for(Car& c : cars) {
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

bool simple_document_test_no_except() {
  TEST_START();
  simdjson::padded_string json =
      R"( { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] })"_padded;
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  Car c;
  auto error = doc.get(c);
  if(error) { std::cerr << simdjson::error_message(error); return false; }
  std::cout << c.make << std::endl;
  ASSERT_EQUAL(c.make, "Toyota");
  TEST_SUCCEED();
}
#endif // SIMDJSON_EXCEPTIONS
bool run() {
  return
#if SIMDJSON_EXCEPTIONS && defined(__cpp_concepts)
      append_test() &&
      uint8_t_test() &&
      readme_test() &&
      custom_test() &&
      simple_document_test() &&
      simple_document_test_no_except() &&
      custom_test_crazier() &&
#endif // SIMDJSON_EXCEPTIONS
      true;
}

} // namespace doc_custom_types_tests

int main(const int argc, char *argv[]) {
  return test_main(argc, argv, doc_custom_types_tests::run);
}
