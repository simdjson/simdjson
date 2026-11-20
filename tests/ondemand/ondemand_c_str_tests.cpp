#include "simdjson.h"
#include "test_ondemand.h"
#include <cstring>

using namespace simdjson;

namespace c_str_tests {
  using namespace std;

  bool document_c_str() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"("hello")"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    const char *s = nullptr;
    ASSERT_SUCCESS(doc.get_c_str().get(s));
    ASSERT_EQUAL(s, "hello");
    ASSERT_EQUAL(s[5], '\0');
    TEST_SUCCEED();
  }

  bool value_c_str() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"({"msg":"hello","empty":"","esc":"a\nb"})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    const char *s = nullptr;
    ASSERT_SUCCESS(doc["msg"].get_c_str().get(s));
    ASSERT_EQUAL(s, "hello");
    ASSERT_EQUAL(s[5], '\0');

    ondemand::document doc2;
    ASSERT_SUCCESS(parser.iterate(json).get(doc2));
    ASSERT_SUCCESS(doc2["empty"].get_c_str().get(s));
    ASSERT_EQUAL(s, "");
    ASSERT_EQUAL(s[0], '\0');

    ondemand::document doc3;
    ASSERT_SUCCESS(parser.iterate(json).get(doc3));
    ASSERT_SUCCESS(doc3["esc"].get_c_str().get(s));
    ASSERT_EQUAL(s, "a\nb");
    ASSERT_EQUAL(s[3], '\0');
    TEST_SUCCEED();
  }

  bool successive_c_str_remain_terminated() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"(["one","two"])"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ondemand::array arr;
    ASSERT_SUCCESS(doc.get_array().get(arr));
    const char *first = nullptr;
    const char *second = nullptr;
    auto it = arr.begin();
    ASSERT_SUCCESS((*it).get_c_str().get(first));
    ++it;
    ASSERT_SUCCESS((*it).get_c_str().get(second));
    ASSERT_EQUAL(first, "one");
    ASSERT_EQUAL(second, "two");
    ASSERT_EQUAL(first[3], '\0');
    ASSERT_EQUAL(second[3], '\0');
    TEST_SUCCEED();
  }

  bool embedded_null() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"("a\u0000b")"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    std::string_view view;
    ASSERT_SUCCESS(doc.get_string().get(view));
    ASSERT_EQUAL(view.size(), 3);
    ASSERT_EQUAL(view[0], 'a');
    ASSERT_EQUAL(view[1], '\0');
    ASSERT_EQUAL(view[2], 'b');
    ASSERT_EQUAL(view.data()[3], '\0');
    TEST_SUCCEED();
  }

  bool incorrect_type() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"({"n":1})"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    ASSERT_ERROR(doc["n"].get_c_str(), INCORRECT_TYPE);
    TEST_SUCCEED();
  }

  bool get_template() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"("hello")"_padded;
    ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    const char *s = nullptr;
    ASSERT_SUCCESS(doc.get(s));
    ASSERT_EQUAL(s, "hello");
    TEST_SUCCEED();
  }

  bool run() {
    return document_c_str() &&
           value_c_str() &&
           successive_c_str_remain_terminated() &&
           embedded_null() &&
           incorrect_type() &&
           get_template() &&
           true;
  }
} // namespace c_str_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, c_str_tests::run);
}
