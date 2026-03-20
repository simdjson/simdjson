#include "simdjson.h"
#include "test_macros.h"
#include <sstream>

using namespace simdjson;

namespace big_integer_tests {

  bool option_off_by_default() {
    TEST_START();
    dom::parser parser;
    ASSERT_EQUAL(parser.number_as_string(), false);
    TEST_SUCCEED();
  }

  bool default_returns_bigint_error() {
    TEST_START();
    dom::parser parser;
    auto result = parser.parse(R"({"val":123456789012345678901})"_padded);
    ASSERT_EQUAL(result.error(), BIGINT_ERROR);
    TEST_SUCCEED();
  }

  bool opt_in_parses_big_integer() {
    TEST_START();
    dom::parser parser;
    parser.number_as_string(true);
    dom::element doc;
    ASSERT_SUCCESS(parser.parse(R"({"val":123456789012345678901})"_padded).get(doc));
    dom::element val;
    ASSERT_SUCCESS(doc["val"].get(val));
    ASSERT_EQUAL(val.type(), dom::element_type::BIGINT);
    ASSERT_TRUE(val.is_bigint());
    std::string_view digits;
    ASSERT_SUCCESS(val.get_bigint().get(digits));
    ASSERT_EQUAL(digits, "123456789012345678901");
    // Verify streaming works (same pattern as readme_examples)
    std::ostringstream os;
    os << val.get_bigint().value_unsafe();
    ASSERT_EQUAL(os.str(), "123456789012345678901");
    TEST_SUCCEED();
  }

  bool negative_big_integer() {
    TEST_START();
    dom::parser parser;
    parser.number_as_string(true);
    dom::element doc;
    ASSERT_SUCCESS(parser.parse(R"({"val":-12345678901234567890})"_padded).get(doc));
    dom::element val;
    ASSERT_SUCCESS(doc["val"].get(val));
    ASSERT_EQUAL(val.type(), dom::element_type::BIGINT);
    std::string_view digits;
    ASSERT_SUCCESS(val.get_bigint().get(digits));
    ASSERT_EQUAL(digits, "-12345678901234567890");
    TEST_SUCCEED();
  }

  bool big_integer_in_array() {
    TEST_START();
    dom::parser parser;
    parser.number_as_string(true);
    dom::element doc;
    ASSERT_SUCCESS(parser.parse(R"([1, 123456789012345678901, 3])"_padded).get(doc));
    dom::array arr;
    ASSERT_SUCCESS(doc.get_array().get(arr));
    size_t count = 0;
    for (auto elem : arr) { (void)elem; count++; }
    ASSERT_EQUAL(count, 3);
    TEST_SUCCEED();
  }

  bool serialization_preserves_digits() {
    TEST_START();
    dom::parser parser;
    parser.number_as_string(true);
    dom::element doc;
    ASSERT_SUCCESS(parser.parse(R"({"val":123456789012345678901})"_padded).get(doc));
    std::string output = simdjson::to_string(doc);
    // Raw digits should appear
    ASSERT_TRUE(output.find("123456789012345678901") != std::string::npos);
    // Must not be quoted — it's a number, not a string
    ASSERT_EQUAL(output.find("\"123456789012345678901\""), std::string::npos);
    TEST_SUCCEED();
  }

  bool normal_numbers_unaffected() {
    TEST_START();
    dom::parser parser;
    parser.number_as_string(true);
    dom::element doc;
    ASSERT_SUCCESS(parser.parse(R"({"a":42,"b":-1,"c":3.14,"d":18446744073709551615})"_padded).get(doc));
    int64_t a;
    ASSERT_SUCCESS(doc["a"].get_int64().get(a));
    ASSERT_EQUAL(a, 42);
    uint64_t d;
    ASSERT_SUCCESS(doc["d"].get_uint64().get(d));
    ASSERT_EQUAL(d, 18446744073709551615ULL);
    TEST_SUCCEED();
  }

  bool type_checks() {
    TEST_START();
    dom::parser parser;
    parser.number_as_string(true);
    dom::element doc;
    ASSERT_SUCCESS(parser.parse(R"({"val":123456789012345678901})"_padded).get(doc));
    dom::element val;
    ASSERT_SUCCESS(doc["val"].get(val));
    // is_bigint should be true
    ASSERT_TRUE(val.is_bigint());
    // other type checks should be false
    ASSERT_FALSE(val.is_int64());
    ASSERT_FALSE(val.is_uint64());
    ASSERT_FALSE(val.is_double());
    ASSERT_FALSE(val.is_string());
    ASSERT_FALSE(val.is_bool());
    ASSERT_FALSE(val.is_null());
    // get_int64/uint64/double should return INCORRECT_TYPE
    ASSERT_ERROR(val.get_int64(), INCORRECT_TYPE);
    ASSERT_ERROR(val.get_uint64(), INCORRECT_TYPE);
    ASSERT_ERROR(val.get_double(), INCORRECT_TYPE);
    ASSERT_ERROR(val.get_string(), INCORRECT_TYPE);
    TEST_SUCCEED();
  }

  bool element_type_output() {
    TEST_START();
    std::ostringstream os;
    os << dom::element_type::BIGINT;
    ASSERT_EQUAL(os.str(), "bigint");
    TEST_SUCCEED();
  }

  bool run() {
    return option_off_by_default() &&
           default_returns_bigint_error() &&
           opt_in_parses_big_integer() &&
           negative_big_integer() &&
           big_integer_in_array() &&
           serialization_preserves_digits() &&
           normal_numbers_unaffected() &&
           type_checks() &&
           element_type_output();
  }

} // namespace big_integer_tests

int main() {
  return big_integer_tests::run() ? 0 : 1;
}
