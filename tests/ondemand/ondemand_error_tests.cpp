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

  namespace wrong_type {

#define TEST_CAST_ERROR(JSON, TYPE, ERROR) \
    std::cout << "- Subtest: get_" << (#TYPE) << "() - JSON: " << (JSON) << std::endl; \
    if (!test_ondemand_doc((JSON##_padded), [&](auto doc_result) { \
      ASSERT_ERROR( doc_result.get_##TYPE(), (ERROR) ); \
      return true; \
    })) { \
      return false; \
    } \
    { \
      padded_string a_json(std::string(R"({ "a": )") + JSON + " })"); \
      std::cout << R"(- Subtest: get_)" << (#TYPE) << "() - JSON: " << a_json << std::endl; \
      if (!test_ondemand_doc(a_json, [&](auto doc_result) { \
        ASSERT_ERROR( doc_result["a"].get_##TYPE(), (ERROR) ); \
        return true; \
      })) { \
        return false; \
      }; \
    }

    bool wrong_type_array() {
      TEST_START();
      TEST_CAST_ERROR("[]", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("[]", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("[]", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("[]", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("[]", double, NUMBER_ERROR);
      TEST_CAST_ERROR("[]", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("[]", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_object() {
      TEST_START();
      TEST_CAST_ERROR("{}", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("{}", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("{}", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("{}", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("{}", double, NUMBER_ERROR);
      TEST_CAST_ERROR("{}", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("{}", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_true() {
      TEST_START();
      TEST_CAST_ERROR("true", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("true", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("true", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("true", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("true", double, NUMBER_ERROR);
      TEST_CAST_ERROR("true", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("true", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_false() {
      TEST_START();
      TEST_CAST_ERROR("false", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("false", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("false", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("false", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("false", double, NUMBER_ERROR);
      TEST_CAST_ERROR("false", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("false", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_null() {
      TEST_START();
      TEST_CAST_ERROR("null", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("null", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("null", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("null", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("null", double, NUMBER_ERROR);
      TEST_CAST_ERROR("null", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("null", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_1() {
      TEST_START();
      TEST_CAST_ERROR("1", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("1", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("1", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("1", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("1", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_negative_1() {
      TEST_START();
      TEST_CAST_ERROR("-1", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("-1", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("-1", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("-1", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("-1", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("-1", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_float() {
      TEST_START();
      TEST_CAST_ERROR("1.1", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("1.1", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("1.1", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("1.1", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("1.1", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("1.1", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("1.1", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_negative_int64_overflow() {
      TEST_START();
      TEST_CAST_ERROR("-9223372036854775809", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("-9223372036854775809", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("-9223372036854775809", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("-9223372036854775809", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("-9223372036854775809", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("-9223372036854775809", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("-9223372036854775809", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_int64_overflow() {
      TEST_START();
      TEST_CAST_ERROR("9223372036854775808", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("9223372036854775808", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("9223372036854775808", bool, INCORRECT_TYPE);
      // TODO BUG: this should be an error but is presently not
      // TEST_CAST_ERROR("9223372036854775808", int64, NUMBER_ERROR);
      TEST_CAST_ERROR("9223372036854775808", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("9223372036854775808", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool wrong_type_uint64_overflow() {
      TEST_START();
      TEST_CAST_ERROR("18446744073709551616", array, INCORRECT_TYPE);
      TEST_CAST_ERROR("18446744073709551616", object, INCORRECT_TYPE);
      TEST_CAST_ERROR("18446744073709551616", bool, INCORRECT_TYPE);
      TEST_CAST_ERROR("18446744073709551616", int64, NUMBER_ERROR);
      // TODO BUG: this should be an error but is presently not
      // TEST_CAST_ERROR("18446744073709551616", uint64, NUMBER_ERROR);
      TEST_CAST_ERROR("18446744073709551616", string, INCORRECT_TYPE);
      TEST_CAST_ERROR("18446744073709551616", raw_json_string, INCORRECT_TYPE);
      TEST_SUCCEED();
    }

    bool run() {
      return
            wrong_type_1() &&
            wrong_type_array() &&
            wrong_type_false() &&
            wrong_type_float() &&
            wrong_type_int64_overflow() &&
            wrong_type_negative_1() &&
            wrong_type_negative_int64_overflow() &&
            wrong_type_null() &&
            wrong_type_object() &&
            wrong_type_true() &&
            wrong_type_uint64_overflow() &&
            true;
    }

  } // namespace wrong_type

  template<typename V, typename T>
  bool assert_iterate(T array, V *expected, size_t N, simdjson::error_code *expected_error, size_t N2) {
    size_t count = 0;
    for (auto elem : std::forward<T>(array)) {
      V actual;
      auto actual_error = elem.get(actual);
      if (count >= N) {
        if (count >= (N+N2)) {
          std::cerr << "FAIL: Extra error reported: " << actual_error << std::endl;
          return false;
        }
        ASSERT_ERROR(actual_error, expected_error[count - N]);
      } else {
        ASSERT_SUCCESS(actual_error);
        ASSERT_EQUAL(actual, expected[count]);
      }
      count++;
    }
    ASSERT_EQUAL(count, N+N2);
    return true;
  }

  template<typename V, size_t N, size_t N2, typename T>
  bool assert_iterate(T &array, V (&&expected)[N], simdjson::error_code (&&expected_error)[N2]) {
    return assert_iterate<V, T&>(array, expected, N, expected_error, N2);
  }

  template<size_t N2, typename T>
  bool assert_iterate(T &array, simdjson::error_code (&&expected_error)[N2]) {
    return assert_iterate<int64_t, T&>(array, nullptr, 0, expected_error, N2);
  }

  template<typename V, size_t N, typename T>
  bool assert_iterate(T &array, V (&&expected)[N]) {
    return assert_iterate<V, T&&>(array, expected, N, nullptr, 0);
  }

  template<typename V, size_t N, size_t N2, typename T>
  bool assert_iterate(T &&array, V (&&expected)[N], simdjson::error_code (&&expected_error)[N2]) {
    return assert_iterate<V, T&&>(std::forward<T>(array), expected, N, expected_error, N2);
  }

  template<size_t N2, typename T>
  bool assert_iterate(T &&array, simdjson::error_code (&&expected_error)[N2]) {
    return assert_iterate<int64_t, T&&>(std::forward<T>(array), nullptr, 0, expected_error, N2);
  }

  template<typename V, size_t N, typename T>
  bool assert_iterate(T &&array, V (&&expected)[N]) {
    return assert_iterate<V, T&&>(std::forward<T>(array), expected, N, nullptr, 0);
  }

  bool top_level_array_iterate_error() {
    TEST_START();
    ONDEMAND_SUBTEST("missing comma", "[1 1]",  assert_iterate(doc, { int64_t(1) }, { TAPE_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", "[1,,1]", assert_iterate(doc, { int64_t(1) }, { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", "[,]",    assert_iterate(doc,                 { NUMBER_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", "[,,]",   assert_iterate(doc,                 { NUMBER_ERROR, NUMBER_ERROR, TAPE_ERROR }));
    TEST_SUCCEED();
  }
  bool top_level_array_iterate_unclosed_error() {
    TEST_START();
    ONDEMAND_SUBTEST("unclosed extra comma", "[,", assert_iterate(doc,                 { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", "[1 ",    assert_iterate(doc, { int64_t(1) }, { TAPE_ERROR }));
    // TODO These pass the user values that may run past the end of the buffer if they aren't careful
    // In particular, if the padding is decorated with the wrong values, we could cause overrun!
    ONDEMAND_SUBTEST("unclosed extra comma", "[,,", assert_iterate(doc,                 { NUMBER_ERROR, NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", "[1,",    assert_iterate(doc, { int64_t(1) }, { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", "[1",     assert_iterate(doc,                 { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", "[",      assert_iterate(doc,                 { NUMBER_ERROR, TAPE_ERROR }));
    TEST_SUCCEED();
  }

  bool array_iterate_error() {
    TEST_START();
    ONDEMAND_SUBTEST("missing comma", R"({ "a": [1 1] })",  assert_iterate(doc["a"], { int64_t(1) }, { TAPE_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", R"({ "a": [1,,1] })", assert_iterate(doc["a"], { int64_t(1) }, { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", R"({ "a": [1,,] })",  assert_iterate(doc["a"], { int64_t(1) }, { NUMBER_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", R"({ "a": [,] })",    assert_iterate(doc["a"],                 { NUMBER_ERROR }));
    ONDEMAND_SUBTEST("extra comma  ", R"({ "a": [,,] })",   assert_iterate(doc["a"],                 { NUMBER_ERROR, NUMBER_ERROR, TAPE_ERROR }));
    TEST_SUCCEED();
  }
  bool array_iterate_unclosed_error() {
    TEST_START();
    ONDEMAND_SUBTEST("unclosed extra comma", R"({ "a": [,)",  assert_iterate(doc["a"],                 { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed extra comma", R"({ "a": [,,)", assert_iterate(doc["a"],                 { NUMBER_ERROR, NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", R"({ "a": [1 )",        assert_iterate(doc["a"], { int64_t(1) }, { TAPE_ERROR }));
    // TODO These pass the user values that may run past the end of the buffer if they aren't careful
    // In particular, if the padding is decorated with the wrong values, we could cause overrun!
    ONDEMAND_SUBTEST("unclosed     ", R"({ "a": [1,)",        assert_iterate(doc["a"], { int64_t(1) }, { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", R"({ "a": [1)",         assert_iterate(doc["a"],                 { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed     ", R"({ "a": [)",          assert_iterate(doc["a"],                 { NUMBER_ERROR, TAPE_ERROR }));
    TEST_SUCCEED();
  }

  template<typename V, typename T>
  bool assert_iterate_object(T &&object, const char **expected_key, V *expected, size_t N, simdjson::error_code *expected_error, size_t N2) {
    size_t count = 0;
    for (auto field : object) {
      V actual;
      auto actual_error = field.value().get(actual);
      if (count >= N) {
        ASSERT((count - N) < N2, "Extra error reported");
        ASSERT_ERROR(actual_error, expected_error[count - N]);
      } else {
        ASSERT_SUCCESS(actual_error);
        ASSERT_EQUAL(field.key().first, expected_key[count]);
        ASSERT_EQUAL(actual, expected[count]);
      }
      count++;
    }
    ASSERT_EQUAL(count, N+N2);
    return true;
  }

  template<typename V, size_t N, size_t N2, typename T>
  bool assert_iterate_object(T &&object, const char *(&&expected_key)[N], V (&&expected)[N], simdjson::error_code (&&expected_error)[N2]) {
    return assert_iterate_object<V, T>(std::forward<T>(object), expected_key, expected, N, expected_error, N2);
  }

  template<size_t N2, typename T>
  bool assert_iterate_object(T &&object, simdjson::error_code (&&expected_error)[N2]) {
    return assert_iterate_object<int64_t, T>(std::forward<T>(object), nullptr, nullptr, 0, expected_error, N2);
  }

  template<typename V, size_t N, typename T>
  bool assert_iterate_object(T &&object, const char *(&&expected_key)[N], V (&&expected)[N]) {
    return assert_iterate_object<V, T>(std::forward<T>(object), expected_key, expected, N, nullptr, 0);
  }

  bool object_iterate_error() {
    TEST_START();
    ONDEMAND_SUBTEST("missing colon", R"({ "a"  1, "b": 2 })",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("missing key  ", R"({    : 1, "b": 2 })",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("missing value", R"({ "a":  , "b": 2 })",    assert_iterate_object(doc.get_object(),                          { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("missing comma", R"({ "a": 1  "b": 2 })",    assert_iterate_object(doc.get_object(), { "a" }, { int64_t(1) }, { TAPE_ERROR }));
    TEST_SUCCEED();
  }
  bool object_iterate_wrong_key_type_error() {
    TEST_START();
    ONDEMAND_SUBTEST("wrong key type", R"({ 1:   1, "b": 2 })",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("wrong key type", R"({ true: 1, "b": 2 })",   assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("wrong key type", R"({ false: 1, "b": 2 })",  assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("wrong key type", R"({ null: 1, "b": 2 })",   assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("wrong key type", R"({ []:  1, "b": 2 })",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("wrong key type", R"({ {}:  1, "b": 2 })",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    TEST_SUCCEED();
  }
  bool object_iterate_unclosed_error() {
    TEST_START();
    ONDEMAND_SUBTEST("unclosed", R"({ "a": 1,         )",    assert_iterate_object(doc.get_object(), { "a" }, { int64_t(1) }, { TAPE_ERROR }));
    // TODO These next two pass the user a value that may run past the end of the buffer if they aren't careful.
    // In particular, if the padding is decorated with the wrong values, we could cause overrun!
    ONDEMAND_SUBTEST("unclosed", R"({ "a": 1          )",    assert_iterate_object(doc.get_object(), { "a" }, { int64_t(1) }, { TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed", R"({ "a":            )",    assert_iterate_object(doc.get_object(),                          { NUMBER_ERROR, TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed", R"({ "a"             )",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    ONDEMAND_SUBTEST("unclosed", R"({                 )",    assert_iterate_object(doc.get_object(),                          { TAPE_ERROR }));
    TEST_SUCCEED();
  }

  bool object_lookup_error() {
    TEST_START();
    ONDEMAND_SUBTEST("missing colon", R"({ "a"  1, "b": 2 })",    assert_error(doc["a"], TAPE_ERROR));
    ONDEMAND_SUBTEST("missing key  ", R"({    : 1, "b": 2 })",    assert_error(doc["a"], TAPE_ERROR));
    ONDEMAND_SUBTEST("missing value", R"({ "a":  , "b": 2 })",    assert_success(doc["a"]));
    ONDEMAND_SUBTEST("missing comma", R"({ "a": 1  "b": 2 })",    assert_success(doc["a"]));
    TEST_SUCCEED();
  }
  bool object_lookup_unclosed_error() {
    TEST_START();
    // TODO This one passes the user a value that may run past the end of the buffer if they aren't careful.
    // In particular, if the padding is decorated with the wrong values, we could cause overrun!
    ONDEMAND_SUBTEST("unclosed", R"({ "a":            )",    assert_success(doc["a"]));
    ONDEMAND_SUBTEST("unclosed", R"({ "a"             )",    assert_error(doc["a"], TAPE_ERROR));
    ONDEMAND_SUBTEST("unclosed", R"({                 )",    assert_error(doc["a"], TAPE_ERROR));
    TEST_SUCCEED();
  }

  bool object_lookup_miss_error() {
    TEST_START();
    ONDEMAND_SUBTEST("missing colon", R"({ "a"  1, "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("missing key  ", R"({    : 1, "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("missing value", R"({ "a":  , "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("missing comma", R"({ "a": 1  "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    TEST_SUCCEED();
  }
  bool object_lookup_miss_wrong_key_type_error() {
    TEST_START();
    ONDEMAND_SUBTEST("wrong key type", R"({ 1:   1, "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("wrong key type", R"({ true: 1, "b": 2 })",   assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("wrong key type", R"({ false: 1, "b": 2 })",  assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("wrong key type", R"({ null: 1, "b": 2 })",   assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("wrong key type", R"({ []:  1, "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("wrong key type", R"({ {}:  1, "b": 2 })",    assert_error(doc["b"], TAPE_ERROR));
    TEST_SUCCEED();
  }
  bool object_lookup_miss_unclosed_error() {
    TEST_START();
    ONDEMAND_SUBTEST("unclosed", R"({ "a": 1,         )",    assert_error(doc["b"], TAPE_ERROR));
    // TODO These next two pass the user a value that may run past the end of the buffer if they aren't careful.
    // In particular, if the padding is decorated with the wrong values, we could cause overrun!
    ONDEMAND_SUBTEST("unclosed", R"({ "a": 1          )",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("unclosed", R"({ "a":            )",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("unclosed", R"({ "a"             )",    assert_error(doc["b"], TAPE_ERROR));
    ONDEMAND_SUBTEST("unclosed", R"({                 )",    assert_error(doc["b"], TAPE_ERROR));
    TEST_SUCCEED();
  }
  bool object_lookup_miss_next_error() {
    TEST_START();
    ONDEMAND_SUBTEST("missing comma", R"({ "a": 1  "b": 2 })", ([&]() {
      auto obj = doc.get_object();
      return assert_result<int64_t>(obj["a"], 1) && assert_error(obj["b"], TAPE_ERROR);
    })());
    TEST_SUCCEED();
  }

  bool run() {
    return
           empty_document_error() &&
           top_level_array_iterate_error() &&
           top_level_array_iterate_unclosed_error() &&
           array_iterate_error() &&
           array_iterate_unclosed_error() &&
           wrong_type::run() &&
           object_iterate_error() &&
           object_iterate_wrong_key_type_error() &&
           object_iterate_unclosed_error() &&
           object_lookup_error() &&
           object_lookup_unclosed_error() &&
           object_lookup_miss_error() &&
           object_lookup_miss_unclosed_error() &&
           object_lookup_miss_wrong_key_type_error() &&
           object_lookup_miss_next_error() &&
           true;
  }
}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, error_tests::run);
}
