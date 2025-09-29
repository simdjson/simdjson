#include "simdjson.h"
#include "test_builder.h"
#include <string>
#include <string_view>
#include <vector>

using namespace simdjson;
struct Car {
  std::string make;
  std::string model;
  int64_t year;
  std::vector<double> tire_pressure;
};

struct kid {
  int age;
  std::string name;
  std::vector<std::string> toys;
  bool operator<=> (const kid&) const = default;
};

struct Z {
  int x;
  bool operator<=> (const Z&) const = default;
};

struct Y {
  int g;
  std::string h;
  std::vector<int> i;
  Z z;
  bool operator<=> (const Y&) const = default;
};

struct X {
  char a;
  int b;
  int c;
  std::string d;
  std::vector<int> e;
  std::vector<std::string> f;
  Y y;
  bool operator<=> (const X&) const = default;
};

namespace builder_tests {


  bool car_test() {
    TEST_START();
#if SIMDJSON_STATIC_REFLECTION
    Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
    std::string pstr;
    ASSERT_SUCCESS(builder::to_json_string(c).get(pstr));
    ASSERT_EQUAL(pstr, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(simdjson::pad(pstr)).get(doc));
    Car c2;
    ASSERT_SUCCESS(doc.get<Car>().get(c2));
    ASSERT_EQUAL(c2.make, "Toyota");
    ASSERT_EQUAL(c2.model, "Corolla");
    ASSERT_EQUAL(c2.year, 2017);
    ASSERT_EQUAL(c2.tire_pressure.size(), 4);
    ASSERT_EQUAL(c2.tire_pressure[0], 30.0);
    ASSERT_EQUAL(c2.tire_pressure[1], 30.2);
    ASSERT_EQUAL(c2.tire_pressure[2], 30.513);
    ASSERT_EQUAL(c2.tire_pressure[3], 30.79);
#endif
    TEST_SUCCEED();
  }
  #if SIMDJSON_EXCEPTIONS
  bool car_test_exception() {
      TEST_START();
      simdjson::builder::string_builder sb;
      Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
      append(sb, c);
      std::string_view p{sb};
      (void)p; // to avoid unused variable warning
      TEST_SUCCEED();
  }
  bool car_test_exception2() {
      TEST_START();
      simdjson::builder::string_builder sb;
      Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
      sb << c;
      std::string_view p{sb};
      (void)p; // to avoid unused variable warning
      TEST_SUCCEED();
  }
  bool car_test_to_json_exception() {
    TEST_START();
    Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
    std::string json = simdjson::to_json(c);
    TEST_SUCCEED();
  }
  bool car_test_to_json_exception_value() {
    TEST_START();
    std::string json = simdjson::to_json(Car{"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}});
    TEST_SUCCEED();
  }
  #endif // SIMDJSON_EXCEPTIONS


bool serialize_deserialize_kid() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  simdjson::padded_string json_str =
      R"({"age": 12, "name": "John", "toys": ["car", "ball"]})"_padded;
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json_str).get(doc));
  kid k;
  ASSERT_SUCCESS(doc.get<kid>().get(k));
  ASSERT_EQUAL(k.age, 12);
  ASSERT_EQUAL(k.name, "John");
  ASSERT_EQUAL(k.toys.size(), 2);
  ASSERT_EQUAL(k.toys[0], "car");
  ASSERT_EQUAL(k.toys[1], "ball");
  // Now, go the other direction:
  std::string json;
  ASSERT_SUCCESS(builder::to_json_string(k).get(json));
  std::cout << json << std::endl;
  // Now we parse it back:
  simdjson::ondemand::parser parser2;
  simdjson::ondemand::document doc2;
  ASSERT_SUCCESS(parser2.iterate(simdjson::pad(json)).get(doc2));
  kid k2;
  ASSERT_SUCCESS(doc2.get<kid>().get(k2));
  ASSERT_EQUAL(k2.age, 12);
  ASSERT_EQUAL(k2.name, "John");
  ASSERT_EQUAL(k2.toys.size(), 2);
  ASSERT_EQUAL(k2.toys[0], "car");
  ASSERT_EQUAL(k2.toys[1], "ball");
#endif
  TEST_SUCCEED();
}

bool serialize_deserialize_x_y_z() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  X s1 = {.a = '1',
          .b = 10,
          .c = 0,
          .d = "test string\n\r\"",
          .e = {1, 2, 3},
          .f = {"ab", "cd", "fg"},
          .y = {.g = 100,
                .h = "test string\n\r\"",
                .i = {1, 2, 3},
                .z = {.x = 1000}}};
  std::string pstr;
  ASSERT_SUCCESS(builder::to_json_string(s1).get(pstr));
  ASSERT_EQUAL(
      pstr,
      R"({"a":"1","b":10,"c":0,"d":"test string\n\r\"","e":[1,2,3],"f":["ab","cd","fg"],"y":{"g":100,"h":"test string\n\r\"","i":[1,2,3],"z":{"x":1000}}})");
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(simdjson::pad(pstr)).get(doc));
  X s2;
  ASSERT_SUCCESS(doc.get<X>().get(s2));
  ASSERT_TRUE(s1 == s2);
#endif
  TEST_SUCCEED();
}

  bool run() {
    return

  #if SIMDJSON_EXCEPTIONS
          car_test_exception() &&
          car_test_exception2() &&
          car_test_to_json_exception() &&
          car_test_to_json_exception_value() &&
  #endif // SIMDJSON_EXCEPTIONS
          car_test() &&
          serialize_deserialize_kid() &&
          serialize_deserialize_x_y_z() &&
          true;
  }

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, builder_tests::run);
}