#include "simdjson.h"
#include "test_ondemand.h"

#include <string>
#include <vector>

#ifdef __cpp_concepts

namespace simdjson {

// unique_ptr<T>
template <typename T>
auto tag_invoke(deserialize_tag, std::type_identity<std::unique_ptr<T>>, auto &val) {
  return simdjson_result{std::make_unique<T>(val.template get<T>())};
}

// vector<T>
template <typename T, typename AllocT = std::allocator<T>>
  requires (std::is_default_constructible_v<T>)
simdjson_result<std::vector<T, AllocT>> tag_invoke(deserialize_tag, std::type_identity<std::vector<T, AllocT>>, auto &value) {
  ondemand::array array;
  if (auto error = value.get_array().get(array)) {
    return error;
  }
  std::vector<T, AllocT> vec;
  for (auto v : array) {
    T val;
    if (auto error = v.template get<T>().get(val)) {
      return error;
    }
    vec.push_back(val);
  }
  return vec;
}

} // namespace simdjson

#endif // __cpp_concepts

namespace doc_custom_types_tests {
#if SIMDJSON_EXCEPTIONS && defined(__cpp_concepts)
struct Car {
  std::string make;
  std::string model;
  int64_t year = 0;
  std::vector<double> tire_pressure;

  friend simdjson_result<Car> tag_invoke(simdjson::deserialize_tag,
                                         std::type_identity<Car>, auto &val) {
    simdjson::ondemand::object obj;
    if (auto const error = val.get_object().get(obj)) {
      return error;
    }
    Car car{};
    // Instead of repeatedly obj["something"], we iterate through the object
    // which we expect to be faster.
    for (auto field : obj) {
      simdjson::ondemand::raw_json_string key;
      if (auto const error = field.key().get(key)) {
        return error;
      }
      if (key == "make") {
        if (auto const error = field.value().get_string(car.make)) {
          return error;
        }
      } else if (key == "model") {
        if (auto const error = field.value().get_string(car.model)) {
          return error;
        }
      } else if (key == "year") {
        if (auto const error = field.value().get_int64().get(car.year)) {
          return error;
        }
      } else if (key == "tire_pressure") {
        if (auto const error = field.value().get<std::vector<double>>().get(
                car.tire_pressure)) {
          return error;
        }
      }
    }
    return car;
  }
};

static_assert(simdjson::tag_invocable<simdjson::deserialize_tag,
                                      std::type_identity<std::unique_ptr<Car>>,
                                      simdjson::ondemand::value &>,
              "It should be invocable");

static_assert(simdjson::tag_invocable<simdjson::deserialize_tag,
                                      std::type_identity<std::unique_ptr<Car>>,
                                      simdjson::ondemand::document &>,
              "Tag_invoke should work with document as well.");

static_assert(simdjson::tag_invocable<simdjson::deserialize_tag,
                                      std::type_identity<std::vector<Car>>,
                                      simdjson::ondemand::value &>,
              "It should be invocable");

static_assert(simdjson::tag_invocable<simdjson::deserialize_tag,
                                      std::type_identity<std::vector<Car>>,
                                      simdjson::ondemand::document &>,
              "Tag_invoke should work with document as well.");

bool custom_test() {
  TEST_START();
  auto const json = R"( {
      "make": "Toyota",
      "model": "Camry",
      "year": 2018,
      "tire_pressure": [ 40.1, 39.9 ]
 } )"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);

  // creating car directly from "doc":
  const std::unique_ptr<Car> car(doc);
  if (car->make != "Toyota") {
    return false;
  }
  if (car->model != "Camry") {
    return false;
  }
  if (car->year != 2018) {
    return false;
  }
  if (car->tire_pressure.size() != 2) {
    return false;
  }
  return true;
}

bool custom_test_move_semantics() {
  TEST_START();
  auto const json = R"( {
      "make": "Toyota",
      "model": "Camry",
      "year": 2018,
      "tire_pressure": [ 40.1, 39.9 ]
 } )"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc_orig = parser.iterate(json);
  simdjson::ondemand::document&& doc = std::move(doc_orig);

  // this should use `document::get() &&` function
  const std::unique_ptr<Car> car(std::move(doc).get<std::unique_ptr<Car>>());
  if (car->make != "Toyota") {
    return false;
  }
  if (car->model != "Camry") {
    return false;
  }
  if (car->year != 2018) {
    return false;
  }
  if (car->tire_pressure.size() != 2) {
    return false;
  }
  return true;
}

#endif // SIMDJSON_EXCEPTIONS
bool run() {
  return
#if SIMDJSON_EXCEPTIONS && defined(__cpp_concepts)
      custom_test() &&
#endif // SIMDJSON_EXCEPTIONS
      true;
}

} // namespace doc_custom_types_tests

int main(const int argc, char *argv[]) {
  return test_main(argc, argv, doc_custom_types_tests::run);
}
