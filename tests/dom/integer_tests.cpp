
#include <iostream>
#include <limits>

#include "simdjson.h"
#include "test_macros.h"

// we define our own asserts to get around NDEBUG
#ifndef ASSERT
#define ASSERT(x)                                                       \
{    if (!(x)) {                                                        \
        abort ();                                                       \
    }                                                                   \
}
#endif

using namespace simdjson;

static const std::string make_json(const std::string value) {
  const std::string s = "{\"key\": ";
  return s + value + "}";
}

// e.g. make_json(123) => {"key": 123} as string
template <typename T> static const std::string make_json(T value) {
  return make_json(std::to_string(value));
}

template <typename T>
static bool parse_and_validate(const std::string src, T expected) {
  std::cout << "src: " << src << ", ";
  const padded_string pstr{src};
  simdjson::dom::parser parser;

  SIMDJSON_IF_CONSTEXPR (std::is_same<int64_t, T>::value) {
    int64_t actual{};
    ASSERT_SUCCESS( parser.parse(pstr)["key"].get(actual) );
    std::cout << std::boolalpha << "test: " << (expected == actual) << std::endl;
    ASSERT_EQUAL( expected, actual );
  } else {
    uint64_t actual{};
    ASSERT_SUCCESS( parser.parse(pstr)["key"].get(actual) );
    std::cout << std::boolalpha << "test: " << (expected == actual) << std::endl;
    ASSERT_EQUAL( expected, actual );
  }
  return true;
}

static bool parse_and_check_signed(const std::string src) {
  std::cout << "src: " << src << ", expecting signed" << std::endl;
  const padded_string pstr{src};
  simdjson::dom::parser parser;
  simdjson::dom::element value;
  ASSERT_SUCCESS( parser.parse(pstr).get_object()["key"].get(value) );
  ASSERT_EQUAL( value.is<int64_t>(), true );
  return true;
}

static bool parse_and_check_unsigned(const std::string src) {
  std::cout << "src: " << src << ", expecting signed" << std::endl;
  const padded_string pstr{src};
  simdjson::dom::parser parser;
  simdjson::dom::element value;
  ASSERT_SUCCESS( parser.parse(pstr).get_object()["key"].get(value) );
  ASSERT_EQUAL( value.is<uint64_t>(), true );
  return true;
}

int main() {
  using std::numeric_limits;
  constexpr auto int64_max = numeric_limits<int64_t>::max();
  constexpr auto int64_min = numeric_limits<int64_t>::lowest();
  constexpr auto uint64_max = numeric_limits<uint64_t>::max();
  constexpr auto uint64_min = numeric_limits<uint64_t>::lowest();
  constexpr auto int64_max_plus1 = static_cast<uint64_t>(int64_max) + 1;
  if (true
    && parse_and_validate(make_json(int64_max), int64_max)
    && parse_and_validate(make_json(uint64_max), uint64_max)
    && parse_and_validate(make_json(uint64_min), uint64_min)
    && parse_and_validate(make_json(int64_min), int64_min)
    && parse_and_validate(make_json(uint64_max), uint64_max)
    && parse_and_validate(make_json(uint64_min), uint64_min)
    && parse_and_validate(make_json(int64_max_plus1), int64_max_plus1)
    && parse_and_check_signed(make_json(int64_max))
    && parse_and_check_unsigned(make_json(uint64_max))
  ) {
    std::cout << "All ok." << std::endl;
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

