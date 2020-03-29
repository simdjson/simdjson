#include <iostream>
#include "simdjson.h"

using namespace std;
using namespace simdjson;
using error_code=simdjson::error_code;

void basics_1() {
  const char *filename = "x.txt";

  dom::parser parser;
  dom::element doc = parser.load(filename); // load and parse a file

  cout << doc;
}

void basics_2() {
  dom::parser parser;
  dom::element doc = parser.parse("[1,2,3]"_padded); // parse a string

  cout << doc;
}

void basics_dom_1() {
  auto cars_json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;
  dom::parser parser;
  dom::array cars = parser.parse(cars_json).get<dom::array>();

  // Iterating through an array of objects
  for (dom::object car : cars) {
    // Accessing a field by name
    cout << "Make/Model: " << car["make"] << "/" << car["model"] << endl;

    // Casting a JSON element to an integer
    uint64_t year = car["year"];
    cout << "- This car is " << 2020 - year << "years old." << endl;

    // Iterating through an array of floats
    double total_tire_pressure = 0;
    for (double tire_pressure : car["tire_pressure"]) {
      total_tire_pressure += tire_pressure;
    }
    cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;

    // Writing out all the information about the car
    for (auto [key, value] : car) {
      cout << "- " << key << ": " << value << endl;
    }
  }
}

void basics_dom_2() {
  auto cars_json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;
  dom::parser parser;
  dom::element cars = parser.parse(cars_json);
  cout << cars.at("0/tire_pressure/1") << endl; // Prints 39.9}
}

void basics_ndjson() {
  dom::parser parser;
  for (dom::element doc : parser.load_many("x.txt")) {
    cout << doc["foo"] << endl;
  }
  // Prints 1 2 3
}

void implementation_selection_1() {
  cout << "simdjson v" << STRINGIFY(SIMDJSON_VERSION) << endl;
  cout << "Detected the best implementation for your machine: " << simdjson::active_implementation->name();
  cout << "(" << simdjson::active_implementation->description() << ")" << endl;
}

void implementation_selection_2() {
  for (auto implementation : simdjson::available_implementations) {
    cout << implementation->name() << ": " << implementation->description() << endl;
  }
}

void implementation_selection_3() {
  cout << simdjson::available_implementations["fallback"]->description() << endl;
}

void implementation_selection_4() {
  // Use the fallback implementation, even though my machine is fast enough for anything
  simdjson::active_implementation = simdjson::available_implementations["fallback"];
}

int main() {
  return 0;
}
