/***************
 * We refer the programmer to
 * JavaScript Object Notation (JSON) Pointer
 * https://tools.ietf.org/html/rfc6901
 */

#include <iostream>

#include "simdjson.h"
#include "test_macros.h"

// we define our own asserts to get around NDEBUG
#ifndef ASSERT
#define ASSERT(x)                                                       \
{    if (!(x)) {                                                        \
        std::cerr << "Failed assertion " << #x << std::endl;            \
        return false;                                                   \
    }                                                                   \
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
  double x = cars.at_pointer("/0/tire_pressure/1");
  if(x != 39.9) return false;
  // Iterating through an array of objects
  std::vector<double> measured;
  for (dom::element car_element : cars) {
    dom::object car;
    simdjson::error_code error;
    if ((error = car_element.get(car))) { std::cerr << error << std::endl; return false; }
    double x3 = car.at_pointer("/tire_pressure/1");
    measured.push_back(x3);
  }
  std::vector<double> expected = {39.9, 31, 30};
  if(measured != expected) {
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

bool json_pointer_success_test(const padded_string & source, const char *json_pointer, std::string_view expected_value) {
  std::cout << "Running successful JSON pointer test '" << json_pointer << "' ..." << std::endl;
  dom::parser parser;
  dom::element doc;
  auto error = parser.parse(source).get(doc);
  if(error) { std::cerr << "cannot parse: " << error << std::endl; return false; }
  dom::element answer;
  error = doc.at_pointer(json_pointer).get(answer);
  if(error) { std::cerr << "cannot access pointer: " << error << std::endl; return false; }
  std::string str_answer = simdjson::minify(answer);
  if(str_answer != expected_value) {
    std::cerr << "They differ!!!" << std::endl;
    std::cerr << "   found    '" << str_answer << "'" << std::endl;
    std::cerr << "   expected '" << expected_value << "'" << std::endl;
  }
  ASSERT_EQUAL(str_answer, expected_value);
  return true;
}

bool json_pointer_failure_test(const padded_string & source, const char *json_pointer, error_code expected_error) {
  std::cout << "Running invalid JSON pointer test '" << json_pointer << "' ..." << std::endl;
  dom::parser parser;
  ASSERT_ERROR(parser.parse(source).at_pointer(json_pointer).error(), expected_error);
  return true;
}

#ifdef SIMDJSON_ENABLE_DEPRECATED_API
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_DEPRECATED_WARNING
// for pre 0.4 users (not standard compliant)
bool legacy_support() {
#if SIMDJSON_EXCEPTIONS
  std::cout << "legacy test" << std::endl;
  auto legacy_json = R"({"key": "value", "array": [0, 1, 2]})"_padded;
  dom::parser parser;
  dom::element legacy = parser.parse(legacy_json);
  std::string_view value_str = legacy.at("key");
  ASSERT_EQUAL(value_str, "value");
  int64_t array0 = legacy.at("array/0");
  ASSERT_EQUAL(array0, 0);
  array0 = legacy.at("array").at("0");
  ASSERT_EQUAL(array0, 0);
  ASSERT_ERROR(legacy.at("no_such_key").error(), NO_SUCH_FIELD);
  ASSERT_ERROR(legacy.at("array/9").error(), INDEX_OUT_OF_BOUNDS);
  ASSERT_ERROR(legacy.at("array/not_a_num").error(), INCORRECT_TYPE);
  ASSERT_ERROR(legacy.at("array/").error(), INVALID_JSON_POINTER);
#endif
  return true;
}
SIMDJSON_POP_DISABLE_WARNINGS
#endif // #if SIMDJSON_ENABLE_DEPRECATED_API

// for 0.5 version and following (standard compliant)
bool modern_support() {
#if SIMDJSON_EXCEPTIONS
  std::cout << "modern test" << std::endl;
  auto example_json = R"({"key": "value", "array": [0, 1, 2]})"_padded;
  dom::parser parser;
  dom::element example = parser.parse(example_json);
  std::string_view value_str = example.at_pointer("/key");
  ASSERT_EQUAL(value_str, "value");
  int64_t array0 = example.at_pointer("/array/0");
  ASSERT_EQUAL(array0, 0);
  array0 = example.at_pointer("/array").at_pointer("/0");
  ASSERT_EQUAL(array0, 0);
  ASSERT_ERROR(example.at_pointer("/no_such_key").error(), NO_SUCH_FIELD);
  ASSERT_ERROR(example.at_pointer("/array/9").error(), INDEX_OUT_OF_BOUNDS);
  ASSERT_ERROR(example.at_pointer("/array/not_a_num").error(), INCORRECT_TYPE);
  ASSERT_ERROR(example.at_pointer("/array/").error(), INVALID_JSON_POINTER);
#endif
  return true;
}
bool issue1142() {
#if SIMDJSON_EXCEPTIONS
  std::cout << "issue 1142" << std::endl;
  auto example_json = R"([1,2,{"1":"bla"}])"_padded;
  dom::parser parser;
  dom::element example = parser.parse(example_json);
  auto e0 = dom::array(example).at(0).at_pointer("");
  ASSERT_EQUAL(std::string("1"), simdjson::minify(e0))
  auto o = dom::array(example).at(2).at_pointer("");
  ASSERT_EQUAL(std::string(R"({"1":"bla"})"), simdjson::minify(o))
  std::string_view s0 = dom::array(example).at(2).at_pointer("/1").at_pointer("");
  if(s0 != "bla") {
    std::cerr << s0 << std::endl;
    return false;
  }
  auto example_json2 = R"("just a string")"_padded;
  dom::element example2 = parser.parse(example_json2).at_pointer("");
  if(std::string_view(example2) != "just a string") {
    std::cerr << std::string_view(example2) << std::endl;
    return false;
  }
  auto example_json3 = R"([])"_padded;
  dom::element example3 = parser.parse(example_json3).at_pointer("");
  ASSERT_EQUAL(std::string(R"([])"), simdjson::minify(example3));

  const char * input_array = "[]";
  size_t input_length = std::strlen(input_array);
  auto element4 = parser.parse(input_array, input_length).at_pointer("");;
  ASSERT_EQUAL(std::string(R"([])"), simdjson::minify(element4));

#endif
  return true;
}

int main() {
  if (true
    && demo()
    && issue1142()
#ifdef SIMDJSON_ENABLE_DEPRECATED_API
    && legacy_support()
#endif
    && modern_support()
    && json_pointer_success_test(TEST_RFC_JSON, "", R"({"foo":["bar","baz"],"":0,"a/b":1,"c%d":2,"e^f":3,"g|h":4,"i\\j":5,"k\"l":6," ":7,"m~n":8})")
    && json_pointer_success_test(TEST_RFC_JSON, "/foo", "[\"bar\",\"baz\"]")
    && json_pointer_success_test(TEST_RFC_JSON, "/foo/0", "\"bar\"")
    && json_pointer_success_test(TEST_RFC_JSON, "/", "0")
    && json_pointer_success_test(TEST_RFC_JSON, "/a~1b", "1")
    && json_pointer_success_test(TEST_RFC_JSON, "/c%d", "2")
    && json_pointer_success_test(TEST_RFC_JSON, "/e^f", "3")
    && json_pointer_success_test(TEST_RFC_JSON, "/g|h", "4")
    && json_pointer_success_test(TEST_RFC_JSON, "/i\\j", "5")
    && json_pointer_success_test(TEST_RFC_JSON, "/k\"l", "6")
    && json_pointer_success_test(TEST_RFC_JSON, "/ ", "7")
    && json_pointer_success_test(TEST_RFC_JSON, "/m~0n", "8")
    && json_pointer_success_test(TEST_JSON, "",R"({"/~01abc":[0,{"\\\" 0":["value0","value1"]}],"0":"0 ok","01":"01 ok","":"empty ok","arr":[]})")
    && json_pointer_success_test(TEST_JSON, "/~1~001abc",R"([0,{"\\\" 0":["value0","value1"]}])")
    && json_pointer_success_test(TEST_JSON, "/~1~001abc/1",R"({"\\\" 0":["value0","value1"]})")
    && json_pointer_success_test(TEST_JSON, "/~1~001abc/1/\\\" 0",R"(["value0","value1"])")
    && json_pointer_success_test(TEST_JSON, "/~1~001abc/1/\\\" 0/0", "\"value0\"")
    && json_pointer_success_test(TEST_JSON, "/~1~001abc/1/\\\" 0/1", "\"value1\"")
    && json_pointer_failure_test(TEST_JSON, "/~1~001abc/1/\\\" 0/2", INDEX_OUT_OF_BOUNDS) // index actually out of bounds
    && json_pointer_success_test(TEST_JSON, "/arr", R"([])") // get array
    && json_pointer_failure_test(TEST_JSON, "/arr/0", INDEX_OUT_OF_BOUNDS) // array index 0 out of bounds on empty array
    && json_pointer_failure_test(TEST_JSON, "~1~001abc", INVALID_JSON_POINTER)
    && json_pointer_success_test(TEST_JSON, "/0", "\"0 ok\"") // object index with integer-ish key
    && json_pointer_success_test(TEST_JSON, "/01", "\"01 ok\"") // object index with key that would be an invalid integer
    && json_pointer_success_test(TEST_JSON, "", R"({"/~01abc":[0,{"\\\" 0":["value0","value1"]}],"0":"0 ok","01":"01 ok","":"empty ok","arr":[]})") // object index with empty key
    && json_pointer_failure_test(TEST_JSON, "/~01abc", NO_SUCH_FIELD) // Test that we don't try to compare the literal key
    && json_pointer_failure_test(TEST_JSON, "/~1~001abc/01", INVALID_JSON_POINTER) // Leading 0 in integer index
    && json_pointer_failure_test(TEST_JSON, "/~1~001abc/", INVALID_JSON_POINTER) // Empty index to array
    && json_pointer_failure_test(TEST_JSON, "/~1~001abc/-", INDEX_OUT_OF_BOUNDS) // End index is always out of bounds
  ) {
    std::cout << "Success!" << std::endl;
    return 0;
  } else {
    std::cerr << "Failed!" << std::endl;
    return 1;
  }
}
