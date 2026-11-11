#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace error_tests {
  using namespace std;

  bool issue2120() {
    TEST_START();
    ondemand::parser parser;
    size_t bad_capacity = 0;
    ondemand::document doc;
    ASSERT_ERROR( parser.iterate("{\"a\": 0 }", bad_capacity).get(doc), INSUFFICIENT_PADDING);
    TEST_SUCCEED();
  }

  bool badbadjson() {
    TEST_START();
    ondemand::parser parser;
    auto json_string = R"({
      "main": "therain"_"in_spain"_
})"_padded;
    ondemand::document document;
    auto error = parser.iterate(json_string).get(document);
    if(error != simdjson::SUCCESS) {
      return false;
    }
    ondemand::object obj;
    error =  document.get_object().get(obj);
    if(error != simdjson::SUCCESS) {
      return false;
    }
    std::string_view name_value{};
    error = obj["name"].get_string().get(name_value);
    ASSERT_ERROR(error,TAPE_ERROR);
    // Check for "main" field
    std::string_view main_value{};
    error = obj["main"].get_string().get(main_value);
    ASSERT_ERROR(error,TAPE_ERROR);
    TEST_SUCCEED();
  }


  bool badbadjson2() {
    TEST_START();
    ondemand::parser parser;
    auto json_string = R"({
      "main": "therain"_"in_spain"_
})"_padded;
    ondemand::document document;
    auto error = parser.iterate(json_string).get(document);
    if(error != simdjson::SUCCESS) {
      return false;
    }
    ondemand::object obj;
    error =  document.get_object().get(obj);
    if(error != simdjson::SUCCESS) {
      return false;
    }

    std::string_view main_value{};
    error = obj["main"].get_string().get(main_value);
    ASSERT_ERROR(error, simdjson::SUCCESS);
    std::string_view name_value{};
    error = obj["name"].get_string().get(name_value);
    ASSERT_ERROR(error,TAPE_ERROR);
    TEST_SUCCEED();
  }

  bool issue1834() {
    TEST_START();
    ondemand::parser parser;
    auto json = "[[]"_padded;
    json.data()[json.size()] = ']';
    auto doc = parser.iterate(json);
    size_t cnt{};
    auto error = doc.count_elements().get(cnt);
    return error != simdjson::SUCCESS;
  }
  bool issue1834_2() {
    TEST_START();
    ondemand::parser parser;
    auto json = "{\"a\":{}"_padded;
    json.data()[json.size()] = '}';
    auto doc = parser.iterate(json);
    size_t cnt{};
    auto error = doc.count_fields().get(cnt);
    return error != simdjson::SUCCESS;
  }
  bool skip_child_incomplete_container() {
    TEST_START();
    auto check = [](const padded_string &json, simdjson::error_code expected) {
      ondemand::parser parser;
      ondemand::document doc;
      ASSERT_SUCCESS(parser.iterate(json).get(doc));
      ASSERT_ERROR(doc.raw_json().error(), expected);
      return true;
    };
    SUBTEST("unclosed array", check("["_padded, INCOMPLETE_ARRAY_OR_OBJECT));
    SUBTEST("unclosed object", check("{"_padded, INCOMPLETE_ARRAY_OR_OBJECT));
    SUBTEST("unclosed nested array", check("[[0]"_padded, INCOMPLETE_ARRAY_OR_OBJECT));
    SUBTEST("unclosed nested object", check("{\"a\":{}"_padded, INCOMPLETE_ARRAY_OR_OBJECT));
    SUBTEST("closed array", check("[]"_padded, SUCCESS));
    SUBTEST("closed object", check("{}"_padded, SUCCESS));
    SUBTEST("closed nested array", check("[[0]]"_padded, SUCCESS));
    SUBTEST("closed nested object", check("{\"a\":{}}"_padded, SUCCESS));
    TEST_SUCCEED();
  }
  bool empty_document_error() {
    TEST_START();
    ondemand::parser parser;
    auto json = ""_padded;
    ASSERT_ERROR( parser.iterate(json), EMPTY );
    TEST_SUCCEED();
  }
  bool raw_json_string_error() {
    TEST_START();
    ondemand::parser parser;
    auto json = "{\"haha\":{\"df2\":3.5, \"df3\": \"fd\"}}"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(json).get(doc) );
    ondemand::raw_json_string rawjson;
    ASSERT_ERROR( doc.get_raw_json_string().get(rawjson), INCORRECT_TYPE );
    TEST_SUCCEED();
  }
