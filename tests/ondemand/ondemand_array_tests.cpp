#include "simdjson.h"
#include "test_ondemand.h"

using namespace simdjson;

namespace array_tests {
  using namespace std;
  using simdjson::ondemand::json_type;

  bool iterate_document_array() {
    TEST_START();
    const auto json = R"([ 1, 10, 100 ])"_padded;
    const uint64_t expected_value[] = { 1, 10, 100 };

    SUBTEST("ondemand::array", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::array array;
      ASSERT_RESULT( doc_result.type(), json_type::array );
      ASSERT_SUCCESS( doc_result.get(array) );

      size_t i=0;
      for (auto value : array) {
        int64_t actual;
        ASSERT_SUCCESS( value.get(actual) );
        ASSERT_EQUAL(actual, expected_value[i]);
        i++;
      }
      ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      return true;
    }));

    SUBTEST("simdjson_result<ondemand::array>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::array> array = doc_result.get_array();
      ASSERT_RESULT( doc_result.type(), json_type::array );
      size_t i=0;
      for (simdjson_unused auto value : array) { int64_t actual; ASSERT_SUCCESS( value.get(actual) ); ASSERT_EQUAL(actual, expected_value[i]); i++; }
      ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      return true;
    }));

    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::document doc;
      ASSERT_SUCCESS( std::move(doc_result).get(doc) );
      ASSERT_RESULT( doc.type(), json_type::array );
      size_t i=0;
      for (simdjson_unused auto value : doc) { int64_t actual; ASSERT_SUCCESS( value.get(actual) ); ASSERT_EQUAL(actual, expected_value[i]); i++; }
      ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      size_t i=0;
      ASSERT_RESULT( doc_result.type(), json_type::array );
      for (simdjson_unused auto value : doc_result) { int64_t actual; ASSERT_SUCCESS( value.get(actual) ); ASSERT_EQUAL(actual, expected_value[i]); i++; }
      ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      return true;
    }));
    TEST_SUCCEED();
  }

  bool iterate_array() {
    TEST_START();
    const auto json = R"( [ [ 1, 10, 100 ] ] )"_padded;
    const uint64_t expected_value[] = { 1, 10, 100 };

    SUBTEST("ondemand::array", test_ondemand_doc(json, [&](auto doc_result) {
      bool found = false;
      for (simdjson_result<ondemand::value> array_result : doc_result) {
        ASSERT_TRUE(!found); found = true;

        ondemand::array array;
        ASSERT_RESULT( array_result.type(), json_type::array );
        ASSERT_SUCCESS( array_result.get(array) );

        size_t i=0;
        for (auto value : array) {
          int64_t actual;
          ASSERT_SUCCESS( value.get(actual) );
          ASSERT_EQUAL(actual, expected_value[i]);
          i++;
        }
        ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      }
        ASSERT_TRUE(found);
      return true;
    }));

    SUBTEST("simdjson_result<ondemand::array>", test_ondemand_doc(json, [&](auto doc_result) {
      bool found = false;
      for (simdjson_result<ondemand::value> array_result : doc_result) {
        ASSERT_TRUE(!found); found = true;

        ondemand::array array;
        ASSERT_RESULT( array_result.type(), json_type::array );
        ASSERT_SUCCESS( array_result.get(array) );

        size_t i=0;
        for (simdjson_unused auto value : array) { int64_t actual; ASSERT_SUCCESS( value.get(actual) ); ASSERT_EQUAL(actual, expected_value[i]); i++; }
        ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      }
      ASSERT_TRUE(found);
      return true;
    }));

    SUBTEST("ondemand::value", test_ondemand_doc(json, [&](auto doc_result) {
      bool found = false;
      for (simdjson_result<ondemand::value> array_result : doc_result) {
        ASSERT_TRUE(!found); found = true;

        ondemand::value array;
        ASSERT_SUCCESS( std::move(array_result).get(array) );
        ASSERT_RESULT( array.type(), json_type::array );
        size_t i=0;
        for (simdjson_unused auto value : array) { int64_t actual; ASSERT_SUCCESS( value.get(actual) ); ASSERT_EQUAL(actual, expected_value[i]); i++; }
        ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      }
      ASSERT_TRUE(found);
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::value>", test_ondemand_doc(json, [&](auto doc_result) {
      bool found = false;
      for (simdjson_result<ondemand::value> array_result : doc_result) {
        ASSERT_TRUE(!found); found = true;

        size_t i=0;
        ASSERT_RESULT( array_result.type(), json_type::array );
        for (simdjson_unused auto value : array_result) { int64_t actual; ASSERT_SUCCESS( value.get(actual) ); ASSERT_EQUAL(actual, expected_value[i]); i++; }
        ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      }
      ASSERT_TRUE(found);
      return true;
    }));
    TEST_SUCCEED();
  }

  bool iterate_array_partial_children() {
    TEST_START();
    auto json = R"(
      [
        0,
        [],
        {},
        { "x": 3, "y": 33 },
        { "x": 4, "y": 44 },
        { "x": 5, "y": 55 },
        { "x": 6, "y": 66 },
        [ 7, 77, 777 ],
        [ 8, 88, 888 ],
        { "a": [ { "b": [ 9, 99 ], "c": 999 }, 9999 ], "d": 99999 },
        10
      ]
    )"_padded;
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      size_t i = 0;
      for (auto value : doc_result) {
        ASSERT_SUCCESS(value);

        switch (i) {
          case 0: {
            std::cout << "  - After ignoring empty scalar ..." << std::endl;
            break;
          }
          case 1: {
            std::cout << "  - After ignoring empty array ..." << std::endl;
            break;
          }
          case 2: {
            std::cout << "  - After ignoring empty object ..." << std::endl;
            break;
          }
          // Break after using first value in child object
          case 3: {
            for (auto [ child_field, error ] : value.get_object()) {
              ASSERT_SUCCESS(error);
              ASSERT_EQUAL(child_field.key(), "x");
              uint64_t x;
              ASSERT_SUCCESS( child_field.value().get(x) );
              ASSERT_EQUAL(x, 3);
              break; // Break after the first value
            }
            std::cout << "  - After using first value in child object ..." << std::endl;
            break;
          }

          // Break without using first value in child object
          case 4: {
            for (auto [ child_field, error ] : value.get_object()) {
              ASSERT_SUCCESS(error);
              ASSERT_EQUAL(child_field.key(), "x");
              break;
            }
            std::cout << "  - After reaching (but not using) first value in child object ..." << std::endl;
            break;
          }

          // Only look up one field in child object
          case 5: {
            uint64_t x;
            ASSERT_SUCCESS( value["x"].get(x) );
            ASSERT_EQUAL( x, 5 );
            std::cout << "  - After looking up one field in child object ..." << std::endl;
            break;
          }

          // Only look up one field in child object, but don't use it
          case 6: {
            ASSERT_SUCCESS( value["x"] );
            std::cout << "  - After looking up (but not using) one field in child object ..." << std::endl;
            break;
          }

          // Break after first value in child array
          case 7: {
            for (auto [ child_value, error ] : value) {
              ASSERT_SUCCESS(error);
              uint64_t x;
              ASSERT_SUCCESS( child_value.get(x) );
              ASSERT_EQUAL( x, 7 );
              break;
            }
            std::cout << "  - After using first value in child array ..." << std::endl;
            break;
          }

          // Break without using first value in child array
          case 8: {
            for (auto child_value : value) {
              ASSERT_SUCCESS(child_value);
              break;
            }
            std::cout << "  - After reaching (but not using) first value in child array ..." << std::endl;
            break;
          }

          // Break out of multiple child loops
          case 9: {
            for (auto child1 : value.get_object()) {
              for (auto child2 : child1.value().get_array()) {
                for (auto child3 : child2.get_object()) {
                  for (auto child4 : child3.value().get_array()) {
                    uint64_t x;
                    ASSERT_SUCCESS( child4.get(x) );
                    ASSERT_EQUAL( x, 9 );
                    break;
                  }
                  break;
                }
                break;
              }
              break;
            }
            std::cout << "  - After breaking out of quadruply-nested arrays and objects ..." << std::endl;
            break;
          }

          // Test the actual value
          case 10: {
            uint64_t actual_value;
            ASSERT_SUCCESS( value.get(actual_value) );
            ASSERT_EQUAL( actual_value, 10 );
            break;
          }
        }

        i++;
      }
      ASSERT_EQUAL( i, 11 ); // Make sure we found all the keys we expected
      return true;
    }));
    return true;
  }

  bool iterate_empty_array() {
    TEST_START();
    auto json = "[]"_padded;
    SUBTEST("ondemand::array", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::array array;
      ASSERT_SUCCESS( doc_result.get(array) );
      for (simdjson_unused auto value : array) { TEST_FAIL("Unexpected value"); }
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::array>", test_ondemand_doc(json, [&](auto doc_result) {
      simdjson_result<ondemand::array> array_result = doc_result.get_array();
      for (simdjson_unused auto value : array_result) { TEST_FAIL("Unexpected value"); }
      return true;
    }));
    SUBTEST("ondemand::document", test_ondemand_doc(json, [&](auto doc_result) {
      ondemand::document doc;
      ASSERT_SUCCESS( std::move(doc_result).get(doc) );
      for (simdjson_unused auto value : doc) { TEST_FAIL("Unexpected value"); }
      return true;
    }));
    SUBTEST("simdjson_result<ondemand::document>", test_ondemand_doc(json, [&](auto doc_result) {
      for (simdjson_unused auto value : doc_result) { TEST_FAIL("Unexpected value"); }
      return true;
    }));
    TEST_SUCCEED();
  }

