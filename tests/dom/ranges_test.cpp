#include <iostream>
#include <ranges>
#include <string_view>

#include "simdjson.h"
#include "test_macros.h"
#include "test_main.h"

using namespace simdjson;
namespace ranges_test {
#if SIMDJSON_EXCEPTIONS
  bool printout() {
    TEST_START();
    auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
      ] )"_padded;
    dom::parser parser;
    auto justmodel = [](auto car) { return car["model"]; };
    for (auto car :
        parser.parse(cars_json).get_array() | std::views::transform(justmodel)) {
      std::cout << car << std::endl;
    }
    TEST_SUCCEED();
  }

  bool checkvalues() {
    TEST_START();
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
    ASSERT_TRUE(std::ranges::equal(cars, expected));
    TEST_SUCCEED();
  }
  #endif // SIMDJSON_EXCEPTIONS

  bool run() {
    return
  #if SIMDJSON_EXCEPTIONS
        printout() && checkvalues() &&
  #endif // SIMDJSON_EXCEPTIONS
        true;
  }

} // namespace ranges_test

int main(int argc, char *argv[]) {
  return test_main(argc, argv, ranges_test::run);
}
