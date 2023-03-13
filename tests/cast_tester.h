#ifndef CAST_TESTER_H
#define CAST_TESTER_H

#include "simdjson.h"
#include "test_macros.h"

namespace {
  using simdjson::error_code;
  using simdjson::simdjson_error;
  using simdjson::simdjson_result;
  using simdjson::dom::array;
  using simdjson::dom::element;
  using simdjson::dom::object;
}

// cast_tester<T> tester;
// tester.test_implicit(value, [](T value) { return true; })
// tester.test_implicit_error(value, error)
// Used to test casts to a type. In the case of const char * in particular, we don't test
// implicit casts at all, so that method always returns true.
template<typename T>
class cast_tester {
public:
  bool test_get(element element, T expected = {});
  bool test_get(simdjson_result<element> element, T expected = {});
  bool test_get_error(element element, error_code expected_error);
  bool test_get_error(simdjson_result<element> element, error_code expected_error);

  bool test_get_t(element element, T expected = {});
  bool test_get_t(simdjson_result<element> element, T expected = {});
  bool test_get_t_error(element element, error_code expected_error);
  bool test_get_t_error(simdjson_result<element> element, error_code expected_error);

#if SIMDJSON_EXCEPTIONS
  bool test_implicit_cast(element element, T expected = {});
  bool test_implicit_cast(simdjson_result<element> element, T expected = {});
  bool test_implicit_cast_error(element element, error_code expected_error);
  bool test_implicit_cast_error(simdjson_result<element> element, error_code expected_error);
#endif // SIMDJSON_EXCEPTIONS

  bool test_is(element element, bool expected);
  bool test_is(simdjson_result<element> element, bool expected);

  bool test_named_get(element element, T expected = {});
  bool test_named_get(simdjson_result<element> element, T expected = {});
  bool test_named_get_error(element element, error_code expected_error);
  bool test_named_get_error(simdjson_result<element> element, error_code expected_error);

  bool test_named_is(element element, bool expected);
  bool test_named_is(simdjson_result<element> element, bool expected);

private:
  simdjson_result<T> named_get(element element);
  simdjson_result<T> named_get(simdjson_result<element> element);
  bool named_is(element element);
  bool named_is(simdjson_result<element> element);
  bool assert_equal(const T& expected, const T& actual);
};

template<typename T>
bool cast_tester<T>::test_get(element element, T expected) {
  T actual{};
  ASSERT_SUCCESS(element.get(actual));
  return assert_equal(actual, expected);
}

template<typename T>
bool cast_tester<T>::test_get(simdjson_result<element> element, T expected) {
  T actual{};
  ASSERT_SUCCESS(element.get(actual));
  return assert_equal(actual, expected);
}

template<typename T>
bool cast_tester<T>::test_get_error(element element, error_code expected_error) {
  T actual{};
  ASSERT_EQUAL(element.get(actual), expected_error);
  return true;
}

template<typename T>
bool cast_tester<T>::test_get_error(simdjson_result<element> element, error_code expected_error) {
  T actual{};
  ASSERT_EQUAL(element.get(actual), expected_error);
  return true;
}

template<typename T>
bool cast_tester<T>::test_get_t(element element, T expected) {
  auto actual = element.get<T>();
  ASSERT_SUCCESS(actual.error());
  return assert_equal(actual.value_unsafe(), expected);
}

template<typename T>
bool cast_tester<T>::test_get_t(simdjson_result<element> element, T expected) {
  auto actual = element.get<T>();
  ASSERT_SUCCESS(actual.error());
  return assert_equal(actual.value_unsafe(), expected);
}

template<typename T>
bool cast_tester<T>::test_get_t_error(element element, error_code expected_error) {
  ASSERT_EQUAL(element.get<T>().error(), expected_error);
  return true;
}

template<typename T>
bool cast_tester<T>::test_get_t_error(simdjson_result<element> element, error_code expected_error) {
  ASSERT_EQUAL(element.get<T>().error(), expected_error);
  return true;
}

template<typename T>
bool cast_tester<T>::test_named_get(element element, T expected) {
  T actual{};
  ASSERT_SUCCESS(named_get(element).get(actual));
  return assert_equal(actual, expected);
}

template<typename T>
bool cast_tester<T>::test_named_get(simdjson_result<element> element, T expected) {
  T actual{};
  ASSERT_SUCCESS(named_get(element).get(actual));
  return assert_equal(actual, expected);
}

template<typename T>
bool cast_tester<T>::test_named_get_error(element element, error_code expected_error) {
  T actual{};
  ASSERT_EQUAL(named_get(element).get(actual), expected_error);
  return true;
}

template<typename T>
bool cast_tester<T>::test_named_get_error(simdjson_result<element> element, error_code expected_error) {
  T actual{};
  ASSERT_EQUAL(named_get(element).get(actual), expected_error);
  return true;
}

#if SIMDJSON_EXCEPTIONS

template<typename T>
bool cast_tester<T>::test_implicit_cast(element element, T expected) {
  T actual{};
  try {
    actual = element;
  } catch(simdjson_error &e) {
    std::cerr << e.error() << std::endl;
    return false;
  }
  return assert_equal(actual, expected);
}

template<typename T>
bool cast_tester<T>::test_implicit_cast(simdjson_result<element> element, T expected) {
  T actual{};
  try {
    actual = element;
  } catch(simdjson_error &e) {
    std::cerr << e.error() << std::endl;
    return false;
  }
  return assert_equal(actual, expected);
}

