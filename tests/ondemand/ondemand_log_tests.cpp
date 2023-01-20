#define SIMDJSON_VERBOSE_LOGGING 1
#include "simdjson.h"

#include <iostream>
#include <vector>
#include <string>

using namespace simdjson;


int main() {
#if SIMDJSON_EXCEPTIONS
  auto cars_json = R"( [
		{ "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
		{ "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
		{ "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
	] )"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(cars_json);
  std::vector<std::string> container;
  for(ondemand::object o : doc) {
    container.emplace_back(std::string_view(o["make"]));
  }
  std::cout << "---" << std::endl;
  for(std::string& m : container) {
    std::cout << m << std::endl;
  }
#endif // #if SIMDJSON_EXCEPTIONS
  return EXIT_SUCCESS;
}