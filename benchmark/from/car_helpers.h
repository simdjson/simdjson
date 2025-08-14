#ifndef CAR_HELPERS_H
#define CAR_HELPERS_H

#include <iomanip>
#include <random>
#include <sstream>
#include <vector>

#include <simdjson.h>

struct Car {
  std::string make;
  std::string model;
  int64_t year; // We deliberately do not include the tire pressure.
};

using namespace simdjson;

std::string random_car_json() {
  static const std::vector<std::string> makes = {"Toyota", "Honda", "Ford",
                                                 "BMW", "Mazda"};
  static const std::vector<std::string> models = {"Camry", "Civic", "Focus",
                                                  "320i", "3"};
  static thread_local std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<int> make_dist(0, makes.size() - 1);
  std::uniform_int_distribution<int> model_dist(0, models.size() - 1);
  std::uniform_int_distribution<int> year_dist(2000, 2025);
  std::uniform_real_distribution<double> pressure_dist(30.0, 45.0);

  std::ostringstream oss;
  oss << R"({ "make": ")" << makes[make_dist(rng)] << R"(", "model": ")"
      << models[model_dist(rng)] << R"(",  "year": )" << year_dist(rng)
      << R"(, "tire_pressure": [ )" << std::fixed << std::setprecision(1)
      << pressure_dist(rng) << ", " << pressure_dist(rng) << R"( ] })";
  return oss.str();
}

std::string generate_car_json_stream(size_t N) {
  std::ostringstream oss;
  for (size_t i = 0; i < N; ++i) {
    oss << random_car_json();
    oss << "\n";
  }
  return oss.str();
}

std::string generate_car_json_array(size_t N) {
  std::ostringstream oss;
  oss << "[\n";
  for (size_t i = 0; i < N; ++i) {
    oss << random_car_json();
    if (i < N - 1) {
      oss << ",";
    }
    oss << "\n";
  }
  oss << "]";
  return oss.str();
}

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
  return car;
}

template <>
simdjson_inline simdjson_result<Car>
simdjson::ondemand::document_reference::get() & noexcept {
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
  return car;
}

template <>
simdjson_inline simdjson_result<Car>
simdjson::ondemand::document_reference::get() && noexcept {
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
  return car;
}

#endif