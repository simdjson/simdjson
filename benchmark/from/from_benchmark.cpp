// We are going to assume C++20.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <simdjson.h>
#include <string>

#include "benchmark_helpers.h"
#include "car_helpers.h"

using namespace simdjson;

void run_benchmarks() {
  printf("Running benchmarks on tiny input...\n");
  simdjson::padded_string json =
      R"({ "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9 ] })"_padded;
  volatile size_t dummy = 0;
  ondemand::parser parser;

  // Classic On-Demand
  pretty_print("simdjson classic", json.size(),
               bench([&json, &dummy, &parser]() {
                 ondemand::document doc = parser.iterate(json);
                 Car car = doc.get<Car>();
                 dummy = dummy + car.year;
               }));

  // Benchmark simdjson::from() without parser
  pretty_print("simdjson::from<Car>() (no parser)", json.size(),
               bench([&json, &dummy]() {
                 Car car = simdjson::from(json);
                 dummy = dummy + car.year;
               }));

  // Benchmark simdjson::from() with parser
  pretty_print("simdjson::from<Car>() (with parser)", json.size(),
               bench([&json, &dummy, &parser]() {
                 Car car = simdjson::from(parser, json);
                 dummy = dummy + car.year;
               }));
}

void run_array_benchmarks() {
  printf("Running benchmarks on large array...\n");
  size_t N = 1'000'000;
  simdjson::padded_string json = generate_car_json_array(N);
  volatile size_t dummy = 0;
  ondemand::parser parser;

  // Classic On-Demand
  pretty_print_array("simdjson classic", json.size(), N,
                     bench([&json, &dummy, &parser]() {
                       dummy = 0;
                       ondemand::document doc = parser.iterate(json);
                       ondemand::array array = doc.get_array();
                       for (auto value : array) {
                         Car car = value.get<Car>();
                         dummy = dummy + car.year;
                       }
                     }));

  // from with array
  pretty_print_array("simdjson::from(json).array()", json.size(), N,
                     bench([&json, &dummy, &parser]() {
                       dummy = 0;

                       for (auto value : simdjson::from(json).array()) {
                         Car car = value.get<Car>();
                         dummy = dummy + car.year;
                       }
                     }));
#if SIMDJSON_SUPPORTS_RANGES_FROM
  // Benchmark simdjson::from() without parser (EXPERIMENTAL)
  pretty_print_array("simdjson::from<Car>() (experimental, no parser)",
                     json.size(), N, bench([&json, &dummy]() {
                       dummy = 0;
                       for (Car car :
                            simdjson::from(json) | simdjson::as<Car>()) {
                         dummy = dummy + car.year;
                       }
                     }));

  // Benchmark simdjson::from() with parser (EXPERIMENTAL)
  pretty_print_array("simdjson::from<Car>() (experimental, with parser)",
                     json.size(), N, bench([&json, &dummy, &parser]() {
                       dummy = 0;
                       for (Car car : simdjson::from(parser, json) |
                                          simdjson::as<Car>()) {
                         dummy = dummy + car.year;
                       }
                     }));
#endif // SIMDJSON_SUPPORTS_RANGES_FROM
}

void run_stream_benchmarks() {
  printf("Running stream benchmarks...\n");
  simdjson::padded_string json = generate_car_json_stream(1'000'000);
  volatile size_t dummy = 0;
  ondemand::parser parser;

  // Classic On-Demand
  pretty_print("simdjson classic", json.size(),
               bench([&json, &dummy, &parser]() {
                 ondemand::document_stream stream = parser.iterate_many(json);
                 for (auto doc : stream) {
                   Car car = doc.get<Car>();
                   dummy = dummy + car.year;
                 }
               }));
  // from_many
  pretty_print("simdjson with thread local", json.size(),
               bench([&json, &dummy, &parser]() {
                 ondemand::document_stream stream =
                     ondemand::parser::get_parser().iterate_many(json);
                 for (auto doc : stream) {
                   Car car = doc.get<Car>();
                   dummy = dummy + car.year;
                 }
               }));
}

int main() {
  for (size_t trial = 0; trial < 3; trial++) {
    printf("Trial %zu:\n", trial + 1);
    run_array_benchmarks();
    run_stream_benchmarks();
    run_benchmarks();
    printf("\n");
  }
  return EXIT_SUCCESS;
}