#if SIMDJSON_EXCEPTIONS
  // This is a compile-only test
  struct Class {
      Class(std::string text)
          : parser()
          , doc(parser.iterate(text))
      {
      }
      simdjson::ondemand::parser parser;
      simdjson::ondemand::document doc;
  };
  bool document_in_class() {
    TEST_START();
    std::string text = "{}";
    Class c(text);
    TEST_SUCCEED();
  }
  bool direct_document() {
    TEST_START();
    std::string text = "{}";
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc(parser.iterate(text));
    TEST_SUCCEED();
  }
  bool raw_json_string_except() {
    TEST_START();
    ondemand::parser parser;
    auto json = "{\"haha\":{\"df2\":3.5, \"df3\": \"fd\"}}"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(json).get(doc) );
    try {
      ondemand::raw_json_string rawjson = doc.get_raw_json_string();
      (void)rawjson;
      TEST_FAIL("Should have thrown an exception!")
    } catch(simdjson_error& e) {
      ASSERT_ERROR(e.error(), INCORRECT_TYPE);
      TEST_SUCCEED();
    }
  }
  bool raw_json_string_except_with_io() {
    TEST_START();
    ondemand::parser parser;
    auto json = "{\"haha\":{\"df2\":3.5, \"df3\": \"fd\"}}"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(json).get(doc) );
    try {
      auto rawjson = doc.get_raw_json_string();
      std::cout << rawjson;
      TEST_FAIL("Should have thrown an exception!")
    } catch(simdjson_error& e) {
      ASSERT_ERROR(e.error(), INCORRECT_TYPE);
      TEST_SUCCEED();
    }
  }

