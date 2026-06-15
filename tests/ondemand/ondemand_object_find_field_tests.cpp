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

    // for_each forwards on a simdjson_result<object>, so an error-code-style
    // chain can call it without first extracting the object.
    SUBTEST("simdjson_result<ondemand::object> with key_selector for_each", test_ondemand_doc(json, [&](auto doc_result) {
      std::string_view name_val{};
      std::size_t matched = 0;
      auto walk = doc_result.get_object().template for_each<sel_t>([&](std::size_t index, ondemand::value v) {
        ++matched;
        if (index == 0) { std::string_view s; if (!v.get(s)) { name_val = s; } }
      });
      ASSERT_SUCCESS( walk.error );
      ASSERT_EQUAL(matched, 3);
      ASSERT_EQUAL(name_val, "John");
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
    // A non-throwing callback keeps for_each noexcept (the conditional-noexcept
    // counterpart to for_each_throwing_callback_propagates below).
    static_assert(noexcept(std::declval<ondemand::object&>().for_each<sel_t>(
        [](std::size_t, ondemand::value) noexcept {})),
        "for_each must be noexcept when the callback is noexcept");

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

  // Exercises the variadic for_each forms using ONLY non-throwing operations
  // (direct variable binding via value::get, and error_code-returning lambdas),
  // so this test compiles and runs in builds with SIMDJSON_EXCEPTIONS=OFF. It
  // confirms the API is fully usable without exceptions: errors surface through
  // the returned for_each_result, and the walk stops at the first failing field.
  bool object_for_each_variadic_no_exceptions() {
    TEST_START();
    auto json = R"({ "name": "Daniel", "age": 42, "city": "Montreal", "extra": 1 })"_padded;
    using sel3 = ondemand::key_selector<"name", "city", "age">;

    SUBTEST("direct variable binding via named selector", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS( doc_result.get(obj) );
      std::string_view name, city;
      uint64_t age = 0;
      auto error = obj.for_each<sel3>(name, city, age);
      ASSERT_SUCCESS( error );
      ASSERT_EQUAL( error.matched_count, 3u );
      ASSERT_EQUAL( name, "Daniel" );
      ASSERT_EQUAL( city, "Montreal" );
      ASSERT_EQUAL( age, 42u );
      return true;
    }));

    SUBTEST("direct variable binding via direct keys", test_ondemand_doc(json, [&](auto doc_result) {
      std::string_view name, city;
      uint64_t age = 0;
      ASSERT_SUCCESS( doc_result.get_object().template for_each<"name", "city", "age">(name, city, age) );
      ASSERT_EQUAL( name, "Daniel" );
      ASSERT_EQUAL( city, "Montreal" );
      ASSERT_EQUAL( age, 42u );
      return true;
    }));

    SUBTEST("error_code per-value lambdas", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS( doc_result.get(obj) );
      std::string_view name, city; uint64_t age = 0;
      ASSERT_SUCCESS( obj.for_each<sel3>(
        [&](ondemand::value v) -> simdjson::error_code { return v.get(name); },
        [&](ondemand::value v) -> simdjson::error_code { return v.get(city); },
        [&](ondemand::value v) -> simdjson::error_code { return v.get(age); }
      ) );
      ASSERT_EQUAL( name, "Daniel" );
      ASSERT_EQUAL( city, "Montreal" );
      ASSERT_EQUAL( age, 42u );
      return true;
    }));

    SUBTEST("mixed direct target and error_code lambda", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS( doc_result.get(obj) );
      std::string_view name; uint64_t age = 0;
      ASSERT_SUCCESS( obj.for_each<ondemand::key_selector<"name", "age">>(
        name, // direct target
        [&](ondemand::value v) -> simdjson::error_code { return v.get(age); }
      ) );
      ASSERT_EQUAL( name, "Daniel" );
      ASSERT_EQUAL( age, 42u );
      return true;
    }));

    SUBTEST("direct binding stops on type mismatch", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS( doc_result.get(obj) );
      // "age" is a number; binding it to a string_view fails with INCORRECT_TYPE.
      std::string_view name, city, age_as_string;
      auto error = obj.for_each<sel3>(name, city, age_as_string);
      ASSERT_ERROR( error, INCORRECT_TYPE );
      return true;
    }));

    SUBTEST("error_code lambda stops the walk", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS( doc_result.get(obj) );
      // sel3 keys are name(0), city(1), age(2); JSON order is name, age, city.
      // So the walk fires cb0 (name), then cb2 (age), then cb1 (city). Make the
      // age callback (fired 2nd) stop the walk, and assert the city callback
      // (which would fire 3rd) never runs.
      bool ran_name = false, ran_age = false, ran_city = false;
      auto error = obj.for_each<sel3>(
        [&](ondemand::value) -> simdjson::error_code { ran_name = true; return SUCCESS; },
        [&](ondemand::value) -> simdjson::error_code { ran_city = true; return SUCCESS; },
        [&](ondemand::value) -> simdjson::error_code { ran_age = true; return INCORRECT_TYPE; }
      );
      ASSERT_ERROR( error, INCORRECT_TYPE );
      ASSERT_TRUE( ran_name );
      ASSERT_TRUE( ran_age );
      ASSERT_FALSE( ran_city );
      return true;
    }));

    auto nested_json = R"({ "id": 7, "author": { "name": "Lemire", "handle": "x" } })"_padded;
    SUBTEST("nested for_each result propagates", test_ondemand_doc(nested_json, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS( doc_result.get(obj) );
      uint64_t id = 0; std::string_view author_name, author_handle;
      ASSERT_SUCCESS( obj.for_each<ondemand::key_selector<"id", "author">>(
        id,
        [&](ondemand::value v) -> simdjson::error_code {
          ondemand::object author;
          SIMDJSON_TRY( v.get_object().get(author) );
          // Return the nested for_each_result; it converts to error_code.
          return author.for_each<"name", "handle">(author_name, author_handle);
        }
      ) );
      ASSERT_EQUAL( id, 7u );
      ASSERT_EQUAL( author_name, "Lemire" );
      ASSERT_EQUAL( author_handle, "x" );
      return true;
    }));

    TEST_SUCCEED();
  }

  // The following key_selector_doc_* functions are exercised, verbatim, as the
  // examples in doc/basics.md "Key selectors". They use the error-code style (no
  // throwing conversions) so they compile and run with SIMDJSON_EXCEPTIONS=OFF.

  // Mirrors doc/basics.md "Key selectors": binding fields straight to variables,
  // calling for_each on the simdjson_result<object> directly (error forwarding),
  // and reading matched_count.
  bool key_selector_doc_bind_variables() {
    TEST_START();
    auto json = R"({ "name": "Daniel", "age": 42, "city": "Montreal" })"_padded;
    ondemand::parser parser;
    auto doc = parser.iterate(json);
    std::string_view name, city;
    uint64_t age = 0;
    // for_each is called on the simdjson_result<object> returned by get_object():
    // had get_object() failed, that error would surface through result.error
    // without ever touching the object.
    auto result = doc.get_object().for_each<"name", "city", "age">(name, city, age);
    ASSERT_SUCCESS(result.error);
    ASSERT_EQUAL(result.matched_count, 3u);
    ASSERT_EQUAL(name, "Daniel");
    ASSERT_EQUAL(city, "Montreal");
    ASSERT_EQUAL(age, 42u);
    TEST_SUCCEED();
  }

  // Mirrors doc/basics.md "Key selectors": matched_count when only some selected
  // keys are present in the document.
  bool key_selector_doc_matched_count() {
    TEST_START();
    // Of the three selected keys, only "name" appears in the document.
    auto json = R"({ "name": "Daniel", "age": 42 })"_padded;
    ondemand::parser parser;
    auto doc = parser.iterate(json);
    std::string_view name, city, country;
    auto result = doc.get_object().for_each<"name", "city", "country">(name, city, country);
    ASSERT_SUCCESS(result.error);
    ASSERT_EQUAL(result.matched_count, 1u); // only "name" matched
    ASSERT_EQUAL(name, "Daniel");
    TEST_SUCCEED();
  }

  // Mirrors doc/basics.md "Key selectors": a value-parsing error (type mismatch)
  // surfaces through the for_each result and stops the walk.
  bool key_selector_doc_error_propagation() {
    TEST_START();
    // "age" is a number; binding it to a std::string_view fails with
    // INCORRECT_TYPE, and that error is reported through result.error.
    auto json = R"({ "name": "Daniel", "age": 42 })"_padded;
    ondemand::parser parser;
    auto doc = parser.iterate(json);
    std::string_view name;
    std::string_view age_as_string; // wrong target type for the number "age"
    auto result = doc.get_object().for_each<"name", "age">(name, age_as_string);
    ASSERT_ERROR(result.error, INCORRECT_TYPE);
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

    std::string_view name, city;
    ASSERT_SUCCESS( obj.for_each<"name", "city">(
      [&](ondemand::value v){ name = std::string_view(v); },
      [&](ondemand::value v){ city = std::string_view(v); }
    ) );
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

    uint64_t id = 0;
    std::string_view handle;
    ASSERT_SUCCESS( user.for_each<"id", "screen_name">(
      [&](ondemand::value v){ id     = uint64_t(v); },
      [&](ondemand::value v){ handle = std::string_view(v); }
    ) );
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

    uint64_t id = 0;
    std::string_view title, author_name, author_handle;
    ASSERT_SUCCESS( obj.for_each<"id", "author", "title">(
      [&](ondemand::value v){ id = uint64_t(v); },
      [&](ondemand::value v) -> simdjson::error_code {  // "author" is itself an object
        ondemand::object author;
        SIMDJSON_TRY( v.get_object().get(author) );
        // Bind the inner fields directly; return the nested for_each result so
        // any inner error propagates out through the outer for_each.
        return author.for_each<"name", "handle">(author_name, author_handle);
      },
      [&](ondemand::value v){ title = std::string_view(v); }
    ) );
    ASSERT_EQUAL(id, 42);
    ASSERT_EQUAL(title, "On Demand");
    ASSERT_EQUAL(author_name, "Daniel");
    ASSERT_EQUAL(author_handle, "lemire");
    TEST_SUCCEED();
  }

  // Mirrors doc/basics.md "Key selectors": mixing a direct variable target and a
  // lambda in the same call (one handler per key).
  bool key_selector_doc_mixed() {
    TEST_START();
    auto json = R"({ "name": "Daniel", "age": 42, "city": "Montreal" })"_padded;
    ondemand::parser parser;
    ondemand::document doc = parser.iterate(json);
    ondemand::object obj = doc.get_object();

    std::string_view name;
    uint64_t age_doubled = 0;
    // "name" binds straight to a variable; "age" runs a lambda for custom logic.
    ASSERT_SUCCESS( obj.for_each<"name", "age">(
      name,
      [&](ondemand::value v){ age_doubled = 2 * uint64_t(v); }
    ) );
    ASSERT_EQUAL(name, "Daniel");
    ASSERT_EQUAL(age_doubled, 84u);
    TEST_SUCCEED();
  }

  // Mirrors doc/basics.md "Key selectors": the index-based single-callback form.
  bool key_selector_doc_index_callback() {
    TEST_START();
    auto json = R"({ "name": "Daniel", "age": 42, "city": "Montreal" })"_padded;
    ondemand::parser parser;
    ondemand::document doc = parser.iterate(json);
    ondemand::object obj = doc.get_object();

    using sel = ondemand::key_selector<"name", "city", "age">;
    std::string_view name, city;
    uint64_t age = 0;
    // One callback receives (index, value); the index is the key's position in
    // the selector ("name"=0, "city"=1, "age"=2).
    ASSERT_SUCCESS( obj.for_each<sel>([&](std::size_t i, ondemand::value v) {
      switch (i) {
        case 0: name = std::string_view(v); break;
        case 1: city = std::string_view(v); break;
        case 2: age  = uint64_t(v); break;
      }
    }) );
    ASSERT_EQUAL(name, "Daniel");
    ASSERT_EQUAL(city, "Montreal");
    ASSERT_EQUAL(age, 42u);
    TEST_SUCCEED();
  }

  // Regression test for the conditionally-noexcept for_each: the documented
  // callback style uses throwing conversions (std::string_view(v), uint64_t(v)),
  // and the callback runs inside for_each's frame. On a type mismatch the
  // exception must propagate out to the caller (catchable) instead of crossing a
  // noexcept boundary and calling std::terminate.
  bool for_each_throwing_callback_propagates() {
    TEST_START();
    // "name" is a number, but the callback converts it to std::string_view,
    // which throws INCORRECT_TYPE.
    auto json = R"({ "name": 123, "city": "Montreal" })"_padded;
    ondemand::parser parser;
    ondemand::document doc = parser.iterate(json);
    ondemand::object obj = doc.get_object();
    using fields = ondemand::key_selector<"name", "city">;
    // for_each must NOT be noexcept here: the callback can throw.
    static_assert(!noexcept(obj.for_each<fields>(
        [&](std::size_t, ondemand::value v) { (void)std::string_view(v); })),
        "for_each must be potentially-throwing when the callback can throw");
    std::string_view name, city;
    bool threw = false;
    try {
      // The callback throws before for_each can return, so discard the result.
      (void) obj.for_each<fields>([&](std::size_t i, ondemand::value v) {
        switch (i) {
          case 0: name = std::string_view(v); break; // throws INCORRECT_TYPE
          case 1: city = std::string_view(v); break;
        }
      });
    } catch (simdjson_error &) {
      threw = true;
    }
    ASSERT_TRUE(threw);
    TEST_SUCCEED();
  }

  // Tests for the new variadic per-key callback forms (both <Selector>(cb0,cb1,...)
  // and the direct-key <"k0","k1",...>(cb0,cb1,...) shorthand). Exercises
  // success, error_code return, void callbacks, simdjson_result forwarding,
  // N=1 edge, error propagation, and mixed void/error-cb.
  bool object_for_each_variadic_callbacks() {
    TEST_START();
    auto json = R"({ "name": "Daniel", "age": 42, "city": "Montreal", "extra": 1 })"_padded;
    using sel3 = ondemand::key_selector<"name", "city", "age">; // 3 keys, order in JSON different

    SUBTEST("variadic via named selector (3 cbs, void)", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS(doc_result.get(obj));
      std::string_view name, city;
      uint64_t age = 0;
      auto res = obj.for_each<sel3>(
        [&](ondemand::value v){ name = std::string_view(v); },
        [&](ondemand::value v){ city = std::string_view(v); },
        [&](ondemand::value v){ age  = uint64_t(v); }
      );
      ASSERT_SUCCESS(res.error);
      ASSERT_EQUAL(res.matched_count, 3u);
      ASSERT_EQUAL(name, "Daniel");
      ASSERT_EQUAL(city, "Montreal");
      ASSERT_EQUAL(age, 42u);
      return true;
    }));

    SUBTEST("variadic via named selector (error_code cbs)", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS(doc_result.get(obj));
      std::string_view name; uint64_t age = 0;
      ASSERT_SUCCESS( obj.for_each<sel3>(
        [&](ondemand::value v) -> simdjson::error_code { return v.get(name); },
        [&](ondemand::value v) -> simdjson::error_code { std::string_view ignore; return v.get(ignore); }, // city ignored
        [&](ondemand::value v) -> simdjson::error_code { return v.get(age); }
      ) );
      ASSERT_EQUAL(name, "Daniel");
      ASSERT_EQUAL(age, 42u);
      return true;
    }));

    SUBTEST("variadic error propagation (type mismatch in 2nd cb)", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS(doc_result.get(obj));
      ASSERT_ERROR( obj.for_each<sel3>(
        [&](ondemand::value v){ (void)std::string_view(v); },
        [&](ondemand::value v){ (void)std::string_view(v); },
        [&](ondemand::value v) -> simdjson::error_code { std::string_view s; return v.get(s); } // age (number) as string -> error
      ), INCORRECT_TYPE );
      return true;
    }));

    SUBTEST("direct keys shorthand", test_ondemand_doc(json, [&](auto doc_result) {
      std::string_view name, city; uint64_t age = 0;
      auto res = doc_result.get_object().template for_each<"name", "city", "age">(
        [&](ondemand::value v){ name = std::string_view(v); },
        [&](ondemand::value v){ city = std::string_view(v); },
        [&](ondemand::value v){ age  = uint64_t(v); }
      );
      ASSERT_SUCCESS(res);
      ASSERT_EQUAL(name, "Daniel");
      ASSERT_EQUAL(city, "Montreal");
      ASSERT_EQUAL(age, 42u);
      return true;
    }));

    SUBTEST("N=1 direct key (absent key -> matched_count 0)", test_ondemand_doc(json, [&](auto doc_result) {
      auto r1 = doc_result.get_object().template for_each<"only">(
        [&](ondemand::value /*v*/){ /* not present, cb not called */ }
      );
      ASSERT_SUCCESS(r1.error);
      ASSERT_EQUAL(r1.matched_count, 0u);
      return true;
    }));

    SUBTEST("N=1 via named selector (present)", test_ondemand_doc(json, [&](auto doc_result) {
      using only_sel = ondemand::key_selector<"name">;
      std::string_view nm;
      auto r2 = doc_result.get_object().template for_each<only_sel>(
        [&](ondemand::value v) -> simdjson::error_code { return v.get(nm); }
      );
      ASSERT_SUCCESS(r2);
      ASSERT_EQUAL(nm, "Daniel");
      return true;
    }));

    SUBTEST("simdjson_result<object> forwarding for variadic", test_ondemand_doc(json, [&](auto doc_result) {
      std::string_view name; uint64_t a = 0;
      auto walk = doc_result.get_object().template for_each<"name", "age">(
        [&](ondemand::value v){ name = std::string_view(v); },
        [&](ondemand::value v){ a = uint64_t(v); }
      );
      ASSERT_SUCCESS(walk.error);
      ASSERT_EQUAL(walk.matched_count, 2u);
      ASSERT_EQUAL(name, "Daniel");
      ASSERT_EQUAL(a, 42u);
      return true;
    }));

    SUBTEST("direct variable binding (no lambdas)", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS(doc_result.get(obj));
      std::string_view name, city;
      uint64_t age = 0;
      // Each key binds straight to a variable; the matched value is assigned via
      // value::get -- no per-field lambda needed.
      auto res = obj.for_each<sel3>(name, city, age);
      ASSERT_SUCCESS(res.error);
      ASSERT_EQUAL(res.matched_count, 3u);
      ASSERT_EQUAL(name, "Daniel");
      ASSERT_EQUAL(city, "Montreal");
      ASSERT_EQUAL(age, 42u);
      return true;
    }));

    SUBTEST("direct variable binding via direct keys", test_ondemand_doc(json, [&](auto doc_result) {
      std::string_view name, city;
      uint64_t age = 0;
      auto res = doc_result.get_object().template for_each<"name", "city", "age">(name, city, age);
      ASSERT_SUCCESS(res.error);
      ASSERT_EQUAL(res.matched_count, 3u);
      ASSERT_EQUAL(name, "Daniel");
      ASSERT_EQUAL(city, "Montreal");
      ASSERT_EQUAL(age, 42u);
      return true;
    }));

    SUBTEST("mixed targets and lambda", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS(doc_result.get(obj));
      std::string_view name;
      uint64_t age_times_two = 0;
      // name binds directly; age uses a lambda for custom logic.
      auto res = obj.for_each<ondemand::key_selector<"name", "age">>(
        name,
        [&](ondemand::value v){ age_times_two = 2 * uint64_t(v); }
      );
      ASSERT_SUCCESS(res.error);
      ASSERT_EQUAL(res.matched_count, 2u);
      ASSERT_EQUAL(name, "Daniel");
      ASSERT_EQUAL(age_times_two, 84u);
      return true;
    }));

    SUBTEST("direct binding type-mismatch propagates error", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS(doc_result.get(obj));
      std::string_view name;
      std::string_view age_as_string; // "age" is a number -> INCORRECT_TYPE on get
      ASSERT_ERROR( obj.for_each<sel3>(name, name, age_as_string), INCORRECT_TYPE );
      return true;
    }));

    TEST_SUCCEED();
  }
