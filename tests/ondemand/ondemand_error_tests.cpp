#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace error_tests {
  using namespace std;

  bool empty_document_error() {
    TEST_START();
    ondemand::parser parser;
    auto json = ""_padded;
    ASSERT_ERROR( parser.iterate(json), EMPTY );
    TEST_SUCCEED();
  }

  bool get_fail_then_succeed_bool() {
    TEST_START();
    auto json = R"({ "val" : true })"_padded;
    SUBTEST("simdjson_result<ondemand::value>", test_ondemand_doc(json, [&](auto doc) {
      simdjson_result<ondemand::value> val = doc["val"];
      // Get everything that can fail in both forward and backwards order
      ASSERT_EQUAL( val.is_null(), false );
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
      ASSERT_EQUAL( val.is_null(), false );
      ASSERT_SUCCESS( val.get_bool() );
      TEST_SUCCEED();
    }));
    SUBTEST("ondemand::value", test_ondemand_doc(json, [&](auto doc) {
      ondemand::value val;
      ASSERT_SUCCESS( doc["val"].get(val) );
      // Get everything that can fail in both forward and backwards order
      ASSERT_EQUAL( val.is_null(), false );
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
      ASSERT_EQUAL( val.is_null(), false );
      ASSERT_SUCCESS( val.get_bool() );
      TEST_SUCCEED();
    }));
    json = R"(true)"_padded;
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](simdjson_result<ondemand::document> val) {
      // Get everything that can fail in both forward and backwards order
      ASSERT_EQUAL( val.is_null(), false );
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
      ASSERT_EQUAL( val.is_null(), false );
      ASSERT_SUCCESS( val.get_bool());
      TEST_SUCCEED();
    }));
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc) {
      ondemand::document val;
      ASSERT_SUCCESS( std::move(doc).get(val) );      // Get everything that can fail in both forward and backwards order
      ASSERT_EQUAL( val.is_null(), false );
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
      ASSERT_EQUAL( val.is_null(), false );
      ASSERT_SUCCESS( val.get_bool() );
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
      ASSERT_EQUAL( val.is_null(), true );
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
      ASSERT_EQUAL( val.is_null(), true );
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
      ASSERT_EQUAL( val.is_null(), true );
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
      ASSERT_EQUAL( val.is_null(), true );
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
           empty_document_error() &&
           get_fail_then_succeed_bool() &&
           get_fail_then_succeed_null() &&
           invalid_type() &&
           true;
  }
}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, error_tests::run);
}
