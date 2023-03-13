#ifndef ONDEMAND_TEST_ONDEMAND_H
#define ONDEMAND_TEST_ONDEMAND_H

#include <iostream>
#include <unistd.h>
#include "simdjson.h"
#include "cast_tester.h"
#include "test_macros.h"

template<typename T, typename F>
bool test_ondemand(simdjson::ondemand::parser &parser, const simdjson::padded_string &json, const F& f) {
  auto doc = parser.iterate(json);
  T val{};
  ASSERT_SUCCESS( doc.get(val) );
  return f(val);
}
template<typename T, typename F>
bool test_ondemand(const simdjson::padded_string &json, const F& f) {
  simdjson::ondemand::parser parser;
  return test_ondemand<T, F>(parser, json, f);
}

template<typename F>
bool test_ondemand_doc(simdjson::ondemand::parser &parser, const simdjson::padded_string &json, const F& f) {
  return f(parser.iterate(json));
}
template<typename F>
bool test_ondemand_doc(const simdjson::padded_string &json, const F& f) {
  simdjson::ondemand::parser parser;
  return test_ondemand_doc(parser, json, f);
}

#define ONDEMAND_SUBTEST(NAME, JSON, TEST) \
{ \
  std::cout << "- Subtest " << NAME << " - JSON: " << (JSON) << " ..." << std::endl; \
  if (!test_ondemand_doc(JSON##_padded, [&](auto doc) { \
    return (TEST); \
  })) { \
    return false; \
  } \
}

const size_t AMAZON_CELLPHONES_NDJSON_DOC_COUNT = 793;
#define SIMDJSON_SHOW_DEFINE(x) printf("%s=%s\n", #x, SIMDJSON_STRINGIFY(x))

template<typename F>
int test_main(int argc, char *argv[], const F& test_function) {
  std::cout << std::unitbuf;
  int c;
  while ((c = getopt(argc, argv, "a:")) != -1) {
    switch (c) {
    case 'a': {
      const simdjson::implementation *impl = simdjson::get_available_implementations()[optarg];
      if (!impl) {
        std::fprintf(stderr, "Unsupported architecture value -a %s\n", optarg);
        return EXIT_FAILURE;
      }
      simdjson::get_active_implementation() = impl;
      break;
    }
    default:
      std::fprintf(stderr, "Unexpected argument %c\n", c);
      return EXIT_FAILURE;
    }
  }

  // this is put here deliberately to check that the documentation is correct (README),
  // should this fail to compile, you should update the documentation:
  if (simdjson::get_active_implementation()->name() == "unsupported") {
    std::printf("unsupported CPU\n");
    std::abort();
  }
  // We want to know what we are testing.
  std::cout << "builtin_implementation -- " << simdjson::builtin_implementation()->name() << std::endl;
  std::cout << "------------------------------------------------------------" << std::endl;

  std::cout << "Running tests." << std::endl;
  if (test_function()) {
    std::cout << "Success!" << std::endl;
    return EXIT_SUCCESS;
  } else {
    std::cerr << "FAILED." << std::endl;
    return EXIT_FAILURE;
  }
}

#endif // ONDEMAND_TEST_ONDEMAND_H
