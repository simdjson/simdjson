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
    using sel_t = ondemand::key_selector<"name", "age", "city">;

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

  // Regression test for the for_each callback error contract: when the callback
  // returns an error_code, for_each stops at the first non-SUCCESS result and
  // returns it (so a value-parse error is no longer silently dropped).
  bool object_for_each_callback_error() {
    TEST_START();
    using sel_t = ondemand::key_selector<"id", "name">;
    auto json = R"({ "id": 7, "name": "Daniel" })"_padded;

    // Happy path: an error_code-returning callback extracts the values and
    // for_each returns SUCCESS.
    SUBTEST("error_code callback success", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      uint64_t id{};
      std::string_view name{};
      ASSERT_SUCCESS( object.for_each<sel_t>([&](std::size_t index, ondemand::value v) -> simdjson::error_code {
        switch (index) {
          case 0: { return v.get(id); }
          case 1: { return v.get(name); }
          default: { return SUCCESS; }
        }
      }) );
      ASSERT_EQUAL(id, 7);
      ASSERT_EQUAL(name, "Daniel");
      return true;
    }));

    // Error path: a value-parse error inside the callback (here, reading the
    // integer "id" as a string) is propagated by for_each.
    SUBTEST("error_code callback propagates error", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      ASSERT_ERROR( object.for_each<sel_t>([&](std::size_t index, ondemand::value v) -> simdjson::error_code {
        std::string_view s;
        if (index == 0) { return v.get(s); } // wrong type on purpose
        return SUCCESS;
      }), INCORRECT_TYPE );
      return true;
    }));
    TEST_SUCCEED();
  }
#endif

#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
  // Mirrors doc/basics.md "Key selectors", Example 1 (top-level fields).
  bool key_selector_example_toplevel() {
    TEST_START();
    auto json = R"({ "name": "Daniel", "age": 42, "city": "Montreal" })"_padded;
    ondemand::parser parser;
    ondemand::document doc = parser.iterate(json);
    ondemand::object obj = doc.get_object();

    using fields = ondemand::key_selector<"name", "city">;

    std::string_view name, city;
    obj.for_each<fields>([&](std::size_t i, ondemand::value v) {
      switch (i) {
        case 0: name = std::string_view(v); break; // "name"
        case 1: city = std::string_view(v); break; // "city"
      }
    });
    ASSERT_EQUAL(name, "Daniel");
    ASSERT_EQUAL(city, "Montreal");
    TEST_SUCCEED();
  }

  // Mirrors doc/basics.md "Key selectors", Example 2 (nested object).
  bool key_selector_example_nested() {
    TEST_START();
    auto json = R"({ "user": { "id": 1186275104, "screen_name": "ayuu0123", "verified": false } })"_padded;
    ondemand::parser parser;
    ondemand::document doc = parser.iterate(json);
    ondemand::object user = doc.find_field("user").get_object();

    using user_fields = ondemand::key_selector<"id", "screen_name">;

    uint64_t id = 0;
    std::string_view handle;
    user.for_each<user_fields>([&](std::size_t i, ondemand::value v) {
      switch (i) {
        case 0: id     = uint64_t(v);         break; // "id"
        case 1: handle = std::string_view(v); break; // "screen_name"
      }
    });
    ASSERT_EQUAL(id, 1186275104);
    ASSERT_EQUAL(handle, "ayuu0123");
    TEST_SUCCEED();
  }

  // Mirrors doc/basics.md "Key selectors", Example 3 (a selected value that is
  // itself an object, processed with a nested for_each).
  bool key_selector_example_inner_object() {
    TEST_START();
    auto json = R"({
      "id": 42,
      "author": { "name": "Daniel", "handle": "lemire" },
      "title": "On Demand"
    })"_padded;
    ondemand::parser parser;
    ondemand::document doc = parser.iterate(json);
    ondemand::object obj = doc.get_object();

    using post_fields   = ondemand::key_selector<"id", "author", "title">;
    using author_fields = ondemand::key_selector<"name", "handle">;

    uint64_t id = 0;
    std::string_view title, author_name, author_handle;
    obj.for_each<post_fields>([&](std::size_t i, ondemand::value v) {
      switch (i) {
        case 0: id = uint64_t(v); break;                       // "id"
        case 1: {                                              // "author" is itself an object
          ondemand::object author = v.get_object();
          author.for_each<author_fields>([&](std::size_t j, ondemand::value av) {
            switch (j) {
              case 0: author_name   = std::string_view(av); break; // "name"
              case 1: author_handle = std::string_view(av); break; // "handle"
            }
          });
          break;
        }
        case 2: title = std::string_view(v); break;            // "title"
      }
    });
    ASSERT_EQUAL(id, 42);
    ASSERT_EQUAL(title, "On Demand");
    ASSERT_EQUAL(author_name, "Daniel");
    ASSERT_EQUAL(author_handle, "lemire");
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
           object_for_each_callback_error() &&
#endif
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
           key_selector_example_toplevel() &&
           key_selector_example_nested() &&
           key_selector_example_inner_object() &&
#endif
           true;
  }

} // namespace object_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, object_tests::run);
}
