#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <set>
#include <unistd.h>

#include "simdjson.h"

using namespace simdjson;
using namespace std;

#include "test_macros.h"

namespace parser_load {
  const char * NONEXISTENT_FILE = "this_file_does_not_exist.json";
  bool parser_load_capacity() {
    TEST_START();
    dom::parser parser(1); // 1 byte max capacity
    auto error = parser.load(TWITTER_JSON).error();
    ASSERT_ERROR(error, CAPACITY);
    TEST_SUCCEED();
  }
  bool parser_load_many_capacity() {
    TEST_START();
    dom::parser parser(1); // 1 byte max capacity
    dom::document_stream docs;
    ASSERT_SUCCESS(parser.load_many(TWITTER_JSON).get(docs));
    for (auto doc : docs) {
      ASSERT_ERROR(doc.error(), CAPACITY);
      TEST_SUCCEED();
    }
    TEST_FAIL("No documents returned");
  }

  bool parser_parse_many_documents_error_in_the_middle() {
    TEST_START();
    const padded_string DOC = "1 2 [} 3"_padded;
    size_t count = 0;
    dom::parser parser;
    dom::document_stream docs;
    ASSERT_SUCCESS(parser.parse_many(DOC).get(docs));
    for (auto doc : docs) {
      count++;
      uint64_t val{};
      auto error = doc.get(val);
      if (count == 3) {
        ASSERT_ERROR(error, TAPE_ERROR);
      } else {
        ASSERT_SUCCESS(error);
        ASSERT_EQUAL(val, count);
      }
    }
    ASSERT_EQUAL(count, 3);
    TEST_SUCCEED();
  }

  bool parser_parse_many_documents_partial() {
    TEST_START();
    const padded_string DOC = "["_padded;
    size_t count = 0;
    dom::parser parser;
    dom::document_stream docs;
    ASSERT_SUCCESS(parser.parse_many(DOC).get(docs));
    for (auto doc : docs) {
      count++;
      ASSERT_ERROR(doc.error(), TAPE_ERROR);
    }
    ASSERT_EQUAL(count, 0);
    TEST_SUCCEED();
  }

  bool parser_parse_many_documents_partial_at_the_end() {
    TEST_START();
    const padded_string DOC = "1 2 ["_padded;
    size_t count = 0;
    dom::parser parser;
    dom::document_stream docs;
    ASSERT_SUCCESS(parser.parse_many(DOC).get(docs));
    for (auto doc : docs) {
      count++;
      uint64_t val{};
      auto error = doc.get(val);
      if (count == 3) {
        ASSERT_ERROR(error, TAPE_ERROR);
      } else {
        ASSERT_SUCCESS(error);
        ASSERT_EQUAL(val, count);
      }
    }
    ASSERT_EQUAL(count, 2);
    TEST_SUCCEED();
  }

  bool parser_load_nonexistent() {
    TEST_START();
    dom::parser parser;
    ASSERT_ERROR( parser.load(NONEXISTENT_FILE).error(), IO_ERROR );
    TEST_SUCCEED();
  }
  bool parser_load_many_nonexistent() {
    TEST_START();
    dom::parser parser;
    ASSERT_ERROR( parser.load_many(NONEXISTENT_FILE).error(), IO_ERROR );
    TEST_SUCCEED();
  }
  bool padded_string_load_nonexistent() {
    TEST_START();
    ASSERT_ERROR(padded_string::load(NONEXISTENT_FILE).error(), IO_ERROR);
    TEST_SUCCEED();
  }

  bool parser_load_chain() {
    TEST_START();
    dom::parser parser;
    simdjson_unused uint64_t foo;
    ASSERT_ERROR( parser.load(NONEXISTENT_FILE)["foo"].get(foo), IO_ERROR);
    TEST_SUCCEED();
  }
  bool parser_load_many_chain() {
    TEST_START();
    dom::parser parser;
    simdjson_unused dom::document_stream stream;
    ASSERT_ERROR( parser.load_many(NONEXISTENT_FILE).get(stream), IO_ERROR );
    TEST_SUCCEED();
  }

