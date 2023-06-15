#include "simdjson.h"
#include "test_ondemand.h"

#include <cstdint>

using namespace simdjson;

namespace iterate_many_csv_tests {
using namespace std;

bool normal() {
  TEST_START();
  auto json = R"( 1, 2, 3, 4, "a", "b", "c", {"hello": "world"} , [1, 2, 3])"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, json.size(), true).get(doc_stream));

  for (auto doc : doc_stream)
  {
    ASSERT_SUCCESS(doc);
  }

  TEST_SUCCEED();
}

bool small_batch_size() {
  TEST_START();
  auto json = R"( 1, 2, 3, 4, "a", "b", "c", {"hello": "world"} , [1, 2, 3])"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, 32, true).get(doc_stream));

  for (auto doc : doc_stream)
  {
    ASSERT_SUCCESS(doc);
  }

  TEST_SUCCEED();
}

bool trailing_comma() {
  TEST_START();
  auto json = R"(1,)"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, json.size(), true).get(doc_stream));

  for (auto doc : doc_stream)
  {
    ASSERT_SUCCESS(doc);
  }

  TEST_SUCCEED();
}

bool check_parsed_values() {
  TEST_START();

  auto json = R"(  1  , "a" , [100, 1]  , {"hello"  :    "world"}  , )"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, json.size(), true).get(doc_stream));

  auto begin = doc_stream.begin();
  auto end = doc_stream.end();
  int cnt = 0;
  auto it = begin;
  for (; it != end && cnt < 4; ++it, ++cnt) {
    auto doc = *it;
    switch (cnt)
    {
    case 0:
    {
      int64_t actual;
      ASSERT_SUCCESS(doc.get_int64().get(actual));
      ASSERT_EQUAL(actual, 1);
      break;
    }
    case 1:
    {
      std::string_view sv;
      ASSERT_SUCCESS(doc.get_string().get(sv));
      ASSERT_EQUAL(sv, "a");
      break;
    }
    case 2:
    {
      std::vector<int64_t> expected{100, 1};
      ondemand::array arr;
      ASSERT_SUCCESS(doc.get_array().get(arr));
      size_t element_count;
      ASSERT_SUCCESS(arr.count_elements().get(element_count));
      ASSERT_EQUAL(element_count, 2);
      int i = 0;
      for (auto a : arr)
      {
        int64_t actual;
        ASSERT_SUCCESS(a.get(actual));
        ASSERT_EQUAL(actual, expected[i++]);
      }
      break;
    }
    case 3:
    {
      ondemand::object obj;
      ASSERT_SUCCESS(doc.get_object().get(obj));
      std::string_view sv;
      obj.find_field("hello").get(sv);
      ASSERT_EQUAL(sv, "world");
      break;
    }
    default:
      TEST_FAIL("Too many cases")
    }
  }

  ASSERT_EQUAL(cnt, 4);
  ASSERT_TRUE(!(it != end));

  TEST_SUCCEED();
}

#if SIMDJSON_EXCEPTIONS

bool leading_comma() {
  TEST_START();
  auto json = R"(,1)"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  ASSERT_SUCCESS(parser.iterate_many(json, json.size(), true).get(doc_stream));

  try {
    auto begin = doc_stream.begin();
    auto end = doc_stream.end();
    for (auto it = begin; it != end; ++it) {}
  } catch (simdjson_error& e) {
    ASSERT_ERROR(e.error(), TAPE_ERROR);
  }

  TEST_SUCCEED();
}

#endif

bool run() {
  return normal() &&
         small_batch_size() &&
         trailing_comma() &&
         check_parsed_values() &&
#if SIMDJSON_EXCEPTIONS
         leading_comma() &&
#endif
         true;
}

}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, iterate_many_csv_tests::run);
}
