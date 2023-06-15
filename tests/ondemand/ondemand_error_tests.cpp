#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace error_tests {
  using namespace std;

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
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
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
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
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
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
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
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
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
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
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
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
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
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
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
      ASSERT_ERROR( val.get_object(), INCORRECT_TYPE );
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

  bool invalid_type() {
    TEST_START();
    ONDEMAND_SUBTEST("]", "]", doc.type().error() == TAPE_ERROR);
    ONDEMAND_SUBTEST("}", "}", doc.type().error() == TAPE_ERROR);
    ONDEMAND_SUBTEST(":", ":", doc.type().error() == TAPE_ERROR);
    ONDEMAND_SUBTEST(",", ",", doc.type().error() == TAPE_ERROR);
    ONDEMAND_SUBTEST("+", "+", doc.type().error() == TAPE_ERROR);
    TEST_SUCCEED();
  }

  bool run() {
    return
           badbadjson() &&
           badbadjson2() &&
           issue1834() &&
           issue1834_2() &&
#if SIMDJSON_EXCEPTIONS
           raw_json_string_except() &&
           raw_json_string_except_with_io() &&
#endif
           raw_json_string_error() &&
           empty_document_error() &&
           parser_max_capacity() &&
           get_fail_then_succeed_bool() &&
           get_fail_then_succeed_null() &&
           invalid_type() &&
           simple_error_example() &&
#if SIMDJSON_EXCEPTIONS
           simple_error_example_except() &&
#endif
           true;
  }
}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, error_tests::run);
}
