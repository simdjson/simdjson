#include "event_counter.h"
#include <random>
#include <vector>

#include <simdjson.h>

event_collector collector;

struct Car {
  std::string make;
  std::string model;
  int64_t year; // We deliberately do not include the tire pressure.
};

std::vector<Car> generate_random_cars(size_t count) {
  static const std::vector<std::string> makes = {"Toyota", "Honda", "Ford",
                                                 "BMW", "Mazda"};
  static const std::vector<std::string> models = {"Camry", "Civic", "Focus",
                                                  "320i", "3"};
  static thread_local std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<int> make_dist(0, makes.size() - 1);
  std::uniform_int_distribution<int> model_dist(0, models.size() - 1);
  std::uniform_int_distribution<int64_t> year_dist(2000, 2025);
  std::uniform_real_distribution<double> pressure_dist(30.0, 45.0);

  std::vector<Car> cars;
  cars.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    Car car;
    car.make = makes[make_dist(rng)];
    car.model = models[model_dist(rng)];
    car.year = year_dist(rng);
    cars.push_back(std::move(car));
  }
  return cars;
}

std::string_view serialize(simdjson::builder::string_builder &sb,
                           const std::vector<Car> &cars) {
  sb.clear();
  sb.start_array();
  for (const auto &car : cars) {
    sb.start_object();
    sb.append_key_value("make", car.make);
    sb.append_comma();
    sb.append_key_value("model", car.model);
    sb.append_comma();
    sb.append_key_value("year", car.year);
    sb.end_object();
  }
  sb.end_array();
  std::string_view result;
  if (sb.view().get(result)) {
    return ""; // unexpected (error)
  }
  return result;
}

double pretty_print(const std::string &name, size_t num_chars,
                    std::pair<event_aggregate, size_t> result) {
  const auto &agg = result.first;
  size_t N = result.second;
  num_chars *= N;
  printf("%-40s : %8.2f ns %8.2f GB/s", name.c_str(),
         agg.elapsed_ns() / num_chars, num_chars / agg.elapsed_ns());
  if (collector.has_events()) {
    printf(" %8.2f GHz %8.2f cycles/char %8.2f ins./char %8.2f i/c",
           agg.cycles() / agg.elapsed_ns(), agg.cycles() / num_chars,
           agg.instructions() / num_chars, agg.instructions() / agg.cycles());
  }
  printf("\n");
  return num_chars / agg.elapsed_ns();
}

template <class function_type>
std::pair<event_aggregate, size_t>
bench(const function_type &&function, size_t min_repeat = 100,
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

void run_benchmarks() {
  std::vector<Car> source = generate_random_cars(100000);
  simdjson::builder::string_builder sb;
  size_t volume = serialize(sb, source).size();

  pretty_print("string_builder", volume, bench([&source, &sb]() -> size_t {
                 return serialize(sb, source).size();
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