  bool run() {
    return true
        && parser_load_capacity()
        && parser_load_many_capacity()
        && parser_load_nonexistent()
        && parser_load_many_nonexistent()
        && padded_string_load_nonexistent()
        && parser_load_chain()
        && parser_load_many_chain()
        && parser_parse_many_documents_error_in_the_middle()
        && parser_parse_many_documents_partial()
        && parser_parse_many_documents_partial_at_the_end()
    ;
  }
}

namespace adversarial {
  #define PADDING_FILLED_WITH_NUMBERS "22222222222222222222222222222222222222222222222222222222222222222"
  bool number_overrun_at_root() {
    TEST_START();
    constexpr const char *json = "1" PADDING_FILLED_WITH_NUMBERS ",";
    constexpr size_t len = 1; // std::strlen("1");

    dom::parser parser;
    uint64_t foo;
    ASSERT_SUCCESS( parser.parse(json, len).get(foo) ); // Parse just the first digit
    ASSERT_EQUAL( foo, 1 );
    TEST_SUCCEED();
  }
  bool number_overrun_in_array() {
    TEST_START();
    constexpr const char *json = "[1" PADDING_FILLED_WITH_NUMBERS "]";
    constexpr size_t len = 2; // std::strlen("[1");

    dom::parser parser;
    uint64_t foo;
    ASSERT_ERROR( parser.parse(json, len).get(foo), TAPE_ERROR ); // Parse just the first digit
    TEST_SUCCEED();
  }
  bool number_overrun_in_object() {
    TEST_START();
    constexpr const char *json = "{\"key\":1" PADDING_FILLED_WITH_NUMBERS "}";
    constexpr size_t len = 8; // std::strlen("{\"key\":1");

    dom::parser parser;
    uint64_t foo;
    ASSERT_ERROR( parser.parse(json, len).get(foo), TAPE_ERROR ); // Parse just the first digit
    TEST_SUCCEED();
  }
  bool run() {
    constexpr size_t filler_size = 65;
    static_assert(filler_size > SIMDJSON_PADDING, "corruption test doesn't have enough padding"); // 33 = std::strlen(PADDING_FILLED_WITH_NUMBERS)
    return (std::strlen(PADDING_FILLED_WITH_NUMBERS) == filler_size)
      && number_overrun_at_root()
      && number_overrun_in_array()
      && number_overrun_in_object()
    ;
  }
}

int main(int argc, char *argv[]) {
  std::cout << std::unitbuf;
  int c;
  while ((c = getopt(argc, argv, "a:")) != -1) {
    switch (c) {
    case 'a': {
      const simdjson::implementation *impl = simdjson::get_available_implementations()[optarg];
      if (!impl) {
        fprintf(stderr, "Unsupported architecture value -a %s\n", optarg);
        return EXIT_FAILURE;
      }
      if(!impl->supported_by_runtime_system()) {
        fprintf(stderr, "The selected implementation does not match your current CPU: -a %s\n", optarg);
        return EXIT_FAILURE;
      }
      simdjson::get_active_implementation() = impl;
      break;
    }
    default:
      fprintf(stderr, "Unexpected argument %c\n", c);
      return EXIT_FAILURE;
    }
  }

  // this is put here deliberately to check that the documentation is correct (README),
  // should this fail to compile, you should update the documentation:
  if (simdjson::get_active_implementation()->name() == "unsupported") {
    printf("unsupported CPU\n");
  }
  // We want to know what we are testing.
  std::cout << "Running tests against this implementation: " << simdjson::get_active_implementation()->name();
  std::cout << " (" << simdjson::get_active_implementation()->description() << ")" << std::endl;
  std::cout << "------------------------------------------------------------" << std::endl;
  std::cout << "Running error tests." << std::endl;
  if (!(true
        && parser_load::run()
        && adversarial::run()
  )) {
    return EXIT_FAILURE;
  }
  std::cout << "Error tests are ok." << std::endl;
  return EXIT_SUCCESS;
}
