#include "simdjson.h"
#include "test_ondemand.h"

using namespace std;
using namespace simdjson;
using error_code=simdjson::error_code;

#if SIMDJSON_EXCEPTIONS

bool basics_1() {
  TEST_START();

  ondemand::parser parser;
  auto json = padded_string::load("twitter.json");
  ondemand::document doc = parser.iterate(json); // load and parse a file

  simdjson_unused auto unused_doc = doc.get_object();

  TEST_SUCCEED();
}

bool basics_2() {
  TEST_START();

  ondemand::parser parser;
  auto json = "[1,2,3]"_padded; // The _padded suffix creates a simdjson::padded_string instance
  ondemand::document doc = parser.iterate(json); // parse a string

  simdjson_unused auto unused_doc = doc.get_array();

  TEST_SUCCEED();
}

bool basics_3() {
  TEST_START();

  ondemand::parser parser;
  char json[3+SIMDJSON_PADDING];
  strcpy(json, "[1]");
  ondemand::document doc = parser.iterate(json, strlen(json), sizeof(json));

  simdjson_unused auto unused_doc = doc.get_array();

  TEST_SUCCEED();
}

bool using_the_parsed_json_1() {
  TEST_START();

  try {

    ondemand::parser parser;
    auto json = R"(  { "x": 1, "y": 2 }  )"_padded;
    auto doc = parser.iterate(json);
    double y = doc.find_field("y"); // The cursor is now after the 2 (at })
    double x = doc.find_field("x"); // This fails, because there are no more fields after "y"

    cout << x << ", " << y << endl;
  } catch (...) {
    TEST_SUCCEED();
  }
  TEST_FAIL("expected an exception");
}

bool using_the_parsed_json_2() {
  TEST_START();

  ondemand::parser parser;
  auto json = R"(  { "x": 1, "y": 2 }  )"_padded;
  auto doc = parser.iterate(json);
  double y = doc["y"]; // The cursor is now after the 2 (at })
  double x = doc["x"]; // Success: [] loops back around to find "x"

  cout << x << ", " << y << endl;

  TEST_SUCCEED();
}

bool using_the_parsed_json_3() {
  TEST_START();

  ondemand::parser parser;
  auto cars_json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;

  // Iterating through an array of objects
  for (ondemand::object car : parser.iterate(cars_json)) {
    // Accessing a field by name
    cout << "Make/Model: " << std::string_view(car["make"]) << "/" << std::string_view(car["model"]) << endl;

    // Casting a JSON element to an integer
    uint64_t year = car["year"];
    cout << "- This car is " << 2020 - year << "years old." << endl;

    // Iterating through an array of floats
    double total_tire_pressure = 0;
    for (double tire_pressure : car["tire_pressure"]) {
      total_tire_pressure += tire_pressure;
    }
    cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;
  }

  TEST_SUCCEED();
}

bool using_the_parsed_json_4() {
  TEST_START();

  ondemand::parser parser;
  auto points_json = R"( [
      {  "12345" : {"x":12.34, "y":56.78, "z": 9998877}   },
      {  "12545" : {"x":11.44, "y":12.78, "z": 11111111}  }
    ] )"_padded;

  // Parse and iterate through an array of objects
  for (ondemand::object points : parser.iterate(points_json)) {
    for (auto point : points) {
      cout << "id: " << std::string_view(point.unescaped_key()) << ": (";
      cout << point.value()["x"].get_double() << ", ";
      cout << point.value()["y"].get_double() << ", ";
      cout << point.value()["z"].get_int64() << endl;
    }
  }

  TEST_SUCCEED();
}

bool using_the_parsed_json_5() {
  TEST_START();

  auto abstract_json = R"(
    { "str" : { "123" : {"abc" : 3.14 } } }
  )"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(abstract_json);
  cout << doc["str"]["123"]["abc"].get_double() << endl; // Prints 3.14

  TEST_SUCCEED();
}

#endif // SIMDJSON_EXCEPTIONS

int using_the_parsed_json_6_process() {
  auto abstract_json = R"(
    { "str" : { "123" : {"abc" : 3.14 } } }
  )"_padded;
  ondemand::parser parser;

  double value;
  auto doc = parser.iterate(abstract_json);
  auto error = doc["str"]["123"]["abc"].get(value);
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  cout << value << endl; // Prints 3.14

  return EXIT_SUCCESS;
}
bool using_the_parsed_json_6() {
  TEST_START();
  ASSERT_EQUAL(using_the_parsed_json_6_process(), EXIT_SUCCESS);
  TEST_SUCCEED();
}

int main() {
  if (
    true
#if SIMDJSON_EXCEPTIONS
//    && basics_1() // Fails because twitter.json isn't in current directory. Compile test only.
    && basics_2()
    && using_the_parsed_json_1()
    && using_the_parsed_json_2()
    && using_the_parsed_json_3()
    && using_the_parsed_json_4()
    && using_the_parsed_json_5()
#endif
    && using_the_parsed_json_6()
  ) {
    return 0;
  } else {
    return 1;
  }
}
