#include "simdjson.h"
#include "test_builder.h"
#include <string>
#include <string_view>
#include <vector>

using namespace simdjson;

struct kid {
  int age;
  std::string name;
  std::vector<std::string> toys;
};

// This instruct simdjson to instantiate the deserialization routine
// for the type kid. This applies only to struct and class types.
template<>
constexpr bool simdjson::user_defined_type<kid> = true;

struct Z {
  int x;
};

struct Y {
  int g;
  std::string h;
  std::vector<int> i;
  Z z;
};

struct X {
  char a;
  int b;
  int c;
  std::string d;
  std::vector<int> e;
  std::vector<std::string> f;
  Y y;
};

namespace builder_tests {

bool serialize_deserialize_kid() {
  TEST_START();
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
  ASSERT_SUCCESS(simdjson::builder::to_json_string(k).get(json));
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
  TEST_SUCCEED();
}

bool serialize_deserialize_x_y_z() {
  TEST_START();
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
  std::string json_str;
  simdjson::builder::string_builder sb;
  fast_to_json_string(sb, s1);
  std::string_view p;
  ASSERT_SUCCESS(sb.view().get(p));
  std::string pstr(p.data(), p.size());
  std::cout << p << std::endl;
  ASSERT_EQUAL(
      pstr,
      R"({"a":1,"b":10,"c":0,"d":"test string\n\r\"","e":[1,2,3],"f":["ab","cd","fg"],"y":{"g":100,"h":"test string\n\r\"","i":[1,2,3],"z":{"x":1000}}})");
  TEST_SUCCEED();
}

bool run() {
  return serialize_deserialize_kid() && serialize_deserialize_x_y_z() && true;
}

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, builder_tests::run);
}