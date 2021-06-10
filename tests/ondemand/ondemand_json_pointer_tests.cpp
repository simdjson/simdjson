#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace json_pointer_tests {
    bool easytest() {
        TEST_START();
        auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;

        ondemand::parser parser;
        ondemand::document cars;
        double x;
        ASSERT_SUCCESS(parser.iterate(cars_json).get(cars));
        ASSERT_SUCCESS(cars.at_pointer("/0/tire_pressure/1").get(x));
        ASSERT_EQUAL(x,39.9);
        TEST_SUCCEED();
    }

    bool relative_path() {
        TEST_START();
        auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;

        ondemand::parser parser;
        ondemand::document cars;
        std::vector<double> measured;
        ASSERT_SUCCESS(parser.iterate(cars_json).get(cars));
        for (auto car_element : cars) {
            double x;
            ASSERT_SUCCESS(car_element.at_pointer("/tire_pressure/1").get(x));
            measured.push_back(x);
        }

        std::vector<double> expected = {39.9, 31, 30};
        if (measured != expected) { return false; }
        TEST_SUCCEED();
    }

    bool run() {
        return
                easytest() &&
                relative_path() &&
                true;
    }
}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, json_pointer_tests::run);
}