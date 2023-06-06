#include "simdjson.h"
#include "test_ondemand.h"

#include <cstdint>

using namespace simdjson;

namespace iterate_many_csv_tests {
using namespace std;

#if SIMDJSON_EXCEPTIONS

bool normal() {
  TEST_START();
  auto json = R"( 1, 2, 3, 4, "a", "b", "c", {"hello": "world"} , [1, 2, 3])"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream = parser.iterate_many(json, json.size(), true);

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
  ondemand::document_stream doc_stream = parser.iterate_many(json, 32, true);

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
  ondemand::document_stream doc_stream = parser.iterate_many(json, json.size(), true);

  for (auto doc : doc_stream)
  {
    ASSERT_SUCCESS(doc);
  }

  TEST_SUCCEED();
}

bool leading_comma() {
  TEST_START();
  auto json = R"(,1)"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream = parser.iterate_many(json, json.size(), true);

  try {
    auto begin = doc_stream.begin();
    auto end = doc_stream.end();
    for (auto it = begin; it != end; ++it) {}
  } catch (simdjson_error& e) {
    ASSERT_ERROR(e.error(), TAPE_ERROR);
  }

  TEST_SUCCEED();
}

bool check_parsed_values() {
  TEST_START();

  auto json = R"(  1  , "a" , [100, 1]  , {"hello"  :    "world"}  , )"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream = parser.iterate_many(json, json.size(), true);

  auto begin = doc_stream.begin();
  auto end = doc_stream.end();
  int cnt = 0;
  auto it = begin;
  for (; it != end && cnt < 4; ++it, ++cnt) {
    auto doc = *it;
    switch (cnt)
    {
    case 0:
      ASSERT_EQUAL(int(doc.get_int64()), 1);
      break;
    case 1:
      ASSERT_EQUAL(std::string_view(doc.get_string()), "a");
      break;
    case 2:
    {
      std::vector<int64_t> expected{100, 1};
      ondemand::array arr = doc.get_array();
      ASSERT_EQUAL(int(arr.count_elements()), 2);
      int i = 0;
      for (auto a : arr)
      {
        ASSERT_EQUAL(int64_t(a), expected[i++]);
      }
    }
      break;
    case 3:
    {
      ondemand::object obj = doc.get_object();
      ASSERT_EQUAL(std::string_view(obj["hello"]), "world");
    }
      break;
    default:
      TEST_FAIL("Too many cases")
    }
  }

  ASSERT_EQUAL(cnt, 4);
  ASSERT_TRUE(!(it != end));

  TEST_SUCCEED();
}

#endif

bool run() {
  return
#if SIMDJSON_EXCEPTIONS
         normal() &&
         small_batch_size() &&
         trailing_comma() &&
         leading_comma() &&
         check_parsed_values() &&
#endif
         true;
}

}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, iterate_many_csv_tests::run);
}
