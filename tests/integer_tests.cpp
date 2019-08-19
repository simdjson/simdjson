
#include <cassert>
#include <iostream>
#include <limits>

#include "simdjson/jsonparser.h"

using namespace simdjson;

const std::string make_json(const std::string value) {
  const std::string s = "{\"key\": ";
  return s + value + "}";
}

// e.g. make_json(123) => {"key": 123} as string
template <typename T> const std::string make_json(T value) {
  return make_json(std::to_string(value));
}

template <typename T>
void parse_and_validate(const std::string src, T expected) {
  std::cout << "src: " << src << std::endl;
  const padded_string pstr{src};
  auto json = build_parsed_json(pstr);

  assert(json.is_valid());
  ParsedJson::Iterator it{json};
  assert(it.down());
  assert(it.next());
  bool result;
  if constexpr (std::is_same<int64_t, T>::value) {
    int64_t actual = it.get_integer();
    result = expected == actual;
    std::cout << "e:" << expected << ", " << actual << ", " << result
              << std::endl;
  } else {
    uint64_t actual = it.get_unsigned_integer();
    result = expected == actual;
    std::cout << "e:" << expected << ", " << actual << ", " << result
              << std::endl;
  }
  assert(result);
  if (!result) {
    std::cerr << "faild to compare!!";
  }
}

int main() {
  using std::numeric_limits;
#if 0
  constexpr int64_t int64_max = numeric_limits<int64_t>::max();
  parse_and_validate(make_json(int64_max), int64_max);
  constexpr auto int64_min = numeric_limits<int64_t>::min();
  parse_and_validate(make_json(int64_min), int64_min);
#endif
  constexpr auto uint64_max = numeric_limits<uint64_t>::max();
  parse_and_validate(make_json(uint64_max), uint64_max);
}

