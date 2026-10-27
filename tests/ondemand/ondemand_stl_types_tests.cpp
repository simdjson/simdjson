#include "simdjson.h"
#include "test_ondemand.h"

#include <string>
#include <vector>
#include <list>
#include <set>
#include <stack>
#include <queue>
#include <deque>
#include <unordered_set>
#include <optional>
#include <memory>
#include <limits>

#if SIMDJSON_SUPPORTS_CONCEPTS
class Array : public std::vector<float> {};
static_assert(simdjson::concepts::appendable_containers<Array>, "Array must be appendable_containers");
static_assert(!simdjson::require_custom_serialization<Array>);
namespace simdjson {
// This tag_invoke MUST be inside simdjson namespace
template <typename simdjson_value>
auto tag_invoke(deserialize_tag, simdjson_value &val, Array& out) {
    simdjson::ondemand::array arr;
    SIMDJSON_TRY(val.get_array().get(arr));
    for (auto v : arr) {
      float temp;
      if (auto const err = v.get<float>().get(temp); err) {
        return err;
      }
      out.push_back(temp);
      out.push_back(temp); // adding it twice
    }
    return error_code::SUCCESS;
}
} // namespace simdjson

// require_custom_serialization
#endif // SIMDJSON_SUPPORTS_CONCEPTS

namespace stl_types {
#if SIMDJSON_SUPPORTS_CONCEPTS
// Smart pointers to built-in types go through the generic smart-pointer
// tag_invoke: same range checks as get<T>(), null resets the pointer, and a
// failed parse leaves the pointer untouched.
template <template <typename...> class Ptr, typename T>
bool smart_pointer_case(std::string_view json, simdjson::error_code expected, T expected_value = T{}) {
  simdjson::ondemand::parser parser;
  // As a document.
  {
    simdjson::padded_string padded(json);
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    Ptr<T> out;
    ASSERT_EQUAL(doc.get(out), expected);
    if (expected == simdjson::SUCCESS) {
      ASSERT_TRUE(out != nullptr);
      ASSERT_TRUE(*out == expected_value);
    } else {
      ASSERT_TRUE(out == nullptr);
    }
  }
  // As an object field (ondemand::value).
  {
    simdjson::padded_string padded(std::string(R"({"a":)") + std::string(json) + "}");
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    Ptr<T> out;
    ASSERT_EQUAL(doc["a"].get(out), expected);
    if (expected == simdjson::SUCCESS) {
      ASSERT_TRUE(out != nullptr);
      ASSERT_TRUE(*out == expected_value);
    } else {
      ASSERT_TRUE(out == nullptr);
    }
  }
  // null resets a non-null pointer.
  {
    simdjson::padded_string padded(std::string_view(R"({"a":null})"));
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    Ptr<T> out(new T{});
    ASSERT_SUCCESS(doc["a"].get(out));
    ASSERT_TRUE(out == nullptr);
  }
  return true;
}

template <template <typename...> class Ptr>
bool smart_pointer_builtin_types() {
  using simdjson::SUCCESS;
  using simdjson::INCORRECT_TYPE;
  using simdjson::NUMBER_OUT_OF_RANGE;
  return smart_pointer_case<Ptr, bool>("true", SUCCESS, true) &&
         smart_pointer_case<Ptr, bool>("1", INCORRECT_TYPE) &&
         smart_pointer_case<Ptr, int>("-2147483648", SUCCESS, (std::numeric_limits<int>::min)()) &&
         smart_pointer_case<Ptr, int>("2147483647", SUCCESS, (std::numeric_limits<int>::max)()) &&
         smart_pointer_case<Ptr, int>("2147483648", NUMBER_OUT_OF_RANGE) &&
         smart_pointer_case<Ptr, int>("-2147483649", NUMBER_OUT_OF_RANGE) &&
         smart_pointer_case<Ptr, int>("4294967297", NUMBER_OUT_OF_RANGE) &&
         smart_pointer_case<Ptr, int>("\"1\"", INCORRECT_TYPE) &&
         smart_pointer_case<Ptr, int64_t>("-9223372036854775808", SUCCESS, (std::numeric_limits<int64_t>::min)()) &&
         smart_pointer_case<Ptr, int64_t>("9223372036854775808", INCORRECT_TYPE) &&
         smart_pointer_case<Ptr, uint64_t>("18446744073709551615", SUCCESS, (std::numeric_limits<uint64_t>::max)()) &&
         smart_pointer_case<Ptr, uint64_t>("-1", INCORRECT_TYPE) &&
         smart_pointer_case<Ptr, double>("1.5", SUCCESS, 1.5) &&
         smart_pointer_case<Ptr, double>("true", INCORRECT_TYPE) &&
         smart_pointer_case<Ptr, std::string>("\"h\\u00e9llo\"", SUCCESS, std::string("h\xc3\xa9llo")) &&
         smart_pointer_case<Ptr, std::string>("12", INCORRECT_TYPE) &&
         smart_pointer_case<Ptr, std::string_view>("\"hello\"", SUCCESS, std::string_view("hello")) &&
         smart_pointer_case<Ptr, std::string_view>("12", INCORRECT_TYPE);
}

bool smart_pointers_to_builtin_types() {
  TEST_START();
  ASSERT_TRUE(smart_pointer_builtin_types<std::unique_ptr>());
  ASSERT_TRUE(smart_pointer_builtin_types<std::shared_ptr>());
  TEST_SUCCEED();
}
#endif // SIMDJSON_SUPPORTS_CONCEPTS

#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS

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
  ASSERT_EQUAL(codes.size(), 20);
  TEST_SUCCEED();
}


