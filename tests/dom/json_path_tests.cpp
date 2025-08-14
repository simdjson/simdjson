/**
 * refer to pathcheck.cpp
 */

#include <iostream>
#include <string>
#include <vector>

using namespace std::string_literals;

#include "simdjson.h"
#include "test_macros.h"

// we define our own asserts to get around NDEBUG
#ifndef ASSERT
#define ASSERT(x)                                                              \
  {                                                                            \
    if (!(x)) {                                                                \
      std::cerr << "Failed assertion " << #x << std::endl;                     \
      return false;                                                            \
    }                                                                          \
  }
#endif

using namespace simdjson;

bool demo() {
#if SIMDJSON_EXCEPTIONS
  std::cout << "demo test" << std::endl;
  auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
  dom::parser parser;
  dom::element cars = parser.parse(cars_json);
  double x = cars.at_path("$[0].tire_pressure[1]");
  if (x != 39.9)
    return false;
  // Iterating through an array of objects
  std::vector<double> measured;
  for (dom::element car_element : cars) {
    dom::object car;
    simdjson::error_code error;
    if ((error = car_element.get(car))) {
      std::cerr << error << std::endl;
      return false;
    }
    double x3 = car.at_path("$.tire_pressure[1]");
    measured.push_back(x3);
  }
  std::vector<double> expected = {39.9, 31, 30};
  if (measured != expected) {
    return false;
  }
#endif
  return true;
}

const padded_string TEST_JSON = R"(
  {
    "/~01abc": [
      0,
      {
        "\\\" 0": [
          "value0",
          "value1"
        ]
      }
    ],
    "0": "0 ok",
    "01": "01 ok",
    "": "empty ok",
    "arr": []
  }
)"_padded;

const padded_string TEST_RFC_JSON = R"(
   {
      "foo": ["bar", "baz"],
      "": 0,
      "a/b": 1,
      "c%d": 2,
      "e^f": 3,
      "g|h": 4,
      "i\\j": 5,
      "k\"l": 6,
      " ": 7,
      "m~n": 8
   }
)"_padded;

bool run_success_test(const padded_string &source, const char *json_path,
                      std::string_view expected_value) {
  std::cout << "Running successful JSONPath test '" << json_path << "' ..."
            << std::endl;
  dom::parser parser;
  dom::element doc;
  auto error = parser.parse(source).get(doc);
  if (error) {
    std::cerr << "cannot parse: " << error << std::endl;
    return false;
  }
  dom::element answer;
  error = doc.at_path(json_path).get(answer);
  if (error) {
    std::cerr << "cannot access pointer: " << error << std::endl;
    return false;
  }
  std::string str_answer = simdjson::minify(answer);
  if (str_answer != expected_value) {
    std::cerr << "They differ!!!" << std::endl;
    std::cerr << "   found    '" << str_answer << "'" << std::endl;
    std::cerr << "   expected '" << expected_value << "'" << std::endl;
  }
  ASSERT_EQUAL(str_answer, expected_value);
  return true;
}

bool run_failure_test(const padded_string &source, const char *json_path,
                      error_code expected_error) {
  std::cout << "Running invalid JSONPath test '" << json_path << "' ..."
            << std::endl;
  dom::parser parser;
  ASSERT_ERROR(parser.parse(source).at_path(json_path).error(), expected_error);
  return true;
}

bool demo_relative_path() {
  TEST_START();
  auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;

  dom::parser parser;
  dom::element cars;
  std::vector<double> measured;
  auto error = parser.parse(cars_json).get(cars);
  if (error) {
    std::cerr << "cannot parse: " << error << std::endl;
    return false;
  }
  dom::array cars_array;
  error = cars.get(cars_array);
  if (error) {
    std::cerr << "cannot get array: " << error << std::endl;
    return false;
  }
  for (auto car_element : cars_array) {
    double x;
    ASSERT_SUCCESS(car_element.at_path(".tire_pressure[1]").get(x));
    measured.push_back(x);
  }

  std::vector<double> expected = {39.9, 31, 30};
  if (measured != expected) {
    return false;
  }
  TEST_SUCCEED();
}