#endif
  bool parser_max_capacity() {
    TEST_START();
    ondemand::parser parser(1); // max_capacity set to 1 byte
    padded_string json;
    ASSERT_SUCCESS( padded_string::load(TWITTER_JSON).get(json) );
    auto error = parser.iterate(json);
    ASSERT_ERROR(error,CAPACITY);
    TEST_SUCCEED();
  }

  bool parser_set_max_capacity() {
    TEST_START();
    ondemand::parser parser;
    parser.set_max_capacity(1024 * 1024);
    ASSERT_EQUAL(parser.max_capacity(), size_t(1024 * 1024));
    auto json = R"({"k":"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    TEST_SUCCEED();
  }

  // An allocate() rejected for its depth leaves the parser as it was: a fresh
  // parser stays empty and an allocated one keeps its capacity and depth.
  bool failed_allocate_then_iterate() {
    TEST_START();
    auto json = "[1,2,3]"_padded;
    auto scalar = "1"_padded;
    const size_t bad_depths[] = { 0, SIMDJSON_MAX_DEPTH + 1 };
    for (size_t bad_depth : bad_depths) {
      ondemand::parser parser;
      ASSERT_ERROR( parser.allocate(1024, bad_depth), CAPACITY );
      ASSERT_EQUAL( parser.capacity(), 0 );
      ASSERT_EQUAL( parser.max_depth(), DEFAULT_MAX_DEPTH );
      ondemand::document doc;
      ASSERT_SUCCESS( parser.iterate(json).get(doc) );
      ASSERT_SUCCESS( parser.iterate(scalar).get(doc) );
    }
    // A parser that was already allocated keeps its settings.
    for (size_t bad_depth : bad_depths) {
      ondemand::parser parser;
      ASSERT_SUCCESS( parser.allocate(1024, 16) );
      ASSERT_ERROR( parser.allocate(1024, bad_depth), CAPACITY );
      ASSERT_EQUAL( parser.capacity(), 1024 );
      ASSERT_EQUAL( parser.max_depth(), 16 );
      ondemand::document doc;
      ASSERT_SUCCESS( parser.iterate(json).get(doc) );
    }
    TEST_SUCCEED();
  }

  bool simple_error_example() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"({"bad number":3.14.1 })"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS( parser.iterate(json).get(doc) );
    double x;
    ASSERT_ERROR( doc["bad number"].get_double().get(x), NUMBER_ERROR );
    TEST_SUCCEED();
  }

  // An input that ends inside a string gets UNCLOSED_STRING, but that error
  // must not hide invalid UTF-8 or a control character before it:
  // iterate_allow_incomplete_json() goes on after UNCLOSED_STRING. A character
  // cut by the end of the input is part of the unclosed string, not an error.
  bool unclosed_string_keeps_earlier_errors() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    // Vary the length so that the end falls on each side of a 64-byte block.
    for (size_t pad = 0; pad < 130; pad++) {
      const std::string tail = "\",\"b\":\"" + std::string(pad, 'x');
      const padded_string bad_utf8("{\"a\":\"\xff\xfe" + tail);
      const padded_string overlong("{\"a\":\"\xc0\xaf" + tail);
      const padded_string control("{\"a\":\"x\ny" + tail);
      const padded_string cut("{\"a\":\"\xc3\xa9" + tail + "\xe6\x9d");
      ASSERT_ERROR( parser.iterate(bad_utf8).get(doc), UTF8_ERROR );
      ASSERT_ERROR( parser.iterate(overlong).get(doc), UTF8_ERROR );
      ASSERT_ERROR( parser.iterate(control).get(doc), UNESCAPED_CHARS );
      ASSERT_ERROR( parser.iterate(cut).get(doc), UNCLOSED_STRING );
#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
      ASSERT_ERROR( parser.iterate_allow_incomplete_json(bad_utf8).get(doc), UTF8_ERROR );
      ASSERT_ERROR( parser.iterate_allow_incomplete_json(overlong).get(doc), UTF8_ERROR );
      ASSERT_ERROR( parser.iterate_allow_incomplete_json(control).get(doc), UNESCAPED_CHARS );
      ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(cut).get(doc) );
      std::string_view a;
      ASSERT_SUCCESS( doc["a"].get_string().get(a) );
      ASSERT_EQUAL( a, "\xc3\xa9" );
#endif
    }
    TEST_SUCCEED();
  }

