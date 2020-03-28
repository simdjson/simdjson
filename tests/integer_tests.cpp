
#include <iostream>
#include <limits>

#include "simdjson.h"

// we define our own asserts to get around NDEBUG
#ifndef ASSERT
#define ASSERT(x)                                                       \
{    if (!(x)) {                                                        \
        char buf[4096];                                                 \
        snprintf (buf, 4096, "Failure in \"%s\", line %d\n",            \
                 __FILE__, __LINE__);                                   \
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
static void parse_and_validate(const std::string src, T expected) {
  std::cout << "src: " << src << ", ";
  const padded_string pstr{src};
  simdjson::dom::parser parser;

  bool result;
  if constexpr (std::is_same<int64_t, T>::value) {
    auto [actual, error] = parser.parse(pstr).get<dom::object>()["key"].get<int64_t>();
    if (error) { std::cerr << error << std::endl; abort(); }
    result = (expected == actual);
  } else {
    auto [actual, error] = parser.parse(pstr).get<dom::object>()["key"].get<uint64_t>();
    if (error) { std::cerr << error << std::endl; abort(); }
    result = (expected == actual);
  }
  std::cout << std::boolalpha << "test: " << result << std::endl;
  if(!result) {
    std::cerr << "bug detected" << std::endl;
    exit(EXIT_FAILURE);
  }
}

static bool parse_and_check_signed(const std::string src) {
  std::cout << "src: " << src << ", expecting signed" << std::endl;
  const padded_string pstr{src};
  simdjson::dom::parser parser;
  auto [value, error] = parser.parse(pstr).get<dom::object>()["key"];
  if (error) { std::cerr << error << std::endl; abort(); }
  return value.is<int64_t>();
}

static bool parse_and_check_unsigned(const std::string src) {
  std::cout << "src: " << src << ", expecting signed" << std::endl;
  const padded_string pstr{src};
  simdjson::dom::parser parser;
  auto [value, error] = parser.parse(pstr).get<dom::object>()["key"];
  if (error) { std::cerr << error << std::endl; abort(); }
  return value.is<uint64_t>();
}

int main() {
  using std::numeric_limits;
  constexpr auto int64_max = numeric_limits<int64_t>::max();
  constexpr auto int64_min = numeric_limits<int64_t>::lowest();
  constexpr auto uint64_max = numeric_limits<uint64_t>::max();
  constexpr auto uint64_min = numeric_limits<uint64_t>::lowest();
  parse_and_validate(make_json(int64_max), int64_max);
  parse_and_validate(make_json(int64_min), int64_min);
  parse_and_validate(make_json(uint64_max), uint64_max);
  parse_and_validate(make_json(uint64_min), uint64_min);
  constexpr auto int64_max_plus1 = static_cast<uint64_t>(int64_max) + 1;
  parse_and_validate(make_json(int64_max_plus1), int64_max_plus1);
  if(!parse_and_check_signed(make_json(int64_max))) {
    std::cerr << "bug: large signed integers should be represented as signed integers" << std::endl;
    return EXIT_FAILURE;
  }
  if(!parse_and_check_unsigned(make_json(uint64_max))) {
    std::cerr << "bug: a large unsigned integers is not represented as an unsigned integer" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "All ok." << std::endl;
  return EXIT_SUCCESS;
}