bool many_json_paths() {
  TEST_START();
  auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;

  dom::parser parser;
  dom::element cars;
  std::vector<double> measured;
  ASSERT_SUCCESS(parser.parse(cars_json).get(cars));
  for (int i = 0; i < 3; i++) {
    double x;
    std::string json_path = std::string("$[") + std::to_string(i) +
                            std::string("].tire_pressure[1]");
    ASSERT_SUCCESS(cars.at_path(json_path).get(x));
    measured.push_back(x);
  }

  std::vector<double> expected = {39.9, 31, 30};
  if (measured != expected) {
    return false;
  }
  TEST_SUCCEED();
}

bool many_json_paths_object_array() {
  TEST_START();
  auto dogcatpotato =
      R"( { "dog" : [1,2,3], "cat" : [5, 6, 7], "potato" : [1234]})"_padded;

  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(dogcatpotato).get(doc));
  dom::object obj;
  ASSERT_SUCCESS(doc.get_object().get(obj));
  int64_t x;
  ASSERT_SUCCESS(obj.at_path("$.dog[1]").get(x));
  ASSERT_EQUAL(x, 2);
  ASSERT_SUCCESS(obj.at_path("$.potato[0]").get(x));
  ASSERT_EQUAL(x, 1234);
  TEST_SUCCEED();
}
bool many_json_paths_object() {
  TEST_START();
  auto cfoofoo2 =
      R"( { "c" :{ "foo": { "a": [ 10, 20, 30 ] }}, "d": { "foo2": { "a": [ 10, 20, 30 ] }} , "e": 120 })"_padded;
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(cfoofoo2).get(doc));
  dom::object obj;
  ASSERT_SUCCESS(doc.get_object().get(obj));
  int64_t x;
  ASSERT_SUCCESS(obj.at_path("$.c.foo.a[1]").get(x));
  ASSERT_EQUAL(x, 20);
  ASSERT_SUCCESS(obj.at_path("$.d.foo2.a.2").get(x));
  ASSERT_EQUAL(x, 30);
  ASSERT_SUCCESS(obj.at_path("$.e").get(x));
  ASSERT_EQUAL(x, 120);
  TEST_SUCCEED();
}
bool many_json_paths_array() {
  TEST_START();
  auto cfoofoo2 =
      R"( [ 111, 2, 3, { "foo": { "a": [ 10, 20, 33 ] }}, { "foo2": { "a": [ 10, 20, 30 ] }}, 1001 ])"_padded;
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(cfoofoo2).get(doc));
  dom::array arr;
  ASSERT_SUCCESS(doc.get_array().get(arr));
  int64_t x;
  ASSERT_SUCCESS(arr.at_path("$[3].foo.a[1]").get(x));
  ASSERT_EQUAL(x, 20);
  TEST_SUCCEED();
}
struct car_type {
  std::string make;
  std::string model;
  uint64_t year;
  std::vector<double> tire_pressure;
  car_type(std::string_view _make, std::string_view _model, uint64_t _year,
           std::vector<double> &&_tire_pressure)
      : make{_make}, model{_model}, year(_year), tire_pressure(_tire_pressure) {
  }
};

bool json_path_invalidation() {
  TEST_START();
  auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;

  dom::parser parser;
  dom::element cars;
  std::vector<double> measured;
  ASSERT_SUCCESS(parser.parse(cars_json).get(cars));
  std::vector<car_type> content;
  for (int i = 0; i < 3; i++) {
    dom::object obj;
    std::string json_path =
        std::string("$[") + std::to_string(i) + std::string("]");
    // Each successive at_path call invalidates
    // previously parsed values, strings, objects and array.
    ASSERT_SUCCESS(cars.at_path(json_path).get(obj));
    // We materialize the object.
    std::string_view make;
    ASSERT_SUCCESS(obj["make"].get(make));
    std::string_view model;
    ASSERT_SUCCESS(obj["model"].get(model));
    uint64_t year;
    ASSERT_SUCCESS(obj["year"].get(year));
    // We materialize the array.
    dom::array arr;
    ASSERT_SUCCESS(obj["tire_pressure"].get(arr));
    std::vector<double> values;
    for (auto x : arr) {
      double value_double;
      ASSERT_SUCCESS(x.get(value_double));
      values.push_back(value_double);
    }
    content.emplace_back(make, model, year, std::move(values));
  }
  std::string expected[] = {"Toyota", "Kia", "Toyota"};
  int i = 0;
  for (car_type c : content) {
    std::cout << c.make << " " << c.model << " " << c.year << "\n";
    ASSERT_EQUAL(expected[i++], c.make);
  }
  TEST_SUCCEED();
}