#endif

#if SIMDJSON_SUPPORTS_CONCEPTS
  // The key_selector's matcher (match_raw, the perfect hash) must return the
  // right selector index for every key, including tricky cases: keys that are
  // prefixes of one another, keys that extend a real key, a long key, and misses.
  // This guards the prefix/length disambiguation against the hash.
  bool key_selector_matchers_agree() {
    TEST_START();
    // Probe builds "<key>\"" in a padded buffer and checks match_raw returns the
    // expected selector index (or N for a miss).
    auto probe = [](auto sel_tag, std::string_view key, std::size_t expected) -> bool {
      using sel = decltype(sel_tag);
      char buf[64] = {};
      for (size_t i = 0; i < key.size(); ++i) { buf[i] = key[i]; }
      buf[key.size()] = '"';
      ondemand::raw_json_string r(reinterpret_cast<const uint8_t*>(buf));
      ASSERT_EQUAL(sel::match_raw(r), expected);
      return true;
    };
    // Selector with prefix keys and a 30-character key.
    using small_sel = ondemand::key_selector<"a", "ab", "abc", "id", "name",
                                              "abcdefghijklmnopqrstuvwxyz1234">;
    std::size_t i = 0;
    for (auto k : {"a", "ab", "abc", "id", "name", "abcdefghijklmnopqrstuvwxyz1234"}) {
      if (!probe(small_sel{}, k, i++)) { return false; }
    }
    for (auto k : {"x", "abcd", "nam", "names", "i", "ids", "zzzzz", ""}) {
      if (!probe(small_sel{}, k, small_sel::size())) { return false; }
    }
    // Larger selector exercising the same matcher.
    using big_sel = ondemand::key_selector<"k00","k01","k02","k03","k04","k05",
                                            "k06","k07","k08","k09","k10","k11">;
    if (!probe(big_sel{}, "k00", 0)) { return false; }
    if (!probe(big_sel{}, "k05", 5)) { return false; }
    if (!probe(big_sel{}, "k11", 11)) { return false; }
    for (auto k : {"k12","nope",""}) {
      if (!probe(big_sel{}, k, big_sel::size())) { return false; }
    }
    // Single-byte discriminator: position 0 tells "id" from "screen_name".
    using id_sel = ondemand::key_selector<"id", "screen_name">;
    if (!probe(id_sel{}, "id", 0)) { return false; }
    if (!probe(id_sel{}, "screen_name", 1)) { return false; }
    // Misses that share the discriminating byte must still be rejected: "info"
    // and "i" share 'i' with "id"; "set" shares 's' with "screen_name";
    // "identifier" has "id" as a prefix and must fail on the closing quote.
    for (auto k : {"info", "i", "identifier", "set", "screen", "screen_names", "", "x"}) {
      if (!probe(id_sel{}, k, id_sel::size())) { return false; }
    }
    // Prefix keys split by the quote terminator at position 2 ("jo" vs "joe").
    using jo_sel = ondemand::key_selector<"jo", "joe">;
    if (!probe(jo_sel{}, "jo", 0)) { return false; }
    if (!probe(jo_sel{}, "joe", 1)) { return false; }
    for (auto k : {"j", "joey", "jp", "", "joel"}) {
      if (!probe(jo_sel{}, k, jo_sel::size())) { return false; }
    }
    // The partial_tweets keys: no aligned byte separates them, but an unaligned
    // 8-bit window does. Exercise the window fast path end to end.
    using tweet_sel = ondemand::key_selector<"created_at", "id", "text",
        "in_reply_to_status_id", "user", "retweet_count", "favorite_count">;
    const char* tweet_keys[] = {"created_at", "id", "text",
        "in_reply_to_status_id", "user", "retweet_count", "favorite_count"};
    for (std::size_t t = 0; t < tweet_sel::size(); ++t) {
      if (!probe(tweet_sel{}, tweet_keys[t], t)) { return false; }
    }
    // Misses, including ones that share a prefix with a real key.
    for (auto k : {"ide", "i", "id_str", "use", "users", "favorite_countX",
                   "in_reply_to_status_id_str", "created", "", "zzz"}) {
      if (!probe(tweet_sel{}, k, tweet_sel::size())) { return false; }
    }
    TEST_SUCCEED();
  }

  // Keys longer than the old 31-character limit (now up to 63) must match across
  // every 16-byte block boundary, on both the window fast path and the perfect-
  // hash path, and via both match_raw overloads.
  bool key_selector_long_keys() {
    TEST_START();
    // Larger buffer than key_selector_matchers_agree's probe: a 63-char key plus
    // the closing quote, with room for the 64-byte SIMD reads.
    auto probe = [](auto sel_tag, std::string_view key, std::size_t expected) -> bool {
      using sel = decltype(sel_tag);
      char buf[128] = {};
      for (size_t i = 0; i < key.size(); ++i) { buf[i] = key[i]; }
      buf[key.size()] = '"';
      ondemand::raw_json_string r(reinterpret_cast<const uint8_t*>(buf));
      // The length-scanning overload and the precomputed-length overload must
      // agree, and both must return the expected index.
      ASSERT_EQUAL(sel::match_raw(r), expected);
      ASSERT_EQUAL(sel::match_raw(buf, key.size()), expected);
      return true;
    };

    // Distinct first bytes => a single aligned 8-bit window separates them: the
    // window fast path with key lengths straddling the 16/32/48/63 boundaries.
    using win_sel = ondemand::key_selector<
        "aaaaaaaaaaaaaaaa",                                                // 16
        "bbbbbbbbbbbbbbbbb",                                               // 17
        "ccccccccccccccccccccccccccccccc",                                // 31
        "dddddddddddddddddddddddddddddddd",                               // 32
        "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",                              // 33
        "ffffffffffffffffffffffffffffffffffffffffffffffff",              // 48
        "ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg">; // 63
    static_assert(win_sel::window.ok, "expected the window fast path here");
    static_assert(win_sel::max_key_len == 63, "expected a 63-char max key");
    const char* win_keys[] = {
        "aaaaaaaaaaaaaaaa", "bbbbbbbbbbbbbbbbb",
        "ccccccccccccccccccccccccccccccc", "dddddddddddddddddddddddddddddddd",
        "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
        "ffffffffffffffffffffffffffffffffffffffffffffffff",
        "ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg"};
    for (std::size_t i = 0; i < win_sel::size(); ++i) {
      ASSERT_EQUAL(win_sel::key_at(i).size(),
                   (std::array<std::size_t, 7>{16,17,31,32,33,48,63})[i]);
      if (!probe(win_sel{}, win_keys[i], i)) { return false; }
    }
    // Misses: a truncated 63-char key (quote one byte early), an extended one
    // (quote one byte late, so the key is longer than any selector key), a
    // wrong-character key of a matching length, and the empty key.
    for (auto k : {"gggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg",   // 62 g's
                   "gggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg", // 64 g's
                   "cccccccccccccccccccccccccccccch",                                  // 31, last differs
                   ""}) {
      if (!probe(win_sel{}, k, win_sel::size())) { return false; }
    }

    // Same length (63), differing only at offsets 1-2: no 8-bit window separates
    // them, so this exercises the perfect-hash path -- and its SIMD length scan
    // and byte compare -- with long keys.
    using phf_sel = ondemand::key_selector<
        "k00aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "k01aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "k02aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "k03aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "k04aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "k05aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "k10aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "k11aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa">;
    static_assert(!phf_sel::window.ok, "expected the perfect-hash path here");
    static_assert(phf_sel::max_key_len == 63, "expected a 63-char max key");
    for (std::size_t i = 0; i < phf_sel::size(); ++i) {
      ASSERT_EQUAL(phf_sel::key_at(i).size(), std::size_t(63));
      if (!probe(phf_sel{}, phf_sel::key_at(i), i)) { return false; }
    }
    // Misses on the perfect-hash path: a key that matches the common suffix but
    // has an unknown prefix ("k20..."), and a key sharing a 62-char prefix with
    // index 0 but differing in the final byte.
    if (!probe(phf_sel{}, "k20aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", phf_sel::size())) { return false; }
    if (!probe(phf_sel{}, "k00aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab", phf_sel::size())) { return false; }
    TEST_SUCCEED();
  }

  // End-to-end for_each over an object whose keys are 63 characters long (the new
  // maximum), so the full walk -- field_key_with_length deriving the length and
  // match_raw classifying it -- works past the old 31-character limit.
  bool key_selector_long_keys_for_each() {
    TEST_START();
    using sel_t = ondemand::key_selector<
        "ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg",   // 63
        "ccccccccccccccccccccccccccccccc">;                                  // 31
    auto json = R"({
      "ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg": 7,
      "ignored_key": 99,
      "ccccccccccccccccccccccccccccccc": 5
    })"_padded;
    SUBTEST("for_each with 63-char keys", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::object object;
      ASSERT_SUCCESS( doc_result.get(object) );
      uint64_t g_val = 0, c_val = 0;
      std::size_t matched = 0;
      ASSERT_SUCCESS( object.for_each<sel_t>([&](std::size_t index, ondemand::value v) -> simdjson::error_code {
        ++matched;
        switch (index) {
          case 0: return v.get(g_val);
          case 1: return v.get(c_val);
          default: return SUCCESS;
        }
      }) );
      ASSERT_EQUAL(matched, 2);
      ASSERT_EQUAL(g_val, 7);
      ASSERT_EQUAL(c_val, 5);
      return true;
    }));
    TEST_SUCCEED();
  }

  // Exercises the window fast path end-to-end (via object::for_each on real
  // documents) with deliberate prefix/quote-boundary key sets. "jo"/"joe" tests
  // that the closing-quote check inside the window candidate confirmation
  // correctly distinguishes a short key from a longer one that shares its prefix
  // (the window byte may point at the quote for the short key). "id"/"screen_name"
  // exercises a simple offset-0 single-byte discriminator window. Verifies
  // dispatch to the right index, matched_count, absent/short-circuit behavior,
  // and that prefix misses do not spuriously match.
  bool key_selector_window_prefix_quote_for_each() {
    TEST_START();
    // "jo" vs "joe": window + p[len] == '"' check is critical.
    using jo_sel = ondemand::key_selector<"jo", "joe">;
    static_assert(jo_sel::window.ok, "expected window fast path for jo/joe");

    auto json_both = R"({ "jo": 10, "extra": 99, "joe": 20 })"_padded;
    SUBTEST("for_each jo/joe both present (prefix+quote boundary)", test_ondemand_doc(json_both, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS( doc_result.get(obj) );
      int jo_v = 0, joe_v = 0;
      std::size_t matched = 0;
      ASSERT_SUCCESS( obj.for_each<jo_sel>([&](std::size_t i, ondemand::value v) -> simdjson::error_code {
        ++matched;
        if (i == 0) return v.get(jo_v);
        if (i == 1) return v.get(joe_v);
        return SUCCESS;
      }) );
      ASSERT_EQUAL(matched, 2u);
      ASSERT_EQUAL(jo_v, 10);
      ASSERT_EQUAL(joe_v, 20);
      return true;
    }));

    auto json_joe_only = R"({ "joe": 42, "joker": 7 })"_padded;
    SUBTEST("for_each jo/joe (only longer; short prefix must not match via window)", test_ondemand_doc(json_joe_only, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS( doc_result.get(obj) );
      int jo_v = -1, joe_v = 0;
      auto res = obj.for_each<jo_sel>([&](std::size_t i, ondemand::value v) -> simdjson::error_code {
        if (i == 0) return v.get(jo_v);
        if (i == 1) return v.get(joe_v);
        return SUCCESS;
      });
      ASSERT_SUCCESS(res);
      ASSERT_EQUAL(res.matched_count, 1u);
      ASSERT_EQUAL(joe_v, 42);
      ASSERT_EQUAL(jo_v, -1); // short handler must not have run
      return true;
    }));

    // Single-byte window at offset 0.
    using id_sel = ondemand::key_selector<"id", "screen_name">;
    static_assert(id_sel::window.ok, "expected window for id/screen_name");
    auto json_id = R"({ "id": 123, "screen_name": "lemire", "followers": 5 })"_padded;
    SUBTEST("for_each id/screen_name (window, early stop, extra keys ignored)", test_ondemand_doc(json_id, [&](auto doc_result) {
      ondemand::object obj;
      ASSERT_SUCCESS( doc_result.get(obj) );
      uint64_t idv = 0;
      std::string_view sn{};
      auto res = obj.for_each<id_sel>(idv, sn);
      ASSERT_SUCCESS(res);
      ASSERT_EQUAL(res.matched_count, 2u);
      ASSERT_EQUAL(idv, 123u);
      ASSERT_EQUAL(sn, "lemire");
      return true;
    }));

    TEST_SUCCEED();
  }

  // key_selector::describe() returns a complete, compile-time-derived description
  // of the matching algorithm. This checks the window-mode example used verbatim
  // in doc/basics.md, that describe() is a constant expression, and the stable
  // structural parts of the perfect-hash form.
  bool key_selector_describe() {
    TEST_START();
    using fields = ondemand::key_selector<"name", "city">;
    // describe() is usable in a constant expression.
    static_assert(!fields::describe().empty(),
                  "describe() must be usable at compile time");
    // The exact text shown in doc/basics.md (keys differ in their first byte, so a
    // single 8-bit window at offset 0 distinguishes them).
    const std::string expected =
        "key_selector: 2 keys, max key length 4\n"
        "keys:\n"
        "  [0] \"name\" (length 4)\n"
        "  [1] \"city\" (length 4)\n"
        "algorithm: single 8-bit window\n"
        "  step 1: read 2 bytes at offset 0, interpret them as a little-endian 16-bit value, shift right by 0 bits, and keep the low 8 bits\n"
        "  step 2: map that byte through a 256-entry table to a key index (2 means no match):\n"
        "    byte 99 ('c') -> key 1\n"
        "    byte 110 ('n') -> key 0\n"
        "  step 3: confirm the candidate by checking the closing quote sits at the key's length and comparing the key bytes\n";
    ASSERT_EQUAL(fields::describe(), expected);

    // gperf example from doc/basics.md: six anagrams of "abc" -- no character
    // position is unique to a key, so no window works and a gperf-style perfect
    // hash over two positions is used. The hash construction is deterministic, so
    // the slots/h-values are checked verbatim (and match the documentation).
    using anagrams = ondemand::key_selector<"abc","acb","bac","bca","cab","cba">;
    static_assert(!anagrams::window.ok, "anagram example must use the perfect-hash path");
    const std::string expected_gperf =
        "key_selector: 6 keys, max key length 3\n"
        "keys:\n"
        "  [0] \"abc\" (length 3)\n"
        "  [1] \"acb\" (length 3)\n"
        "  [2] \"bac\" (length 3)\n"
        "  [3] \"bca\" (length 3)\n"
        "  [4] \"cab\" (length 3)\n"
        "  [5] \"cba\" (length 3)\n"
        "algorithm: gperf-style perfect hash over 2 character position(s)\n"
        "  step 1: h = key length\n"
        "  step 2: for each position below, add its association value for the key byte there (a position past the key's length contributes 0):\n"
        "    position byte index 0\n"
        "    position last character\n"
        "  step 3: slot = h mod 8 (a power of two, applied as a bitmask)\n"
        "  step 4: slot_to_key[slot] gives the candidate key index\n"
        "  per-key derivation:\n"
        "    \"abc\": h=9 slot=1\n"
        "    \"acb\": h=6 slot=6\n"
        "    \"bac\": h=10 slot=2\n"
        "    \"bca\": h=4 slot=4\n"
        "    \"cab\": h=8 slot=0\n"
        "    \"cba\": h=5 slot=5\n"
        "  occupied slots (slot -> key):\n"
        "    slot 0 -> key 4 (\"cab\", length 3)\n"
        "    slot 1 -> key 0 (\"abc\", length 3)\n"
        "    slot 2 -> key 2 (\"bac\", length 3)\n"
        "    slot 4 -> key 3 (\"bca\", length 3)\n"
        "    slot 5 -> key 5 (\"cba\", length 3)\n"
        "    slot 6 -> key 1 (\"acb\", length 3)\n"
        "  confirm the candidate by checking the key length matches and comparing the key bytes\n";
    ASSERT_EQUAL(anagrams::describe(), expected_gperf);

    // A selector whose keys share their first byte cannot use a single window and
    // falls back to a perfect hash. The exact slot numbers depend on the hash, so
    // we check only the stable structural parts (and that every key is listed).
    using big = ondemand::key_selector<"k00","k01","k02","k03","k04","k05",
                                        "k06","k07","k08","k09","k10","k11">;
    const std::string d = big::describe();
    ASSERT_TRUE(d.find("key_selector: 12 keys, max key length 3\n") != std::string::npos);
    ASSERT_TRUE(d.find("perfect hash") != std::string::npos);
    ASSERT_TRUE(d.find("occupied slots (slot -> key):\n") != std::string::npos);
    ASSERT_TRUE(d.find("confirm the candidate") != std::string::npos);
    for (auto k : {"\"k00\"", "\"k05\"", "\"k11\""}) {
      ASSERT_TRUE(d.find(k) != std::string::npos);
    }
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
           object_for_each_variadic_no_exceptions() &&
           key_selector_doc_bind_variables() &&
           key_selector_doc_matched_count() &&
           key_selector_doc_error_propagation() &&
           key_selector_matchers_agree() &&
           key_selector_long_keys() &&
           key_selector_long_keys_for_each() &&
           key_selector_window_prefix_quote_for_each() &&
           key_selector_describe() &&
#endif
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
           object_for_each_variadic_callbacks() &&
           key_selector_example_toplevel() &&
           key_selector_example_nested() &&
           key_selector_example_inner_object() &&
           key_selector_doc_mixed() &&
           key_selector_doc_index_callback() &&
           for_each_throwing_callback_propagates() &&
#endif
           true;
  }

} // namespace object_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, object_tests::run);
}
