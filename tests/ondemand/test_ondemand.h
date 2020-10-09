#ifndef ONDEMAND_TEST_ONDEMAND_H
#define ONDEMAND_TEST_ONDEMAND_H

#include "simdjson.h"
#include "cast_tester.h"
#include "test_macros.h"

template<typename T, typename F>
bool test_ondemand(simdjson::builtin::ondemand::parser &parser, const simdjson::padded_string &json, const F& f) {
  auto doc = parser.iterate(json);
  T val;
  ASSERT_SUCCESS( doc.get(val) );
  return f(val);
}
template<typename T, typename F>
bool test_ondemand(const simdjson::padded_string &json, const F& f) {
  simdjson::builtin::ondemand::parser parser;
  return test_ondemand<T, F>(parser, json, f);
}

template<typename F>
bool test_ondemand_doc(simdjson::builtin::ondemand::parser &parser, const simdjson::padded_string &json, const F& f) {
  return f(parser.iterate(json));
}
template<typename F>
bool test_ondemand_doc(const simdjson::padded_string &json, const F& f) {
  simdjson::builtin::ondemand::parser parser;
  return test_ondemand_doc(parser, json, f);
}

#endif // ONDEMAND_TEST_ONDEMAND_H
