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

void performance_1() {
  dom::parser parser;

  // This initializes buffers and a document big enough to handle this JSON.
  dom::element doc = parser.parse("[ true, false ]"_padded);
  cout << doc << endl;

  // This reuses the existing buffers, and reuses and *overwrites* the old document
  doc = parser.parse("[1, 2, 3]"_padded);
  cout << doc << endl;

  // This also reuses the existing buffers, and reuses and *overwrites* the old document
  dom::element doc2 = parser.parse("true"_padded);
  // Even if you keep the old reference around, doc and doc2 refer to the same document.
  cout << doc << endl;
  cout << doc2 << endl;
}

// The web_request part of this is aspirational, so we compile as much as we can here
void performance_2() {
  dom::parser parser(1024*1024); // Never grow past documents > 1MB
//   for (web_request request : listen()) {
    auto [doc, error] = parser.parse("1"_padded/*request.body*/);
//     // If the document was above our limit, emit 413 = payload too large
    if (error == CAPACITY) { /* request.respond(413); continue; */ }
//     // ...
//   }
}

// The web_request part of this is aspirational, so we compile as much as we can here
void performance_3() {
  dom::parser parser(0); // This parser will refuse to automatically grow capacity
  simdjson::error_code allocate_error = parser.allocate(1024*1024); // This allocates enough capacity to handle documents <= 1MB
  if (allocate_error) { cerr << allocate_error << endl; exit(1); }

  // for (web_request request : listen()) {
    auto [doc, error] = parser.parse("1"_padded/*request.body*/);
    // If the document was above our limit, emit 413 = payload too large
    if (error == CAPACITY) { /* request.respond(413); continue; */ }
    // ...
  // }
}

int main() {
  return 0;
}
