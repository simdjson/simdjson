#include "simdjson.h"
#include "test_ondemand.h"

#include <string>
#include <vector>
#include <list>

class Array : public std::vector<float> {};

namespace simdjson {
template <>
constexpr bool require_custom_serialization<Array> = true;


template <typename ValT>
error_code tag_invoke(deserialize_tag, ValT &val,
                      Array &out) noexcept(false) {

  ondemand::array arr;
  SIMDJSON_TRY(val.get_array().get(arr));
  for (auto v : arr) {
    float temp;
    if (auto const err = v.get<float>().get(temp); err) {
      return err;
    }
    out.push_back(temp);
    out.push_back(temp); // adding it twice
  }
  return SUCCESS;
}
}

namespace stl_types {
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_DESERIALIZATION

bool basic_general_madness() {
  TEST_START();
  simdjson::padded_string json =
      R"( [ { "codes": [1,2,3,4,18,19] },
            { "codes": [1,3,4,111,1111] } ])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  std::vector<uint16_t> codes;
  for (auto val : doc) {
    simdjson::ondemand::object obj;
    SIMDJSON_TRY(val.get_object().get(obj));
    obj["codes"].get(codes); // append to it
  }

  if (codes.size() != 11) {
    return false;
  }
  if (codes[5] != 19 || codes[7] != 3 || codes[9] != 111) {
    return false;
  }

  TEST_SUCCEED();
}

bool basic_general_madness_vector() {
  TEST_START();
  simdjson::padded_string json =
      R"( [ { "codes": [1.2, 3.4, 5.6, 7.8, 9.0] },
            { "codes": [2.2, 4.4, 6.6, 8.8, 10.0] } ])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  std::vector<float> codes;
  for (auto val : doc) {
    simdjson::ondemand::object obj;
    SIMDJSON_TRY(val.get_object().get(obj));
    obj["codes"].get(codes); // append to it
  }

  if (codes.size() != 10) {
    return false;
  }
  if (codes[5] != 2.2f || codes[7] != 6.6f || codes[9] != 10.0f) {
    return false;
  }

  TEST_SUCCEED();
}

bool basic_general_madness_list() {
  TEST_START();
  simdjson::padded_string json =
      R"( [ { "codes": [1.2, 3.4, 5.6, 7.8, 9.0] },
            { "codes": [2.2, 4.4, 6.6, 8.8, 10.0] } ])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  std::list<float> codes;
  for (auto val : doc) {
    simdjson::ondemand::object obj;
    SIMDJSON_TRY(val.get_object().get(obj));
    obj["codes"].get(codes); // append to it
  }

  if (codes.size() != 10) {
    return false;
  }
  if (codes.back() != 10.0f || codes.front() != 1.2f) {
    return false;
  }

  TEST_SUCCEED();
}



bool basic_general_madness_Array() {
  TEST_START();
  simdjson::padded_string json =
      R"( [ { "codes": [1.2, 3.4, 5.6, 7.8, 9.0] },
            { "codes": [2.2, 4.4, 6.6, 8.8, 10.0] } ])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  Array codes;
  for (auto val : doc) {
    simdjson::ondemand::object obj;
    SIMDJSON_TRY(val.get_object().get(obj));
    obj["codes"].get(codes); // append to it
  }

  if (codes.size() != 20) {
    return false;
  }

  TEST_SUCCEED();
}


#endif // SIMDJSON_EXCEPTIONS
bool run() {
  return
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_DESERIALIZATION
      basic_general_madness() &&
      basic_general_madness_vector() &&
      basic_general_madness_list() &&
      basic_general_madness_Array() &&
#endif // SIMDJSON_EXCEPTIONS
      true;
}

} // namespace stl_types

int main(int argc, char *argv[]) {
  return test_main(argc, argv, stl_types::run);
}
