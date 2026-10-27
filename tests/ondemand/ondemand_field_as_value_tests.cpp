#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace field_as_value_tests {
  using namespace std;

  bool nested_lookup_on_field_result() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"({ "12345" : {"x":12.34, "y":56.78, "z": 9998877} })"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::object points;
    ASSERT_SUCCESS(doc.get_object().get(points));
    for (auto point : points) {
      std::string_view id;
      ASSERT_SUCCESS(point.unescaped_key().get(id));
      ASSERT_EQUAL(id, "12345");
      double x, y;
      int64_t z;
      ASSERT_SUCCESS(point["x"].get_double().get(x));
      ASSERT_SUCCESS(point["y"].get_double().get(y));
      ASSERT_SUCCESS(point["z"].get_int64().get(z));
      ASSERT_EQUAL(x, 12.34);
      ASSERT_EQUAL(y, 56.78);
      ASSERT_EQUAL(z, 9998877);
    }
    TEST_SUCCEED();
  }

  bool nested_lookup_on_extracted_field() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"({"p":{"x":1,"y":2}})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::object obj;
    ASSERT_SUCCESS(doc.get_object().get(obj));
    for (auto field_result : obj) {
      ondemand::field field;
      ASSERT_SUCCESS(std::move(field_result).get(field));
      int64_t x, y;
      ASSERT_SUCCESS(field["x"].get_int64().get(x));
      ASSERT_SUCCESS(field.find_field_unordered("y").get_int64().get(y));
      ASSERT_EQUAL(x, 1);
      ASSERT_EQUAL(y, 2);
    }
    TEST_SUCCEED();
  }

  bool find_field_is_order_sensitive() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"({"p":{"x":1,"y":2}})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::object obj;
    ASSERT_SUCCESS(doc.get_object().get(obj));
    for (auto field_result : obj) {
      ondemand::field field;
      ASSERT_SUCCESS(std::move(field_result).get(field));
      int64_t y;
      ASSERT_SUCCESS(field.find_field("y").get_int64().get(y));
      ASSERT_EQUAL(y, 2);
      ASSERT_ERROR(field.find_field("x"), NO_SUCH_FIELD);
    }
    TEST_SUCCEED();
  }

  bool missing_nested_key() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"({"p":{"x":1}})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::object obj;
    ASSERT_SUCCESS(doc.get_object().get(obj));
    for (auto point : obj) {
      ASSERT_ERROR(point["missing"], NO_SUCH_FIELD);
    }
    TEST_SUCCEED();
  }

  bool field_value_is_not_object() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"({"p":1})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::object obj;
    ASSERT_SUCCESS(doc.get_object().get(obj));
    for (auto point : obj) {
      ASSERT_ERROR(point["x"], INCORRECT_TYPE);
    }
    TEST_SUCCEED();
  }

  bool run() {
    return nested_lookup_on_field_result() &&
           nested_lookup_on_extracted_field() &&
           find_field_is_order_sensitive() &&
           missing_nested_key() &&
           field_value_is_not_object() &&
           true;
  }
} // namespace field_as_value_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, field_as_value_tests::run);
}
