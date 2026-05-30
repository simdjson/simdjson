#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace object_tests {
  using namespace std;
  using simdjson::ondemand::json_type;

  bool object_find_field_unordered() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c/d": 3})"_padded;
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );

      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field_unordered("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field_unordered("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( object.find_field_unordered("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::object>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::object> object;
      object = doc_result.get_object();

      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field_unordered("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field_unordered("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( object.find_field_unordered("d"), NO_SUCH_FIELD );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool document_object_find_field_unordered() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c/d": 3})"_padded;
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::document doc;
      ASSERT_SUCCESS( std::move(doc_result).get(doc) );
      ASSERT_EQUAL( doc.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( doc.find_field_unordered("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( doc.find_field_unordered("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( doc.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( doc.find_field_unordered("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_EQUAL( doc_result.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( doc_result.find_field_unordered("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( doc_result.find_field_unordered("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( doc_result.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( doc_result.find_field_unordered("d"), NO_SUCH_FIELD );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool value_object_find_field_unordered() {
    TEST_START();
    auto json = R"({ "outer": { "a": 1, "b": 2, "c/d": 3 } })"_padded;
    SUBTEST("ondemand::value", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::value object;
      ASSERT_SUCCESS( doc_result.find_field_unordered("outer").get(object) );
      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field_unordered("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field_unordered("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( object.find_field_unordered("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::value>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::value> object = doc_result.find_field_unordered("outer");
      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field_unordered("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field_unordered("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_EQUAL( object.find_field_unordered("a").get_uint64().value_unsafe(), 1 );
      ASSERT_ERROR( object.find_field_unordered("d"), NO_SUCH_FIELD );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool object_find_field() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c/d": 3})"_padded;
    SUBTEST("ondemand::object", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );

      ASSERT_EQUAL( object.find_field("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_ERROR( object.find_field("a"), NO_SUCH_FIELD );
      ASSERT_ERROR( object.find_field("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::object>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::object> object;
      object = doc_result.get_object();

      ASSERT_EQUAL( object.find_field("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_ERROR( object.find_field("a"), NO_SUCH_FIELD );
      ASSERT_ERROR( object.find_field("d"), NO_SUCH_FIELD );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool document_object_find_field() {
    TEST_START();
    auto json = R"({ "a": 1, "b": 2, "c/d": 3})"_padded;
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::document doc;
      ASSERT_SUCCESS( std::move(doc_result).get(doc) );
      ASSERT_EQUAL( doc.find_field("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( doc.find_field("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( doc.find_field("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_ERROR( doc.find_field("a"), NO_SUCH_FIELD );
      ASSERT_ERROR( doc.find_field("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      ASSERT_EQUAL( doc_result.find_field("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( doc_result.find_field("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( doc_result.find_field("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_ERROR( doc_result.find_field("a"), NO_SUCH_FIELD );
      ASSERT_ERROR( doc_result.find_field("d"), NO_SUCH_FIELD );
      return true;
    }));
    TEST_SUCCEED();
  }

  bool value_object_find_field() {
    TEST_START();
    auto json = R"({ "outer": { "a": 1, "b": 2, "c/d": 3 } })"_padded;
    SUBTEST("ondemand::value", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::value object;
      ASSERT_SUCCESS( doc_result.find_field("outer").get(object) );
      ASSERT_EQUAL( object.find_field("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_ERROR( object.find_field("a"), NO_SUCH_FIELD );
      ASSERT_ERROR( object.find_field("d"), NO_SUCH_FIELD );
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::value>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::value> object = doc_result.find_field("outer");
      ASSERT_EQUAL( object.find_field("a").get_uint64().value_unsafe(), 1 );
      ASSERT_EQUAL( object.find_field("b").get_uint64().value_unsafe(), 2 );
      ASSERT_EQUAL( object.find_field("c/d").get_uint64().value_unsafe(), 3 );

      ASSERT_ERROR( object.find_field("a"), NO_SUCH_FIELD );
      ASSERT_ERROR( object.find_field("d"), NO_SUCH_FIELD );
      return true;
    }));
    TEST_SUCCEED();
  }

#if SIMDJSON_SUPPORTS_CONCEPTS
  bool object_find_field_key_selector() {
    TEST_START();
    auto json = R"({ "name": "John", "age": 30, "city": "New York" })"_padded;
    using sel_t = decltype(ondemand::make_key_selector<"name", "age", "city">());

    // The selector maps keys to compile-time indices.
    ASSERT_EQUAL(sel_t::size(), 3);
    ASSERT_EQUAL(sel_t::key_at(0), "name");
    ASSERT_EQUAL(sel_t::key_at(1), "age");
    ASSERT_EQUAL(sel_t::key_at(2), "city");

    SUBTEST("ondemand::object with key_selector for_each", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );

      std::array<bool, 3> seen{};
      std::string_view name_val{};
      uint64_t age_val{};
      std::string_view city_val{};
      // for_each only yields indices in [0, size()), so index is always < 3 here.
      ASSERT_SUCCESS( object.for_each<sel_t>([&](std::size_t index, ondemand::value v) {
        seen[index] = true;
        switch (index) {
          case 0: { std::string_view s; if (!v.get(s)) { name_val = s; } break; }
          case 1: { uint64_t n;         if (!v.get(n)) { age_val  = n; } break; }
          case 2: { std::string_view s; if (!v.get(s)) { city_val = s; } break; }
        }
      }) );

      ASSERT_TRUE(seen[0] && seen[1] && seen[2]);
      ASSERT_EQUAL(name_val, "John");
      ASSERT_EQUAL(age_val, 30);
      ASSERT_EQUAL(city_val, "New York");

      return true;
    }));
    TEST_SUCCEED();
  }
#endif

  bool run() {
    return
           object_find_field_unordered() &&
           document_object_find_field_unordered() &&
           value_object_find_field_unordered() &&
           object_find_field() &&
           document_object_find_field() &&
           value_object_find_field() &&
#if SIMDJSON_SUPPORTS_CONCEPTS
           object_find_field_key_selector() &&
#endif
           true;
  }

} // namespace object_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, object_tests::run);
}