template<typename T>
bool cast_tester<T>::test_implicit_cast_error(element element, error_code expected_error) {
  try {
    simdjson_unused T actual{};
    actual = element;
    return false;
  } catch(simdjson_error &e) {
    ASSERT_EQUAL(e.error(), expected_error);
    return true;
  }
}

template<typename T>
bool cast_tester<T>::test_implicit_cast_error(simdjson_result<element> element, error_code expected_error) {
  try {
    simdjson_unused T actual{};
    actual = element;
    return false;
  } catch(simdjson_error &e) {
    ASSERT_EQUAL(e.error(), expected_error);
    return true;
  }
}

template<> bool cast_tester<const char *>::test_implicit_cast(element, const char *) { return true; }
template<> bool cast_tester<const char *>::test_implicit_cast(simdjson_result<element>, const char *) { return true; }
template<> bool cast_tester<const char *>::test_implicit_cast_error(element, error_code) { return true; }
template<> bool cast_tester<const char *>::test_implicit_cast_error(simdjson_result<element>, error_code) { return true; }

#endif // SIMDJSON_EXCEPTIONS

template<typename T>
bool cast_tester<T>::test_is(element element, bool expected) {
  ASSERT_EQUAL(element.is<T>(), expected);
  return true;
}

template<typename T>
bool cast_tester<T>::test_is(simdjson_result<element> element, bool expected) {
  ASSERT_EQUAL(element.is<T>(), expected);
  return true;
}

template<typename T>
bool cast_tester<T>::test_named_is(element element, bool expected) {
  ASSERT_EQUAL(named_is(element), expected);
  return true;
}

template<typename T>
bool cast_tester<T>::test_named_is(simdjson_result<element> element, bool expected) {
  ASSERT_EQUAL(named_is(element), expected);
  return true;
}

template<> simdjson_result<array> cast_tester<array>::named_get(element element) { return element.get_array(); }
template<> simdjson_result<object> cast_tester<object>::named_get(element element) { return element.get_object(); }
template<> simdjson_result<const char *> cast_tester<const char *>::named_get(element element) { return element.get_c_str(); }
template<> simdjson_result<std::string_view> cast_tester<std::string_view>::named_get(element element) { return element.get_string(); }
template<> simdjson_result<uint64_t> cast_tester<uint64_t>::named_get(element element) { return element.get_uint64(); }
template<> simdjson_result<int64_t> cast_tester<int64_t>::named_get(element element) { return element.get_int64(); }
template<> simdjson_result<double> cast_tester<double>::named_get(element element) { return element.get_double(); }
template<> simdjson_result<bool> cast_tester<bool>::named_get(element element) { return element.get_bool(); }

template<> simdjson_result<array> cast_tester<array>::named_get(simdjson_result<element> element) { return element.get_array(); }
template<> simdjson_result<object> cast_tester<object>::named_get(simdjson_result<element> element) { return element.get_object(); }
template<> simdjson_result<const char *> cast_tester<const char *>::named_get(simdjson_result<element> element) { return element.get_c_str(); }
template<> simdjson_result<std::string_view> cast_tester<std::string_view>::named_get(simdjson_result<element> element) { return element.get_string(); }
template<> simdjson_result<uint64_t> cast_tester<uint64_t>::named_get(simdjson_result<element> element) { return element.get_uint64(); }
template<> simdjson_result<int64_t> cast_tester<int64_t>::named_get(simdjson_result<element> element) { return element.get_int64(); }
template<> simdjson_result<double> cast_tester<double>::named_get(simdjson_result<element> element) { return element.get_double(); }
template<> simdjson_result<bool> cast_tester<bool>::named_get(simdjson_result<element> element) { return element.get_bool(); }

template<> bool cast_tester<array>::named_is(element element) { return element.is_array(); }
template<> bool cast_tester<object>::named_is(element element) { return element.is_object(); }
template<> bool cast_tester<const char *>::named_is(element element) { return element.is_string(); }
template<> bool cast_tester<std::string_view>::named_is(element element) { return element.is_string(); }
template<> bool cast_tester<uint64_t>::named_is(element element) { return element.is_uint64(); }
template<> bool cast_tester<int64_t>::named_is(element element) { return element.is_int64(); }
template<> bool cast_tester<double>::named_is(element element) { return element.is_double(); }
template<> bool cast_tester<bool>::named_is(element element) { return element.is_bool(); }

template<> bool cast_tester<array>::named_is(simdjson_result<element> element) { return element.is_array(); }
template<> bool cast_tester<object>::named_is(simdjson_result<element> element) { return element.is_object(); }
template<> bool cast_tester<const char *>::named_is(simdjson_result<element> element) { return element.is_string(); }
template<> bool cast_tester<std::string_view>::named_is(simdjson_result<element> element) { return element.is_string(); }
template<> bool cast_tester<uint64_t>::named_is(simdjson_result<element> element) { return element.is_uint64(); }
template<> bool cast_tester<int64_t>::named_is(simdjson_result<element> element) { return element.is_int64(); }
template<> bool cast_tester<double>::named_is(simdjson_result<element> element) { return element.is_double(); }
template<> bool cast_tester<bool>::named_is(simdjson_result<element> element) { return element.is_bool(); }

template<typename T> bool cast_tester<T>::assert_equal(const T& expected, const T& actual) {
  ASSERT_EQUAL(expected, actual);
  return true;
}
// We don't actually check equality for objects and arrays, just check that they actually cast
template<> bool cast_tester<array>::assert_equal(const array&, const array&) {
  return true;
}
template<> bool cast_tester<object>::assert_equal(const object&, const object&) {
  return true;
}

#endif