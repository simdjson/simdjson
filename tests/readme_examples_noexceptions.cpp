#include <iostream>
#include "simdjson.h"

using namespace std;
using namespace simdjson;

void basics_error_1() {
  dom::parser parser;
  auto json = "1"_padded;

  auto [doc, error] = parser.parse(json); // doc is a dom::element
  if (error) { cerr << error << endl; exit(1); }
  // Use document here now that we've checked for the error
}

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
  auto [cars, error] = parser.parse(cars_json).get<dom::array>();
  if (error) { cerr << error << endl; exit(1); }

  // Iterating through an array of objects
  for (dom::element car_element : cars) {
    dom::object car;
    car_element.get<dom::object>().tie(car, error);
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
    car["year"].get<uint64_t>().tie(year, error);
    if (error) { cerr << error << endl; exit(1); }
    cout << "- This car is " << 2020 - year << "years old." << endl;

    // Iterating through an array of floats
    double total_tire_pressure = 0;
    dom::array tire_pressure_array;
    car["tire_pressure"].get<dom::array>().tie(tire_pressure_array, error);
    if (error) { cerr << error << endl; exit(1); }
    for (dom::element tire_pressure_element : tire_pressure_array) {
      double tire_pressure;
      tire_pressure_element.get<double>().tie(tire_pressure, error);
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

int main() {
  return 0;
}
