#include "simdjson.h"
#include "test_builder.h"
#include <string>
#include <optional>

using namespace simdjson;

#if SIMDJSON_STATIC_REFLECTION

struct RenamedFields {
  [[= simdjson::rename<"first_name">]] std::string firstName = "";
  [[= simdjson::rename<"last_name">]]  std::string lastName = "";
  int age = 0;
};

struct SkippedField {
  std::string name = "";
  [[= simdjson::skip]] int internalCache = 0;
};

struct MixedAnnotations {
  [[= simdjson::rename<"user_name">]] std::string userName = "";
  [[= simdjson::skip]] int sessionToken = 0;
  int age = 0;
};

#endif // SIMDJSON_STATIC_REFLECTION

namespace annotation_tests {

bool rename_serialize_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  RenamedFields r{"Alice", "Smith", 30};
  std::string out;
  ASSERT_SUCCESS(simdjson::to_json(r).get(out));
  ASSERT_EQUAL(out, "{\"first_name\":\"Alice\",\"last_name\":\"Smith\",\"age\":30}");
#endif
  TEST_SUCCEED();
}

bool rename_deserialize_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  std::string json = R"({"first_name":"Bob","last_name":"Jones","age":25})";
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(simdjson::pad(json)).get(doc));
  RenamedFields r;
  ASSERT_SUCCESS(doc.get<RenamedFields>().get(r));
  ASSERT_EQUAL(r.firstName, "Bob");
  ASSERT_EQUAL(r.lastName, "Jones");
  ASSERT_EQUAL(r.age, 25);
#endif
  TEST_SUCCEED();
}

bool rename_roundtrip_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  RenamedFields original{"Carol", "White", 40};
  std::string json;
  ASSERT_SUCCESS(simdjson::to_json(original).get(json));
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(simdjson::pad(json)).get(doc));
  RenamedFields result;
  ASSERT_SUCCESS(doc.get<RenamedFields>().get(result));
  ASSERT_EQUAL(result.firstName, original.firstName);
  ASSERT_EQUAL(result.lastName, original.lastName);
  ASSERT_EQUAL(result.age, original.age);
#endif
  TEST_SUCCEED();
}

bool skip_serialize_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  SkippedField s{"Alice", 999};
  std::string out;
  ASSERT_SUCCESS(simdjson::to_json(s).get(out));
  ASSERT_EQUAL(out, "{\"name\":\"Alice\"}");
#endif
  TEST_SUCCEED();
}

bool skip_deserialize_ignores_field_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  // The key "internalCache" is present in JSON but the field is annotated skip -
  // it should be ignored and the field should keep its default value.
  std::string json = R"({"name":"Carol","internalCache":999})";
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(simdjson::pad(json)).get(doc));
  SkippedField s;
  ASSERT_SUCCESS(doc.get<SkippedField>().get(s));
  ASSERT_EQUAL(s.name, "Carol");
  ASSERT_EQUAL(s.internalCache, 0);
#endif
  TEST_SUCCEED();
}

bool mixed_annotations_serialize_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  MixedAnnotations m{"dave", 12345, 28};
  std::string out;
  ASSERT_SUCCESS(simdjson::to_json(m).get(out));
  // sessionToken must not appear; userName must appear as "user_name"
  ASSERT_EQUAL(out, "{\"user_name\":\"dave\",\"age\":28}");
#endif
  TEST_SUCCEED();
}

bool mixed_annotations_deserialize_test() {
  TEST_START();
#if SIMDJSON_STATIC_REFLECTION
  std::string json = R"({"user_name":"eve","age":35})";
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(simdjson::pad(json)).get(doc));
  MixedAnnotations m;
  ASSERT_SUCCESS(doc.get<MixedAnnotations>().get(m));
  ASSERT_EQUAL(m.userName, "eve");
  ASSERT_EQUAL(m.age, 35);
  ASSERT_EQUAL(m.sessionToken, 0);
#endif
  TEST_SUCCEED();
}

bool run_all() {
  return rename_serialize_test()
      && rename_deserialize_test()
      && rename_roundtrip_test()
      && skip_serialize_test()
      && skip_deserialize_ignores_field_test()
      && mixed_annotations_serialize_test()
      && mixed_annotations_deserialize_test();
}

} // namespace annotation_tests

int main() {
  return annotation_tests::run_all() ? EXIT_SUCCESS : EXIT_FAILURE;
}
