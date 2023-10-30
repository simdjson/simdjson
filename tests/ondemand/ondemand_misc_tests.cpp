#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace misc_tests {
  using namespace std;
  bool issue1981_success() {
    auto error_phrase = R"(false)"_padded;
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(error_phrase).get(doc));
    bool b;
    ASSERT_SUCCESS(doc.get_bool().get(b));
    ASSERT_FALSE(b);
    TEST_SUCCEED();
  }
  bool skipbom() {
    auto error_phrase = "\xEF\xBB\xBF false"_padded;
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(error_phrase).get(doc));
    bool b;
    ASSERT_SUCCESS(doc.get_bool().get(b));
    ASSERT_FALSE(b);
    TEST_SUCCEED();
  }


  bool issue1981_failure() {
    auto error_phrase = R"(falseA)"_padded;
    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(error_phrase).get(doc));
    bool b;
    ASSERT_ERROR( doc.get_bool().get(b), INCORRECT_TYPE);
    TEST_SUCCEED();
  }

  bool replacement_char() {
    auto fun_phrase = R"( ["I \u2665 Unicode. Even broken \ud800 Unicode." ])"_padded;
    std::string_view expected_fun = "I \xe2\x99\xa5 Unicode. Even broken \xef\xbf\xbd Unicode.";

    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(fun_phrase).get(doc));
    ondemand::array arr;
    ASSERT_SUCCESS( doc.get_array().get(arr));
    std::string_view view;
    ASSERT_SUCCESS( arr.at(0).get_string(true).get(view));
    ASSERT_EQUAL(view, expected_fun);
    TEST_SUCCEED();
  }

  bool wobbly_tests() {
    auto lone_surrogate = R"( "\ud800" )"_padded;
    std::string_view expected_lone = "\xed\xa0\x80";


    auto fun_phrase = R"( ["I \u2665 Unicode. Even broken \ud800 Unicode." ])"_padded;
    std::string_view expected_fun = "I \xe2\x99\xa5 Unicode. Even broken \xed\xa0\x80 Unicode.";

    auto insane_url = R"({"input": "http://example.com/\uDC00\uD834\uDF06\uDC00"} )"_padded;
    std::string_view expected_insane = "http://example.com/\xED\xB0\x80\xF0\x9D\x8C\x86\xED\xB0\x80";

    TEST_START();
    ondemand::parser parser;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(lone_surrogate).get(doc));
    std::string_view view;
    ASSERT_SUCCESS( doc.get_wobbly_string().get(view));
    ASSERT_EQUAL(view, expected_lone);

    ASSERT_SUCCESS(parser.iterate(fun_phrase).get(doc));
    ondemand::array arr;
    ASSERT_SUCCESS( doc.get_array().get(arr));
    ASSERT_SUCCESS( arr.at(0).get_wobbly_string().get(view));
    ASSERT_EQUAL(view, expected_fun);

    ASSERT_SUCCESS(parser.iterate(insane_url).get(doc));
    ondemand::object obj;
    ASSERT_SUCCESS( doc.get_object().get(obj));
    ASSERT_SUCCESS( obj["input"].get_wobbly_string().get(view));

    ASSERT_EQUAL(view, expected_insane);
    TEST_SUCCEED();
  }

  bool test_get_value() {
    TEST_START();
    ondemand::parser parser;
    padded_string json = R"({"a":[[1,null,3.0],["a","b",true],[10000000000,2,3]]})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::value val;
    ASSERT_SUCCESS(doc.get_value().get(val));
    ondemand::object obj;
    ASSERT_SUCCESS(val.get_object().get(obj));
    ondemand::array arr;
    ASSERT_SUCCESS(obj["a"].get_array().get(arr));
     size_t count{};
    ASSERT_SUCCESS(arr.count_elements().get(count));
    ASSERT_EQUAL(3,count);
    TEST_SUCCEED();
  }

  bool issue1661a() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata = R"({"":],"global-groups":[[]}})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ondemand::value global_groups;
    ASSERT_SUCCESS(doc["global-groups"].get(global_groups));
    ondemand::json_type global_type;
    ASSERT_SUCCESS(global_groups.type().get(global_type));
    ASSERT_EQUAL(global_type, ondemand::json_type::array);
    TEST_SUCCEED();
  }

  bool doc_ref() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata = R"({"":2,"v":[]})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ondemand::document_reference ref(doc);
    ondemand::value arr;
    ASSERT_SUCCESS(ref["v"].get(arr));
    ondemand::json_type global_type;
    ASSERT_SUCCESS(arr.type().get(global_type));
    ASSERT_EQUAL(global_type, ondemand::json_type::array);
    TEST_SUCCEED();
  }

  bool issue_uffff() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"(   "wow:\uFFFF"   )"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::string_view view;
    ASSERT_SUCCESS( doc.get_string().get(view));
    ASSERT_EQUAL(view, "wow:\xef\xbf\xbf");
    TEST_SUCCEED();
  }

  bool issue_backslash() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"(   "sc:\\./"   )"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::string_view view;
    ASSERT_SUCCESS( doc.get_string().get(view));
    ASSERT_EQUAL(view, "sc:\\./");
    TEST_SUCCEED();
  }

  bool issue1870() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"("\uDC00")"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::string_view view;
    ASSERT_ERROR( doc.get_string().get(view), STRING_ERROR );
    TEST_SUCCEED();
  }

  // Test a surrogate pair with the low surrogate out of range
  bool issue1894() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"("\uD888\u1234")"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::string_view view;
    ASSERT_ERROR(doc.get_string().get(view), STRING_ERROR);
    TEST_SUCCEED();
  }

  bool issue1894toolarge() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"("\uD888\uE000")"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::string_view view;
    ASSERT_ERROR(doc.get_string().get(view), STRING_ERROR);
    TEST_SUCCEED();
  }

  // Test the smallest surrogate pair, largest surrogate pair, and a surrogate pair in range.
  bool issue1894success() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"("\uD888\uDC00\uD800\uDC00\uDBFF\uDFFF")"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::string_view view;
    ASSERT_SUCCESS(doc.get_string().get(view));
	ASSERT_EQUAL(view, "\xf0\xb2\x80\x80\xf0\x90\x80\x80\xf4\x8f\xbf\xbf");
    TEST_SUCCEED();
  }

  bool issue1660() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata =  R"({"globals":{"a":{"shadowable":[}}}})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ondemand::object globals;
    ASSERT_SUCCESS(doc["globals"].get(globals));
    for (auto global_field : globals) {
      ondemand::value global;
      ASSERT_SUCCESS(global_field.value().get(global));
      ondemand::json_type global_type;
      ASSERT_SUCCESS(global.type().get(global_type));
      ASSERT_EQUAL(global_type, ondemand::json_type::object);
      ondemand::object global_object;
      ASSERT_SUCCESS(global.get(global_object));
      ondemand::value shadowable;
      ASSERT_SUCCESS(global_object["shadowable"].get(shadowable));
      ASSERT_ERROR(shadowable.get_object(), INCORRECT_TYPE);
      ondemand::value badvalue;
      auto error = global_object["writable"].get(badvalue);
      if(error == SUCCESS) {
        return false;
      } else {
        break;
      }
    }
    TEST_SUCCEED();
  }


  bool issue1660_with_bool() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata =  R"({"globals":{"a":{"shadowable":[}}}})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ondemand::object globals;
    ASSERT_SUCCESS(doc["globals"].get(globals));
    for (auto global_field : globals) {
      ondemand::value global;
      ASSERT_SUCCESS(global_field.value().get(global));
      ondemand::json_type global_type;
      ASSERT_SUCCESS(global.type().get(global_type));
      ASSERT_EQUAL(global_type, ondemand::json_type::object);
      ondemand::object global_object;
      ASSERT_SUCCESS(global.get(global_object));
      ondemand::value shadowable;
      ASSERT_SUCCESS(global_object["shadowable"].get(shadowable));
      ASSERT_ERROR(shadowable.get_bool(), INCORRECT_TYPE);
      ondemand::value badvalue;
      auto error = global_object["writable"].get(badvalue);
      if(error == SUCCESS) {
        return false;
      } else {
        break;
      }
    }
    TEST_SUCCEED();
  }


  bool issue1660_with_uint64() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata =  R"({"globals":{"a":{"shadowable":[}}}})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ondemand::object globals;
    ASSERT_SUCCESS(doc["globals"].get(globals));
    for (auto global_field : globals) {
      ondemand::value global;
      ASSERT_SUCCESS(global_field.value().get(global));
      ondemand::json_type global_type;
      ASSERT_SUCCESS(global.type().get(global_type));
      ASSERT_EQUAL(global_type, ondemand::json_type::object);
      ondemand::object global_object;
      ASSERT_SUCCESS(global.get(global_object));
      ondemand::value shadowable;
      ASSERT_SUCCESS(global_object["shadowable"].get(shadowable));
      ASSERT_ERROR(shadowable.get_uint64(), INCORRECT_TYPE);
      ondemand::value badvalue;
      auto error = global_object["writable"].get(badvalue);
      if(error == SUCCESS) {
        return false;
      } else {
        break;
      }
    }
    TEST_SUCCEED();
  }


  bool issue1660_with_int64() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata =  R"({"globals":{"a":{"shadowable":[}}}})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ondemand::object globals;
    ASSERT_SUCCESS(doc["globals"].get(globals));
    for (auto global_field : globals) {
      ondemand::value global;
      ASSERT_SUCCESS(global_field.value().get(global));
      ondemand::json_type global_type;
      ASSERT_SUCCESS(global.type().get(global_type));
      ASSERT_EQUAL(global_type, ondemand::json_type::object);
      ondemand::object global_object;
      ASSERT_SUCCESS(global.get(global_object));
      ondemand::value shadowable;
      ASSERT_SUCCESS(global_object["shadowable"].get(shadowable));
      ASSERT_ERROR(shadowable.get_int64(), INCORRECT_TYPE);
      ondemand::value badvalue;
      auto error = global_object["writable"].get(badvalue);
      if(error == SUCCESS) {
        return false;
      } else {
        break;
      }
    }
    TEST_SUCCEED();
  }

  bool issue1660_with_double() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata =  R"({"globals":{"a":{"shadowable":[}}}})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ondemand::object globals;
    ASSERT_SUCCESS(doc["globals"].get(globals));
    for (auto global_field : globals) {
      ondemand::value global;
      ASSERT_SUCCESS(global_field.value().get(global));
      ondemand::json_type global_type;
      ASSERT_SUCCESS(global.type().get(global_type));
      ASSERT_EQUAL(global_type, ondemand::json_type::object);
      ondemand::object global_object;
      ASSERT_SUCCESS(global.get(global_object));
      ondemand::value shadowable;
      ASSERT_SUCCESS(global_object["shadowable"].get(shadowable));
      ASSERT_ERROR(shadowable.get_double(), INCORRECT_TYPE);
      ondemand::value badvalue;
      auto error = global_object["writable"].get(badvalue);
      if(error == SUCCESS) {
        return false;
      } else {
        break;
      }
    }
    TEST_SUCCEED();
  }


  bool issue1660_with_null() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata =  R"({"globals":{"a":{"shadowable":[}}}})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ondemand::object globals;
    ASSERT_SUCCESS(doc["globals"].get(globals));
    for (auto global_field : globals) {
      ondemand::value global;
      ASSERT_SUCCESS(global_field.value().get(global));
      ondemand::json_type global_type;
      ASSERT_SUCCESS(global.type().get(global_type));
      ASSERT_EQUAL(global_type, ondemand::json_type::object);
      ondemand::object global_object;
      ASSERT_SUCCESS(global.get(global_object));
      ondemand::value shadowable;
      ASSERT_SUCCESS(global_object["shadowable"].get(shadowable));
      bool is_null_value;
      ASSERT_SUCCESS(shadowable.is_null().get(is_null_value));
      ASSERT_TRUE(!is_null_value);
      ondemand::value badvalue;
      auto error = global_object["writable"].get(badvalue);
      if(error == SUCCESS) {
        return false;
      } else {
        break;
      }
    }
    TEST_SUCCEED();
  }


  bool issue1660_with_string() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata =  R"({"globals":{"a":{"shadowable":[}}}})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ondemand::object globals;
    ASSERT_SUCCESS(doc["globals"].get(globals));
    for (auto global_field : globals) {
      ondemand::value global;
      ASSERT_SUCCESS(global_field.value().get(global));
      ondemand::json_type global_type;
      ASSERT_SUCCESS(global.type().get(global_type));
      ASSERT_EQUAL(global_type, ondemand::json_type::object);
      ondemand::object global_object;
      ASSERT_SUCCESS(global.get(global_object));
      ondemand::value shadowable;
      ASSERT_SUCCESS(global_object["shadowable"].get(shadowable));
      ASSERT_ERROR(shadowable.get_string(), INCORRECT_TYPE);
      ondemand::value badvalue;
      auto error = global_object["writable"].get(badvalue);
      if(error == SUCCESS) {
        return false;
      } else {
        break;
      }
    }
    TEST_SUCCEED();
  }
  bool is_alive_root_array() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata = R"([1,2,3)"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_TRUE(doc.is_alive());
    size_t count{0};
    for(simdjson_unused simdjson_result<ondemand::value> val : doc) {
      ASSERT_ERROR(val.error(), INCOMPLETE_ARRAY_OR_OBJECT);
      count++;
    }
    ASSERT_EQUAL(count, 1);
    ASSERT_FALSE(doc.is_alive());
    TEST_SUCCEED();
  }

  bool is_alive_array() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata = R"({"a":[1,2,3})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_TRUE(doc.is_alive());
    ondemand::object obj;
    ASSERT_SUCCESS(doc.get_object().get(obj));
    ondemand::array internal_arr;
    ASSERT_SUCCESS(obj["a"].get_array().get(internal_arr));
    size_t count{0};
    for(simdjson_unused simdjson_result<ondemand::value> val : internal_arr) {
      if(count < 3) {
        ASSERT_SUCCESS(val.error());
        ASSERT_TRUE(doc.is_alive());
      } else {
        ASSERT_ERROR(val.error(), TAPE_ERROR);
        ASSERT_FALSE(doc.is_alive());
      }
      count++;
    }
    ASSERT_EQUAL(count, 4);
    ASSERT_FALSE(doc.is_alive());
    TEST_SUCCEED();
  }

  bool is_alive_root_object() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata = R"({"a":1)"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_TRUE(doc.is_alive());
    ondemand::object obj;
    ASSERT_ERROR(doc.get_object().get(obj), INCOMPLETE_ARRAY_OR_OBJECT);
    ASSERT_FALSE(doc.is_alive());
    TEST_SUCCEED();
  }

  bool is_alive_object() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata = R"([{"a":1])"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ASSERT_TRUE(doc.is_alive());
    ondemand::object obj;
    ASSERT_SUCCESS(doc.at(0).get_object().get(obj));
    ASSERT_TRUE(doc.is_alive());
    size_t count{0};
    for(simdjson_unused simdjson_result<ondemand::field> val : obj) {
      if(count == 0) {
        ASSERT_SUCCESS(val.error());
        ASSERT_TRUE(doc.is_alive());
      }
      if(count == 1) {
        ASSERT_ERROR(val.error(), TAPE_ERROR);
        ASSERT_FALSE(doc.is_alive());
      }
      count++;
    }
    ASSERT_EQUAL(count, 2);
    ASSERT_FALSE(doc.is_alive());
    TEST_SUCCEED();
  }
  bool issue1661() {
    TEST_START();
    ondemand::parser parser;
    padded_string docdata = R"({"":],"global-groups":[[]}})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    ondemand::object global_groups;
    ASSERT_ERROR(doc["global-groups"].get(global_groups), INCORRECT_TYPE);
    ondemand::object globals;
    auto error = doc["globals"].get(globals);
    if(error == SUCCESS) { return false; }
    TEST_SUCCEED();
  }

  simdjson_warn_unused bool big_integer() {
    TEST_START();
    simdjson::ondemand::parser parser;
    simdjson::padded_string docdata =  R"({"value":12321323213213213213213213213211223})"_padded;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    simdjson::ondemand::object o;
    ASSERT_SUCCESS(doc.get_object().get(o));
    string_view token;
    ASSERT_SUCCESS(o["value"].raw_json_token().get(token));
    ASSERT_EQUAL(token, "12321323213213213213213213213211223");
    return true;
  }
  simdjson_warn_unused bool big_integer_in_string() {
    TEST_START();
    simdjson::ondemand::parser parser;
    simdjson::padded_string docdata =  R"({"value":"12321323213213213213213213213211223"})"_padded;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(docdata).get(doc));
    simdjson::ondemand::object o;
    ASSERT_SUCCESS(doc.get_object().get(o));
    string_view token;
    ASSERT_SUCCESS(o["value"].raw_json_token().get(token));
    ASSERT_EQUAL(token, "\"12321323213213213213213213213211223\"");
    return true;
  }
  simdjson_warn_unused bool test_raw_json_token(string_view json, string_view expected_token, int expected_start_index = 0) {
    string title("'");
    title.append(json.data(), json.length());
    title += std::string("'");
    padded_string json_padded = json;
    SUBTEST(title, test_ondemand_doc(json_padded, [&](auto doc) {
      string_view token;
      ASSERT_SUCCESS( doc.raw_json_token().get(token) );
      ASSERT_EQUAL( token, expected_token );
      // Validate the text is inside the original buffer
      ASSERT_EQUAL( reinterpret_cast<const void*>(token.data()), reinterpret_cast<const void*>(&json_padded.data()[expected_start_index]));
      return true;
    }));

    // Test values
    auto json_in_hash = string(R"({"a":)");
    json_in_hash.append(json.data(), json.length());
    json_in_hash += std::string("}");
    json_padded = json_in_hash;
    title = std::string("'");
    title.append(json_in_hash.data(), json_in_hash.length());
    title += std::string("'");
    SUBTEST(title, test_ondemand_doc(json_padded, [&](auto doc) {
      string_view token;
      ASSERT_SUCCESS( doc["a"].raw_json_token().get(token) );
      ASSERT_EQUAL( token, expected_token );
      // Validate the text is inside the original buffer
      // Adjust for the {"a":
      ASSERT_EQUAL( reinterpret_cast<const void*>(token.data()), reinterpret_cast<const void*>(&json_padded.data()[5+expected_start_index]));
      return true;
    }));

    return true;
  }

  bool raw_json_token() {
    TEST_START();
    return
      test_raw_json_token("{}", "{") &&
      test_raw_json_token("{ }", "{ ") &&
      test_raw_json_token("{ \n }", "{ \n ") &&
      test_raw_json_token(" \n { \n } \n ", "{ \n ", 3) &&
      test_raw_json_token("[]", "[") &&
      test_raw_json_token("1", "1") &&
      test_raw_json_token(" \n 1 \n ", "1 \n ", 3) &&
      test_raw_json_token("-123.456e-789", "-123.456e-789") &&
      test_raw_json_token(" \n -123.456e-789 \n ", "-123.456e-789 \n ", 3) &&
      test_raw_json_token("true", "true") &&
      test_raw_json_token("false", "false") &&
      test_raw_json_token("null", "null") &&
      test_raw_json_token("blah2", "blah2") &&
      test_raw_json_token("true false", "true ") &&
      test_raw_json_token("true \n false", "true \n ") &&
      true;
  }

  bool run() {
    return
           skipbom() &&
           issue1981_success() &&
           issue1981_failure() &&
           replacement_char() &&
           wobbly_tests() &&
           issue_uffff() &&
           issue_backslash() &&
           issue1870() &&
           issue1894() &&
           issue1894toolarge() &&
           issue1894success() &&
           is_alive_root_array() &&
           is_alive_root_object() &&
           is_alive_array() &&
           is_alive_object() &&
           test_get_value() &&
           issue1660_with_uint64() &&
           issue1660_with_int64() &&
           issue1660_with_double() &&
           issue1660_with_null() &&
           issue1660_with_string() &&
           issue1660_with_bool() &&
           issue1661a() &&
           issue1660() &&
           issue1661() &&
           big_integer_in_string() &&
           big_integer() &&
           raw_json_token() &&
           doc_ref() &&
           true;
  }

} // namespace twitter_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, misc_tests::run);
}