#if SIMDJSON_EXCEPTIONS

  bool iterate_array_exception() {
    TEST_START();
    auto json = R"([ 1, 10, 100 ])"_padded;
    const uint64_t expected_value[] = { 1, 10, 100 };

    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      size_t i=0;
      for (int64_t actual : doc_result) { ASSERT_EQUAL(actual, expected_value[i]); i++; }
      ASSERT_EQUAL(i*sizeof(uint64_t), sizeof(expected_value));
      return true;
    }));
    TEST_SUCCEED();
  }

  bool iterate_empty_object_exception() {
    TEST_START();
    auto json = R"({})"_padded;

    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      for (simdjson_unused ondemand::field field : doc_result.get_object()) {
        TEST_FAIL("Unexpected field");
      }
      return true;
    }));

    TEST_SUCCEED();
  }

  bool iterate_empty_array_exception() {
    TEST_START();
    auto json = "[]"_padded;

    ASSERT_TRUE(test_ondemand_doc(json, [&](auto doc_result) {
      for (simdjson_unused ondemand::value value : doc_result) { TEST_FAIL("Unexpected value"); }
      return true;
    }));

    TEST_SUCCEED();
  }

#endif // SIMDJSON_EXCEPTIONS

  bool run() {
    return
           iterate_array() &&
           iterate_document_array() &&
           iterate_empty_array() &&
           iterate_array_partial_children() &&
#if SIMDJSON_EXCEPTIONS
           iterate_array_exception() &&
#endif // SIMDJSON_EXCEPTIONS
           true;
  }

} // namespace dom_api_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, array_tests::run);
}
