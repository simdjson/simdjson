#include <iostream>
#include <ranges>
#include <string_view>

#include "simdjson.h"
#include "test_macros.h"
using namespace simdjson;

int main(void) {
  auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
  dom::parser parser;
  auto justmodel = [](auto car) -> std::string_view {
    return car["model"].get_string();
  };
  auto cars =
      parser.parse(cars_json).get_array() | std::views::transform(justmodel);

  const std::vector<std::string_view> expected{"Camry", "Soul", "Tercel"};
  return std::ranges::equal(cars, expected) ? EXIT_SUCCESS : EXIT_FAILURE;
}
