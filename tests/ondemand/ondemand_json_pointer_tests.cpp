#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace json_pointer_tests {
    bool easytest() {
        TEST_START();
        ondemand::parser parser;
        auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": { "model": [5,6,7], "year": 2018 } },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;
        auto cars = parser.iterate(cars_json);
        auto x = cars.at_pointer("/1/tire_pressure");
        //if (x != 39.9) { return false; }
        std::cout << x << std::endl;
        TEST_SUCCEED();
    }

    bool run() {
        return
                easytest() &&
                true;
    }
}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, json_pointer_tests::run);
}