#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
  // An incomplete document may end with a key, possibly cut before its closing
  // quote. The byte past the input must not be taken for the colon: the key
  // would become a field whose key runs past the input.
  bool incomplete_json_ending_with_key() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ondemand::object obj;
    std::string_view key;
    auto cut_key = R"({"a":1,"bc)"_padded;
    cut_key.data()[cut_key.size()] = ':';
    ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(cut_key).get(doc) );
    ASSERT_SUCCESS( doc.get_object().get(obj) );
    size_t count = 0;
    for (auto field : obj) {
      if (count++ == 0) {
        ASSERT_SUCCESS( field.unescaped_key().get(key) );
        ASSERT_EQUAL( key, "a" );
        continue;
      }
      ASSERT_ERROR( field.error(), INCOMPLETE_ARRAY_OR_OBJECT );
      break;
    }
    ASSERT_EQUAL( count, 2 );
    ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(cut_key).get(doc) );
    ASSERT_ERROR( doc["zz"].error(), INCOMPLETE_ARRAY_OR_OBJECT );

    auto whole_key = R"({"a":1,"bc")"_padded;
    whole_key.data()[whole_key.size()] = ':';
    ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(whole_key).get(doc) );
    ASSERT_ERROR( doc["bc"].error(), INCOMPLETE_ARRAY_OR_OBJECT );
    TEST_SUCCEED();
  }
  bool incomplete_json_object_cut_before_first_key() {
    TEST_START();
    ondemand::parser parser;
    for (const char *cut : { R"({"a":1,"b":{)", R"({"a":1,"b":{"c)", R"({"a":1,"b":{"c")" }) {
      padded_string json(std::string{cut});
      ondemand::document doc;
      ondemand::object b;
      int64_t a;
      ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
      ASSERT_SUCCESS( doc["a"].get_int64().get(a) );
      ASSERT_EQUAL( a, 1 );
      if (std::string_view(cut).back() == '{') {
        ASSERT_ERROR( doc["b"].get_object().get(b), INCOMPLETE_ARRAY_OR_OBJECT );
      } else {
        ASSERT_SUCCESS( doc["b"].get_object().get(b) );
        ASSERT_ERROR( b["c"], INCOMPLETE_ARRAY_OR_OBJECT );
      }
    }
    TEST_SUCCEED();
  }
  bool incomplete_json_cut_containers_at_sentinel() {
    TEST_START();
    ondemand::parser parser;
    const char sentinels[] = { '}', ']', ',', ':', '"', '{', '[', '1', '\0' };

    for (char sentinel : sentinels) {
      auto make_cut = [sentinel](const char *text) {
        padded_string json(std::string{text});
        json.data()[json.size()] = sentinel;
        return json;
      };

      // Root object cut before its first key: {
      {
        auto json = make_cut(R"({)");
        ondemand::document doc;
        ondemand::object obj;
        ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
        ASSERT_ERROR( doc.get_object().get(obj), INCOMPLETE_ARRAY_OR_OBJECT );
      }

      // Root array cut before its first element: [
      {
        auto json = make_cut(R"([)");
        ondemand::document doc;
        ondemand::array arr;
        ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
        ASSERT_ERROR( doc.get_array().get(arr), INCOMPLETE_ARRAY_OR_OBJECT );
      }

      // Nested array cut before its first element: {"a":[
      {
        auto json = make_cut(R"({"a":[)");
        ondemand::document doc;
        ondemand::array arr;
        ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
        ASSERT_ERROR( doc["a"].get_array().get(arr), INCOMPLETE_ARRAY_OR_OBJECT );
        ASSERT_ERROR( doc["b"].error(), INCOMPLETE_ARRAY_OR_OBJECT );
      }

      // Nested object cut before its first key: {"a":{
      {
        auto json = make_cut(R"({"a":{)");
        ondemand::document doc;
        ondemand::object obj;
        ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
        ASSERT_ERROR( doc["a"].get_object().get(obj), INCOMPLETE_ARRAY_OR_OBJECT );
        ASSERT_ERROR( doc["b"].error(), INCOMPLETE_ARRAY_OR_OBJECT );
      }

      // Nested object cut inside its first key then skipped by parent lookup: {"a":{"c
      {
        auto json = make_cut(R"({"a":{"c)");
        ondemand::document doc;
        ondemand::object a;
        ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
        ASSERT_SUCCESS( doc["a"].get_object().get(a) );
        ASSERT_ERROR( doc["b"].error(), INCOMPLETE_ARRAY_OR_OBJECT );
      }

      // Object field cut immediately after colon: {"a":
      {
        auto json = make_cut(R"({"a":)");
        ondemand::document doc;
        int64_t val = 0;
        ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
        ASSERT_ERROR( doc["a"].get_int64().get(val), INCOMPLETE_ARRAY_OR_OBJECT );
      }
      {
        auto json = make_cut(R"({"a":)");
        ondemand::document doc;
        ondemand::json_type t;
        ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
        ASSERT_ERROR( doc["a"].type().get(t), INCOMPLETE_ARRAY_OR_OBJECT );
      }
#if SIMDJSON_SUPPORTS_CONCEPTS
      {
        auto json = make_cut(R"({"a":)");
        ondemand::document doc;
        ondemand::object obj;
        bool called = false;
        ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
        ASSERT_SUCCESS( doc.get_object().get(obj) );
        auto res = obj.for_each<"a">([&](ondemand::value) { called = true; });
        ASSERT_ERROR( res.error, INCOMPLETE_ARRAY_OR_OBJECT );
        ASSERT_FALSE( called );
      }
      {
        auto json = make_cut(R"({"a":{"key_exceeding_window_for_length_path_0123456789)");
        ondemand::document doc;
        ondemand::object a;
        bool called = false;
        ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
        ASSERT_SUCCESS( doc["a"].get_object().get(a) );
        auto res = a.for_each<"key_exceeding_window_for_length_path_0123456789">(
            [&](ondemand::value) { called = true; });
        ASSERT_ERROR( res.error, INCOMPLETE_ARRAY_OR_OBJECT );
        ASSERT_FALSE( called );
      }
#endif

      // Object cut after a value or after a trailing comma: {"a":1  and {"a":1,
      for (const char *cut : { R"({"a":1 )", R"({"a":1,)" }) {
        {
          auto json = make_cut(cut);
          ondemand::document doc;
          int64_t val = 0;
          ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
          ASSERT_SUCCESS( doc["a"].get_int64().get(val) );
          ASSERT_EQUAL( val, 1 );
          ASSERT_ERROR( doc["b"].error(), INCOMPLETE_ARRAY_OR_OBJECT );
        }
        {
          auto json = make_cut(cut);
          ondemand::document doc;
          ondemand::object obj;
          size_t count = 0;
          ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
          ASSERT_SUCCESS( doc.get_object().get(obj) );
          ASSERT_ERROR( obj.count_fields().get(count), INCOMPLETE_ARRAY_OR_OBJECT );
        }
        {
          auto json = make_cut(cut);
          ondemand::document doc;
          ondemand::object obj;
          int seen = 0;
          simdjson::error_code err = SUCCESS;
          ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
          ASSERT_SUCCESS( doc.get_object().get(obj) );
          for (auto field : obj) {
            if (field.error()) {
              err = field.error();
              break;
            }
            int64_t val = 0;
            ASSERT_SUCCESS( field.value().get_int64().get(val) );
            ASSERT_EQUAL( val, 1 );
            seen++;
          }
          ASSERT_EQUAL( seen, 1 );
          ASSERT_ERROR( err, INCOMPLETE_ARRAY_OR_OBJECT );
        }
      }

      // Array cut after an element or after a trailing comma: [1  and [1,
      for (const char *cut : { R"([1 )", R"([1,)" }) {
        {
          auto json = make_cut(cut);
          ondemand::document doc;
          ondemand::array arr;
          int seen = 0;
          simdjson::error_code err = SUCCESS;
          ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
          ASSERT_SUCCESS( doc.get_array().get(arr) );
          for (auto elem : arr) {
            if (elem.error()) {
              err = elem.error();
              break;
            }
            int64_t val = 0;
            ASSERT_SUCCESS( elem.get_int64().get(val) );
            ASSERT_EQUAL( val, 1 );
            seen++;
          }
          ASSERT_EQUAL( seen, 1 );
          ASSERT_ERROR( err, INCOMPLETE_ARRAY_OR_OBJECT );
        }
        {
          auto json = make_cut(cut);
          ondemand::document doc;
          ondemand::array arr;
          size_t count = 0;
          ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
          ASSERT_SUCCESS( doc.get_array().get(arr) );
          ASSERT_ERROR( arr.count_elements().get(count), INCOMPLETE_ARRAY_OR_OBJECT );
        }
      }

      // Skipping an incomplete nested container before checking a later field
      {
        auto json = make_cut(R"({"a":{"x":1 )");
        ondemand::document doc;
        ondemand::object a;
        int64_t x = 0;
        ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
        ASSERT_SUCCESS( doc["a"].get_object().get(a) );
        ASSERT_SUCCESS( a["x"].get_int64().get(x) );
        ASSERT_EQUAL( x, 1 );
        ASSERT_ERROR( doc["b"].error(), INCOMPLETE_ARRAY_OR_OBJECT );
      }
      {
        auto json = make_cut(R"({"a":[1 )");
        ondemand::document doc;
        ondemand::array a;
        int64_t x = 0;
        ASSERT_SUCCESS( parser.iterate_allow_incomplete_json(json).get(doc) );
        ASSERT_SUCCESS( doc["a"].get_array().get(a) );
        auto it = a.begin();
        ASSERT_SUCCESS( (*it).get_int64().get(x) );
        ASSERT_EQUAL( x, 1 );
        ASSERT_ERROR( doc["b"].error(), INCOMPLETE_ARRAY_OR_OBJECT );
      }
    }
    TEST_SUCCEED();
  }
#endif

#if SIMDJSON_EXCEPTIONS
  bool simple_error_example_except() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"({"bad number":3.14.1 })"_padded;
    try {
      ondemand::document doc = parser.iterate(json);
      double x = doc["bad number"].get_double();
      TEST_FAIL("should throw got: "+std::to_string(x))
    } catch(simdjson_error& e) {
      ASSERT_ERROR(e.error(), NUMBER_ERROR);
    }
    TEST_SUCCEED();
  }
#endif

  bool get_fail_then_succeed_bool() {
    TEST_START();
    auto json = R"({ "val" : true })"_padded;
    SUBTEST("simdjson_result<ondemand::value>", test_ondemand_doc(json, [&](auto doc) {
      simdjson_result<ondemand::value> val = doc["val"];
      // Get everything that can fail in both forward and backwards order
      bool is_null_value;
      ASSERT_SUCCESS( val.is_null().get(is_null_value) );
      ASSERT_EQUAL( is_null_value, false );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_SUCCESS( val.get_bool() );
      ASSERT_SUCCESS( val.is_null().get(is_null_value) );
      ASSERT_EQUAL( is_null_value, false );
      TEST_SUCCEED();
    }));
    SUBTEST("ondemand::value", test_ondemand_doc(json, [&](auto doc) {
      ondemand::value val;
      ASSERT_SUCCESS( doc["val"].get(val) );
      // Get everything that can fail in both forward and backwards order
      bool is_null_value;
      ASSERT_SUCCESS( val.is_null().get(is_null_value) );
      ASSERT_EQUAL( is_null_value, false );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_SUCCESS( val.get_bool() );
      ASSERT_SUCCESS( val.is_null().get(is_null_value) );
      ASSERT_EQUAL( is_null_value, false );
      TEST_SUCCEED();
    }));
    json = R"(true)"_padded;
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](simdjson_result<ondemand::document> val) {
      // Get everything that can fail in both forward and backwards order
      bool is_null_value;
      ASSERT_SUCCESS( val.is_null().get(is_null_value) );
      ASSERT_EQUAL( is_null_value, false );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_SUCCESS( val.get_bool());
      ASSERT_SUCCESS( val.is_null().get(is_null_value) );
      ASSERT_EQUAL( is_null_value, false );
      TEST_SUCCEED();
    }));
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc) {
      ondemand::document val;
      ASSERT_SUCCESS( std::move(doc).get(val) );      // Get everything that can fail in both forward and backwards order
      bool is_null_value;
      ASSERT_SUCCESS( val.is_null().get(is_null_value) );
      ASSERT_EQUAL( is_null_value, false );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_SUCCESS( val.get_bool() );
      ASSERT_SUCCESS( val.is_null().get(is_null_value) );
      ASSERT_EQUAL( is_null_value, false );
      TEST_SUCCEED();
    }));

    TEST_SUCCEED();
  }

  bool get_fail_then_succeed_null() {
    TEST_START();
    auto json = R"({ "val" : null })"_padded;
    SUBTEST("simdjson_result<ondemand::value>", test_ondemand_doc(json, [&](auto doc) {
      simdjson_result<ondemand::value> val = doc["val"];
      // Get everything that can fail in both forward and backwards order
      ASSERT_ERROR( val.get_bool(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_bool(), INCORRECT_TYPE );
      bool is_null_value;
      ASSERT_SUCCESS( val.is_null().get(is_null_value) );
      ASSERT_EQUAL( is_null_value, true );
      TEST_SUCCEED();
    }));
    SUBTEST("ondemand::value", test_ondemand_doc(json, [&](auto doc) {
      ondemand::value val;
      ASSERT_SUCCESS( doc["val"].get(val) );
      // Get everything that can fail in both forward and backwards order

      ASSERT_ERROR( val.get_bool(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_bool(), INCORRECT_TYPE );
      bool is_null_value;
      ASSERT_SUCCESS( val.is_null().get(is_null_value) );
      ASSERT_EQUAL( is_null_value, true );
      TEST_SUCCEED();
    }));
    json = R"(null)"_padded;
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](simdjson_result<ondemand::document> val) {
      // Get everything that can fail in both forward and backwards order
      ASSERT_ERROR( val.get_bool(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_bool(), INCORRECT_TYPE );
      bool is_null_value;
      ASSERT_SUCCESS( val.is_null().get(is_null_value) );
      ASSERT_EQUAL( is_null_value, true );
      TEST_SUCCEED();
    }));
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc) {
      ondemand::document val;
      ASSERT_SUCCESS( std::move(doc).get(val) );      // Get everything that can fail in both forward and backwards order
      ASSERT_ERROR( val.get_bool(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint32(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_int64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_uint64(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_double(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_string(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_array(), INCORRECT_TYPE );
      ASSERT_ERROR( val.get_bool(), INCORRECT_TYPE );
      bool is_null_value;
      ASSERT_SUCCESS( val.is_null().get(is_null_value) );
      ASSERT_EQUAL( is_null_value, true );
      TEST_SUCCEED();
    }));
    TEST_SUCCEED();
  }
#if SIMDJSON_EXCEPTIONS
  bool invalid_type() {
    TEST_START();
    ONDEMAND_SUBTEST("]", "]", doc.type() == ondemand::json_type::unknown);
    ONDEMAND_SUBTEST("}", "}", doc.type() == ondemand::json_type::unknown);
    ONDEMAND_SUBTEST(":", ":", doc.type() == ondemand::json_type::unknown);
    ONDEMAND_SUBTEST(",", ",", doc.type() == ondemand::json_type::unknown);
    ONDEMAND_SUBTEST("+", "+", doc.type() == ondemand::json_type::unknown);
    TEST_SUCCEED();
  }
#endif // SIMDJSON_EXCEPTIONS

#if SIMDJSON_EXCEPTIONS
  bool operator_tests() {
    TEST_START();
    ondemand::parser parser;

    // Test successful case
    auto json = R"({ "key": "value", "number": 42 })"_padded;
    parser.iterate(json).value(); // Test operator*
    *parser.iterate(json); // Test operator*
    auto doc_result = parser.iterate(json);
    ASSERT_SUCCESS(doc_result.error());

    ASSERT_TRUE(doc_result.has_value());
    double x = doc_result->find_field("number").get<double>();
    ASSERT_EQUAL(x, 42.0);
    TEST_SUCCEED();
  }
#endif // SIMDJSON_EXCEPTIONS

  bool run() {
    return
           issue2120() &&
           badbadjson() &&
           badbadjson2() &&
           issue1834() &&
           issue1834_2() &&
           skip_child_incomplete_container() &&
#if SIMDJSON_EXCEPTIONS
           document_in_class() &&
           direct_document() &&
           raw_json_string_except() &&
           raw_json_string_except_with_io() &&
#endif
           raw_json_string_error() &&
           empty_document_error() &&
           parser_max_capacity() &&
           parser_set_max_capacity() &&
           failed_allocate_then_iterate() &&
           get_fail_then_succeed_bool() &&
           get_fail_then_succeed_null() &&
           simple_error_example() &&
           unclosed_string_keeps_earlier_errors() &&
#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
           incomplete_json_ending_with_key() &&
           incomplete_json_object_cut_before_first_key() &&
           incomplete_json_cut_containers_at_sentinel() &&
#endif
#if SIMDJSON_EXCEPTIONS
           invalid_type() &&
           simple_error_example_except() &&
           operator_tests() &&
#endif
           true;
  }
}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, error_tests::run);
}
