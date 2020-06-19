#include <iostream>
#include "simdjson.h"

using namespace std;
using namespace simdjson;

#ifdef SIMDJSON_CPLUSPLUS17
SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
void basics_error_1() {
  dom::parser parser;
  auto json = "1"_padded;

  auto [doc, error] = parser.parse(json); // doc is a dom::element
  if (error) { cerr << error << endl; exit(1); }
  // Use document here now that we've checked for the error
}
SIMDJSON_POP_DISABLE_WARNINGS
#endif

void basics_error_2() {
  dom::parser parser;
  auto json = "1"_padded;

  dom::element doc;
  simdjson::error_code error;
  parser.parse(json).tie(doc, error); // <-- Assigns to doc and error just like "auto [doc, error]"}
}

void basics_error_3() {
  auto cars_json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;
  dom::parser parser;
  dom::array cars;
  simdjson::error_code error;
  if (!parser.parse(cars_json).get(cars, error)) { cerr << error << endl; exit(1); }

  // Iterating through an array of objects
  for (dom::element car_element : cars) {
      dom::object car;
      if (!car_element.get(car, error)) { cerr << error << endl; exit(1); }

      // Accessing a field by name
      std::string_view make, model;
      if (!car["make"].get(make, error)) { cerr << error << endl; exit(1); }
      if (!car["model"].get(model, error)) { cerr << error << endl; exit(1); }
      cout << "Make/Model: " << make << "/" << model << endl;

      // Casting a JSON element to an integer
      uint64_t year;
      if (!car["year"].get(year, error)) { cerr << error << endl; exit(1); }
      cout << "- This car is " << 2020 - year << "years old." << endl;

      // Iterating through an array of floats
      double total_tire_pressure = 0;
      dom::array tire_pressure_array;
      if (!car["tire_pressure"].get(tire_pressure_array, error)) { cerr << error << endl; exit(1); }
      for (dom::element tire_pressure_element : tire_pressure_array) {
          double tire_pressure;
          if (!tire_pressure_element.get(tire_pressure, error)) { cerr << error << endl; exit(1); }
          total_tire_pressure += tire_pressure;
      }
      cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;

      // Writing out all the information about the car
      for (auto field : car) {
          cout << "- " << field.key << ": " << field.value << endl;
      }
  }
}


void basics_error_4() {
  auto abstract_json = R"( [
      {  "12345" : {"a":12.34, "b":56.78, "c": 9998877}   },
      {  "12545" : {"a":11.44, "b":12.78, "c": 11111111}  }
    ] )"_padded;
  dom::parser parser;
  dom::array rootarray;
  simdjson::error_code error;
  parser.parse(abstract_json).get(rootarray, error);
  if (error) { cerr << error << endl; exit(1); }
  // Iterate through an array of objects
  for (dom::element elem : rootarray) {
      dom::object obj;
      if (!elem.get(obj, error)) { cerr << error << endl; exit(1); }
      for (auto & key_value : obj) {
          cout << "key: " << key_value.key << " : ";
          dom::object innerobj;
          if (!key_value.value.get(innerobj, error)) { cerr << error << endl; exit(1); }

          double va, vb;
          if (!innerobj["a"].get(va, error)) { cerr << error << endl; exit(1); }
          cout << "a: " << va << ", ";
          if (!innerobj["b"].get(vb, error)) { cerr << error << endl; exit(1); }
          cout << "b: " << vb << ", ";

          int64_t vc;
          if (!innerobj["c"].get(vc, error)) { cerr << error << endl; exit(1); }
          cout << "c: " << vc << endl;
      }
  }
}

void basics_error_5() {
  auto abstract_json = R"(
    {  "str" : { "123" : {"abc" : 3.14 } } } )"_padded;
  dom::parser parser;
  double v;
  simdjson::error_code error;
  parser.parse(abstract_json)["str"]["123"]["abc"].get(v, error);
  if (error) { cerr << error << endl; exit(1); }
  cout << "number: " << v << endl;
}




#ifdef SIMDJSON_CPLUSPLUS17
void basics_error_3_cpp17() {
  auto cars_json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;
  dom::parser parser;
  auto [cars, error] = parser.parse(cars_json).get<dom::array>();
  if (error) { cerr << error << endl; exit(1); }

  // Iterating through an array of objects
  for (dom::element car_element : cars) {
    dom::object car;
    car_element.get(car, error);
    if (error) { cerr << error << endl; exit(1); }

    // Accessing a field by name
    dom::element make, model;
    car["make"].tie(make, error);
    if (error) { cerr << error << endl; exit(1); }
    car["model"].tie(model, error);
    if (error) { cerr << error << endl; exit(1); }
    cout << "Make/Model: " << make << "/" << model << endl;

    // Casting a JSON element to an integer
    uint64_t year;
    car["year"].get(year, error);
    if (error) { cerr << error << endl; exit(1); }
    cout << "- This car is " << 2020 - year << "years old." << endl;

    // Iterating through an array of floats
    double total_tire_pressure = 0;
    dom::array tire_pressure_array;
    car["tire_pressure"].get(tire_pressure_array, error);
    if (error) { cerr << error << endl; exit(1); }
    for (dom::element tire_pressure_element : tire_pressure_array) {
      double tire_pressure;
      tire_pressure_element.get(tire_pressure, error);
      if (error) { cerr << error << endl; exit(1); }
      total_tire_pressure += tire_pressure;
    }
    cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;

    // Writing out all the information about the car
    for (auto [key, value] : car) {
      cout << "- " << key << ": " << value << endl;
    }
  }
}
#endif

// See https://github.com/miloyip/nativejson-benchmark/blob/master/src/tests/simdjsontest.cpp
simdjson::dom::parser parser{};

bool parse_double(const char *j, double &d) {
  simdjson::error_code error;
  parser.parse(j, std::strlen(j))
        .at(0)
        .get(d, error);
  if (error) { return false; }
  return true;
}

bool parse_string(const char *j, std::string &s) {
  simdjson::error_code error;
  std::string_view answer;
  parser.parse(j,strlen(j))
        .at(0)
        .get(answer, error);
  if (error) { return false; }
  s.assign(answer.data(), answer.size());
  return true;
}


int main() {
  double x{};
  parse_double("[1.1]",x);
  if(x != 1.1) {
    std::cerr << "bug in parse_double!" << std::endl;
    return EXIT_FAILURE;
  }
  std::string s{};
  parse_string("[\"my string\"]", s);
  if(s != "my string") {
    std::cerr << "bug in parse_string!" << std::endl;
    return EXIT_FAILURE;
  }
  basics_error_2();
  basics_error_3();
  basics_error_4();
  basics_error_5();
  return EXIT_SUCCESS;
}
