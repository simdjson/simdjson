#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace unknown_tests {
  using namespace std;

  bool root_value_type() {
    TEST_START();
    padded_string json = "NaN"_padded;
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
        simdjson::ondemand::json_type type;
        ASSERT_SUCCESS( doc_result.type().get(type) );
        ASSERT_EQUAL( type, simdjson::ondemand::json_type::unknown );
        std::string_view str;
        ASSERT_SUCCESS( doc_result.raw_json_token().get(str) );
        ASSERT_EQUAL( str, "NaN");
        return true;
    }));
    TEST_SUCCEED();
  }
  bool object_value_type() {
    TEST_START();
    padded_string json = "{\"key\": NaN}"_padded;
    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
        simdjson::ondemand::object object;
        ASSERT_SUCCESS( doc_result.get_object().get(object) );

        simdjson::ondemand::json_type type;
        auto val = object["key"];
        ASSERT_SUCCESS( val.type().get(type) );
        ASSERT_EQUAL( type, simdjson::ondemand::json_type::unknown );
        double num;
        ASSERT_EQUAL( val.get(num), simdjson::error_code::INCORRECT_TYPE);
        std::string_view str;
        ASSERT_SUCCESS( val.raw_json_token().get(str) );
        ASSERT_EQUAL( str, "NaN");
        return true;
    }));
    TEST_SUCCEED();
  }
#if SIMDJSON_EXCEPTIONS
  bool object_value_type_exception() {
    TEST_START();
    padded_string json = "{\"key\": NaN}"_padded;
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc = parser.iterate(json);
    simdjson::ondemand::object object = doc.get_object();
    simdjson::ondemand::value val = object["key"];
    simdjson::ondemand::json_type type = val.type();
    ASSERT_EQUAL( type, simdjson::ondemand::json_type::unknown);
    try {
      double num = val.get_double();
      (void)num;
    } catch (const simdjson::simdjson_error& e) {
      ASSERT_EQUAL( e.error(), simdjson::error_code::INCORRECT_TYPE);
    }
    std::string_view str = val.raw_json_token();
    ASSERT_EQUAL( str, "NaN");
    TEST_SUCCEED();
  }
#endif

  bool run() {
    return
           root_value_type() &&
           object_value_type() &&
#if SIMDJSON_EXCEPTIONS
           object_value_type_exception() &&
#endif
           true;
  }

} // namespace unknown_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, unknown_tests::run);
}
