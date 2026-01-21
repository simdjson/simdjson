#ifndef ONDEMAND_TEST_ONDEMAND_H
#define ONDEMAND_TEST_ONDEMAND_H

#include <iostream>
#include <unistd.h>
#include "simdjson.h"
#include "cast_tester.h"
#include "test_macros.h"
#include "test_main.h"

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


#endif // ONDEMAND_TEST_ONDEMAND_H