bool json_path_with_wildcard() {
  TEST_START();

  simdjson::padded_string json_string = R"(
  {
    "firstName": "John",
    "lastName" : "doe",
    "age"      : 26,
    "address"  : {
      "streetAddress": "naist street",
      "city"         : "Nara",
      "postalCode"   : "630-0192"
    },
    "phoneNumbers": [
      {
        "type"  : "iPhone",
        "numbers": [
          "0123-4567-8888",
          "0123-4567-8788",
          "0123-4567-8887"
        ]
      },
      {
        "type"  : "home",
        "numbers": [
          "0123-4567-8910",
          "0123-4267-8910",
          "0103-4567-8910"
        ]
      },
      { },
      {
        "type": "office",
        "numbers": [ ]
      }
    ],
    "empty_object": { },
    "empty_array": [ ]
  })"_padded;

  dom::parser parser;
  dom::element parsed_json;
  ASSERT_SUCCESS(parser.parse(json_string).get(parsed_json));
  std::vector<dom::element> values;


  std::string_view string_value;
  std::uint64_t num_value;
  dom::object obj;
  ASSERT_EQUAL(parsed_json.at_path_with_wildcard("$").error(), INVALID_JSON_POINTER);
  ASSERT_EQUAL(parsed_json.at_path_with_wildcard("1").error(), INVALID_JSON_POINTER);
  ASSERT_EQUAL(parsed_json.at_path_with_wildcard("2").error(), INVALID_JSON_POINTER);
  ASSERT_EQUAL(parsed_json.at_path_with_wildcard("a").error(), INVALID_JSON_POINTER);
  ASSERT_EQUAL(parsed_json.at_path_with_wildcard("$2").error(), INVALID_JSON_POINTER);
  ASSERT_EQUAL(parsed_json.at_path_with_wildcard("$a").error(), INVALID_JSON_POINTER);
  // $.*
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.*").get(values));

  ASSERT_SUCCESS(values[0].get(string_value));
  ASSERT_EQUAL(string_value, "John");

  ASSERT_SUCCESS(values[1].get(string_value));
  ASSERT_EQUAL(string_value, "doe");

  ASSERT_SUCCESS(values[2].get(num_value));
  ASSERT_EQUAL(num_value, 26);

  ASSERT_SUCCESS(values[3].get(obj));
  ASSERT_SUCCESS(obj["streetAddress"].get(string_value));
  ASSERT_EQUAL(string_value, "naist street");

  ASSERT_SUCCESS(obj["city"].get(string_value));
  ASSERT_EQUAL(string_value, "Nara");

  // $[*]
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$[*]").get(values));

  ASSERT_SUCCESS(values[0].get(string_value));
  ASSERT_EQUAL(string_value, "John");

  ASSERT_SUCCESS(values[1].get(string_value));
  ASSERT_EQUAL(string_value, "doe");

  ASSERT_SUCCESS(values[2].get(num_value));
  ASSERT_EQUAL(num_value, 26);

  ASSERT_SUCCESS(values[3].get(obj));
  ASSERT_SUCCESS(obj["streetAddress"].get(string_value));
  ASSERT_EQUAL(string_value, "naist street");

  ASSERT_SUCCESS(obj["city"].get(string_value));
  ASSERT_EQUAL(string_value, "Nara");

  // $.address.*
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.address.*").get(values));

  std::vector<std::string> expected = {"naist street", "Nara", "630-0192"};
  for (int i = 0; i < 3; i++) {
    ASSERT_SUCCESS(values[i].get(string_value));
    ASSERT_EQUAL(string_value, expected[i]);
  }

  // $.*.streetAddress
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.*.streetAddress").get(values));

  ASSERT_SUCCESS(values[0].get(string_value));
  ASSERT_EQUAL(string_value, "naist street");

  // $.phoneNumbers[*].numbers[*]
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.phoneNumbers[*].numbers[*]").get(values));

  std::vector<std::string> expected_numbers = {
    "0123-4567-8888",
    "0123-4567-8788",
    "0123-4567-8887",
    "0123-4567-8910",
    "0123-4267-8910",
    "0103-4567-8910"
  };

  for (int i = 0; i < 6; i++) {
    ASSERT_SUCCESS(values[i].get(string_value));
    ASSERT_EQUAL(string_value, expected_numbers[i]);
  }

  // $.phoneNumbers[*].numbers[1]
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.phoneNumbers[*].numbers[1]").get(values));

  ASSERT_SUCCESS(values[0].get(string_value));
  ASSERT_EQUAL(string_value, expected_numbers[1]);

  ASSERT_SUCCESS(values[1].get(string_value));
  ASSERT_EQUAL(string_value, expected_numbers[4]);

  // $.empty_object.*
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.empty_object.*").get(values));

  ASSERT_EQUAL(values.size(), 0);

  // $.empty_array.*
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.empty_array.*").get(values));
  ASSERT_EQUAL(values.size(), 0);

  // $.phoneNumbers.*.numbers[3]
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.phoneNumbers.*.numbers[3]").get(values));

  ASSERT_EQUAL(values.size(), 0);

  TEST_SUCCEED();
}