template <template <typename...> typename Container, typename ValT = float>
bool basic_general_madness_container() {
  TYPED_TEST_START(Container<ValT>);

  simdjson::padded_string json =
      R"( [ { "codes": [1.0, 3.4, 5.6, 7.8, 9.0] },
            { "codes": [2.2, 4.4, 6.6, 8.8, 10.0] } ])"_padded;

  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc = parser.iterate(json);
  Container<ValT> codes;
  for (auto val : doc) {
    simdjson::ondemand::object obj;
    SIMDJSON_TRY(val.get_object().get(obj));
    obj["codes"].get(codes); // append to it
  }

  if (codes.size() != 10) {
    return false;
  }


  if constexpr (requires {codes.front();}) {
    if constexpr (requires {*codes.front();}) {
      if (*codes.front() != 1.0) {
        return false;
      }
    } else {
      if (codes.front() != 1.0) {
        return false;
      }
    }
  }
  if constexpr (requires {codes.back();} ) {
    if constexpr (requires {*codes.back();}) {
      if (*codes.back() != 10.0) {
        return false;
      }
    } else {
      if (codes.back() != 10.0) {
        return false;
      }
    }
  }

  TEST_SUCCEED();
}


#endif // SIMDJSON_EXCEPTIONS
bool run() {
  return
#if SIMDJSON_SUPPORTS_CONCEPTS
      smart_pointers_to_builtin_types() &&
#endif // SIMDJSON_SUPPORTS_CONCEPTS
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
      basic_general_madness() &&
      basic_general_madness_vector() &&
      basic_general_madness_list() &&
      basic_general_madness_Array() &&
      basic_general_madness_container<std::vector>() &&
      basic_general_madness_container<std::list>() &&
      basic_general_madness_container<std::set>() &&
      basic_general_madness_container<std::stack>() &&
      basic_general_madness_container<std::queue>() &&
      basic_general_madness_container<std::deque>() &&
      basic_general_madness_container<std::priority_queue>() &&
      basic_general_madness_container<std::unordered_set>() &&
      basic_general_madness_container<std::multiset>() &&
      basic_general_madness_container<std::unordered_multiset>() &&
        // double:
      basic_general_madness_container<std::vector, double>() &&
      basic_general_madness_container<std::list, double>() &&
      basic_general_madness_container<std::set, double>() &&
      basic_general_madness_container<std::stack, double>() &&
      basic_general_madness_container<std::queue, double>() &&
      basic_general_madness_container<std::deque, double>() &&
      basic_general_madness_container<std::priority_queue, double>() &&
      basic_general_madness_container<std::unordered_set, double>() &&
      basic_general_madness_container<std::multiset, double>() &&
      basic_general_madness_container<std::unordered_multiset, double>() &&
        // unique_ptr<double>:
      basic_general_madness_container<std::vector, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::list, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::set, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::stack, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::deque, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::queue, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::priority_queue, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::unordered_set, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::multiset, std::unique_ptr<double>>() &&
      basic_general_madness_container<std::unordered_multiset, std::unique_ptr<double>>() &&
        // shared_ptr<double>:
      basic_general_madness_container<std::vector, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::list, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::set, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::stack, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::deque, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::queue, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::priority_queue, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::unordered_set, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::multiset, std::shared_ptr<double>>() &&
      basic_general_madness_container<std::unordered_multiset, std::shared_ptr<double>>() &&
        // optional<double>:
      basic_general_madness_container<std::vector, std::optional<double>>() &&
      basic_general_madness_container<std::list, std::optional<double>>() &&
      basic_general_madness_container<std::set, std::optional<double>>() &&
      basic_general_madness_container<std::stack, std::optional<double>>() &&
      basic_general_madness_container<std::deque, std::optional<double>>() &&
      basic_general_madness_container<std::queue, std::optional<double>>() &&
      basic_general_madness_container<std::priority_queue, std::optional<double>>() &&
      basic_general_madness_container<std::unordered_set, std::optional<double>>() &&
      basic_general_madness_container<std::multiset, std::optional<double>>() &&
      basic_general_madness_container<std::unordered_multiset, std::optional<double>>() &&
#endif // SIMDJSON_EXCEPTIONS
      true;
}

} // namespace stl_types

int main(int argc, char *argv[]) {
  return test_main(argc, argv, stl_types::run);
}
