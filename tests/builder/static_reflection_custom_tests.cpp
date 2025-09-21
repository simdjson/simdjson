


#include "simdjson.h"
#include "test_builder.h"
#include <charconv>
#include <string>

using namespace simdjson;

// Suppose that we want to serialize/deserialize Car using
// strings for the year
struct Car {
  std::string make;
  std::string model;
  int64_t year;
  std::vector<float> tire_pressure;
};



namespace simdjson {
// This tag_invoke MUST be inside simdjson namespace
template <typename simdjson_value>
auto tag_invoke(deserialize_tag, simdjson_value &val, Car& car) {
  ondemand::object obj;
  auto error = val.get_object().get(obj);
  if (error) {
    return error;
  }
  if ((error = obj["make"].get_string(car.make))) {
    return error;
  }
  if ((error = obj["model"].get_string(car.model))) {
    return error;
  }
  std::string_view year_str;
  if ((error = obj["year"].get(year_str))) {
    return error;
  }
  int64_t year_value;
  auto [ptr, ec] = std::from_chars(year_str.data(), year_str.data() + year_str.size(), year_value);
  if (ec != std::errc{}) {
    return INCORRECT_TYPE;
  }
  car.year = year_value;
  if ((error = obj["tire_pressure"].get<std::vector<float>>().get(
           car.tire_pressure))) {
    return error;
  }
  return simdjson::SUCCESS;
}

template <typename builder_type>
void tag_invoke(serialize_tag, builder_type &builder, const Car& car) {
  builder.start_object();
  builder.append_key_value("make", car.make);
  builder.append_comma();
  builder.append_key_value("model", car.model);
  builder.append_comma();
  builder.append_key_value("year", std::to_string(car.year));
  builder.append_comma();
  builder.append_key_value("tire_pressure", car.tire_pressure);
  builder.end_object();
}

} // namespace simdjson
namespace custom_tests {

  bool test_car_deserialization() {
    TEST_START();
    auto json = R"({"make":"Toyota","model":"Camry","year":"2018","tire_pressure":[30.5,30.6,30.7,30.8]})"_padded;
    ondemand::parser parser;
    auto doc_result = parser.iterate(json);
    ASSERT_SUCCESS(doc_result);
    Car car;
    ASSERT_SUCCESS(doc_result.get(car));
    if (car.make != "Toyota" || car.model != "Camry" || std::to_string(car.year) != "2018" || car.tire_pressure.size() != 4 || car.tire_pressure[0] != 30.5f) {
      return false;
    }
    TEST_SUCCEED();
  }
  bool test_car_serialization() {
    TEST_START();
    Car car{"Toyota", "Camry", 2018, {30.5f,30.6f,30.7f,30.8f}};
    simdjson::builder::string_builder builder;
    builder.append(car);
    std::string_view json;
    ASSERT_SUCCESS(builder.view().get(json));
    TEST_SUCCEED();
  }
  bool test_car_roundtrip() {
    TEST_START();
    Car car{"Toyota", "Camry", 2018, {30.5f,30.6f,30.7f,30.8f}};
    simdjson::builder::string_builder builder;
    builder.append(car);
    std::string json;
    ASSERT_SUCCESS(builder.view().get(json));
    ondemand::parser parser;
    auto doc_result = parser.iterate(json);
    ASSERT_SUCCESS(doc_result);
    Car carback;
    ASSERT_SUCCESS(doc_result.get(carback));

    if (carback.make != car.make || carback.model != car.model || carback.year != car.year || carback.tire_pressure != car.tire_pressure) {
      return false;
    }

    TEST_SUCCEED();
  }

  bool test_car_roundtrip_to_json() {
    TEST_START();
    Car car{"Toyota", "Camry", 2018, {30.5f,30.6f,30.7f,30.8f}};
    std::string json;
    ASSERT_SUCCESS(simdjson::to_json(car).get(json));
    simdjson::builder::string_builder builder;
    ondemand::parser parser;
    auto doc_result = parser.iterate(json);
    ASSERT_SUCCESS(doc_result);
    Car carback;
    ASSERT_SUCCESS(doc_result.get(carback));

    if (carback.make != car.make || carback.model != car.model || carback.year != car.year || carback.tire_pressure != car.tire_pressure) {
      return false;
    }

    TEST_SUCCEED();
  }
  bool run() {
    return test_car_deserialization() && test_car_serialization() && test_car_roundtrip() && test_car_roundtrip_to_json();
  }

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, custom_tests::run);
}