// for 0.5 version and following (standard compliant)
bool modern_support() {
#if SIMDJSON_EXCEPTIONS
  std::cout << "modern test" << std::endl;
  auto example_json = R"({"key": "value", "array": [0, 1, 2]})"_padded;
  dom::parser parser;
  dom::element example = parser.parse(example_json);
  std::string_view value_str = example.at_path("$.key");
  ASSERT_EQUAL(value_str, "value");
  int64_t array0 = example.at_path("$.array[0]");
  ASSERT_EQUAL(array0, 0);
  array0 = example.at_path("$.array").at_path("$[0]");
  ASSERT_EQUAL(array0, 0);
  ASSERT_ERROR(example.at_path("$.no_such_key").error(), NO_SUCH_FIELD);
  ASSERT_ERROR(example.at_path("$.array[9]").error(), INDEX_OUT_OF_BOUNDS);
  ASSERT_ERROR(example.at_path("$.array.not_a_num").error(), INCORRECT_TYPE);
  ASSERT_ERROR(example.at_path("$.array.").error(), INVALID_JSON_POINTER);
#endif
  return true;
}

int main() {
  if (true && json_path_with_wildcard() && demo() && modern_support() &&
      run_success_test(TEST_RFC_JSON, "$.foo", "[\"bar\",\"baz\"]") &&
      run_success_test(TEST_RFC_JSON, "$.foo[0]", "\"bar\"") &&
      run_success_test(TEST_RFC_JSON, "$.", "0") &&
      run_success_test(TEST_RFC_JSON, "$.a/b", "1") &&
      run_success_test(TEST_RFC_JSON, "$.c%d", "2") &&
      run_success_test(TEST_RFC_JSON, "$.e^f", "3") &&
      run_success_test(TEST_RFC_JSON, "$.g|h", "4") &&
      run_success_test(TEST_RFC_JSON, "$.i\\j", "5") &&
      run_success_test(TEST_RFC_JSON, "$.k\"l", "6") &&
      run_success_test(TEST_RFC_JSON, "$. ", "7") &&
      run_success_test(TEST_RFC_JSON, "$.m~n", "8") &&
      run_success_test(TEST_JSON, "$./~01abc",
                       R"([0,{"\\\" 0":["value0","value1"]}])") &&
      run_success_test(TEST_JSON, "$./~01abc[1]",
                       R"({"\\\" 0":["value0","value1"]})") &&
      run_success_test(TEST_JSON, "$./~01abc[1].\\\" 0",
                       R"(["value0","value1"])") &&
      run_success_test(TEST_JSON, "$.arr", R"([])") // get array
      &&
      run_failure_test(TEST_JSON, R"($./~01abc[1].\\\" 0[2])", NO_SUCH_FIELD) &&
      run_failure_test(TEST_JSON, "$.arr[0]", INDEX_OUT_OF_BOUNDS) &&
      run_failure_test(TEST_JSON, "/~01abc", INVALID_JSON_POINTER) &&
      run_failure_test(TEST_JSON, ".~1abc", NO_SUCH_FIELD) &&
      run_failure_test(TEST_JSON, "./~01abc.01", INVALID_JSON_POINTER) &&
      run_failure_test(TEST_JSON, "./~01abc.", INVALID_JSON_POINTER) &&
      run_failure_test(TEST_JSON, "./~01abc.-", INDEX_OUT_OF_BOUNDS)) {
    std::cout << "Success!" << std::endl;
    return 0;
  } else {
    std::cerr << "Failed!" << std::endl;
    return 1;
  }
}
