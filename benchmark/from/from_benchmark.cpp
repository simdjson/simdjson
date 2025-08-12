// We are going to assume C++20.

#include "event_counter.h"
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <simdjson.h>
#include <string>
#include <vector>

struct Car {
  std::string make;
  std::string model;
  int64_t year;
  std::vector<double> tire_pressure;
};

using namespace simdjson;
event_collector collector;

// We want to maximize C++ portability, but this is not needed with static
// reflection (C++26).
template <>
simdjson_inline simdjson_result<Car>
simdjson::ondemand::document::get() & noexcept {
  ondemand::object obj;
  auto error = get_object().get(obj);
  if (error) {
    return error;
  }
  Car car;
  if ((error = obj["make"].get_string(car.make))) {
    return error;
  }
  if ((error = obj["model"].get_string(car.model))) {
    return error;
  }
  if ((error = obj["year"].get_int64().get(car.year))) {
    return error;
  }
  if ((error = obj["tire_pressure"].get<std::vector<double>>().get(
           car.tire_pressure))) {
    return error;
  }
  return car;
}

template <>
simdjson_inline simdjson_result<Car> simdjson::ondemand::value::get() noexcept {
  ondemand::object obj;
  auto error = get_object().get(obj);
  if (error) {
    return error;
  }
  Car car;
  if ((error = obj["make"].get_string(car.make))) {
    return error;
  }
  if ((error = obj["model"].get_string(car.model))) {
    return error;
  }
  if ((error = obj["year"].get_int64().get(car.year))) {
    return error;
  }
  if ((error = obj["tire_pressure"].get<std::vector<double>>().get(
           car.tire_pressure))) {
    return error;
  }
  return car;
}

template <class function_type>
std::pair<event_aggregate, size_t>
bench(const function_type &&function, size_t min_repeat = 10,
      size_t min_time_ns = 40'000'000, size_t max_repeat = 10000000) {
  size_t N = min_repeat;
  if (N == 0) {
    N = 1;
  }
  event_aggregate warm_aggregate{};
  for (size_t i = 0; i < N; i++) {
    std::atomic_thread_fence(std::memory_order_acquire);
    collector.start();
    function();
    std::atomic_thread_fence(std::memory_order_release);
    event_count allocate_count = collector.end();
    warm_aggregate << allocate_count;
    if ((i + 1 == N) && (warm_aggregate.total_elapsed_ns() < min_time_ns) &&
        (N < max_repeat)) {
      N *= 10;
    }
  }
  event_aggregate aggregate{};
  for (size_t i = 0; i < 10; i++) {
    std::atomic_thread_fence(std::memory_order_acquire);
    collector.start();
    for (size_t i = 0; i < N; i++) {
      function();
    }
    std::atomic_thread_fence(std::memory_order_release);
    event_count allocate_count = collector.end();
    aggregate << allocate_count;
  }
  return {aggregate, N};
}

double pretty_print(const std::string &name, size_t num_values,
                    std::pair<event_aggregate, size_t> result) {
  const auto &agg = result.first;
  size_t N = result.second;
  num_values *= N;
  printf("%-50s : %8.2f ns %8.2f GB/s", name.c_str(),
         agg.elapsed_ns() / num_values, num_values / agg.elapsed_ns());
  if (collector.has_events()) {
    printf(" %8.2f GHz %8.2f c %8.2f i %8.2f i/c",
           agg.cycles() / agg.elapsed_ns(), agg.cycles() / num_values,
           agg.instructions() / num_values, agg.instructions() / agg.cycles());
  }
  printf("\n");
  return num_values / agg.elapsed_ns();
}

void run_benchmarks() {
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

int main() {
  for (size_t trial = 0; trial < 3; trial++) {
    printf("Trial %zu:\n", trial + 1);
    run_benchmarks();
    printf("\n");
  }
  return EXIT_SUCCESS;
}
