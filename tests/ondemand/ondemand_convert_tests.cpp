#include "simdjson.h"
#include "simdjson/convert.h"
#include "test_ondemand.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <ranges>
#include <string>
#include <unordered_set>
#include <vector>

#if defined(__has_include)
#if __has_include(<inplace_vector>)
#include <inplace_vector>
#endif
#endif

#if SIMDJSON_SUPPORTS_CONCEPTS
// The containers that from() fills directly, without On Demand, when the elements are strings or doubles.
static_assert(simdjson::internal::direct::appendable_container_of<std::vector<std::string>, std::string>);
static_assert(simdjson::internal::direct::appendable_container_of<std::deque<std::string>, std::string>);
static_assert(simdjson::internal::direct::appendable_container_of<std::list<double>, double>);
static_assert(simdjson::internal::direct::appendable_container_of<std::set<std::string>, std::string>);
static_assert(simdjson::internal::direct::appendable_container_of<std::unordered_multiset<double>, double>);
static_assert(!simdjson::internal::direct::appendable_container_of<std::string, std::string>);
static_assert(!simdjson::internal::direct::appendable_container_of<std::vector<float>, double>);
#endif

#if SIMDJSON_STATIC_REFLECTION

class MyDate {
public:
    void assign(std::string_view str) {
        date_str = str;
    }
    const std::string& to_string() const {
        return date_str;
    }
private:
    std::string date_str{};
};


struct Meeting {
    std::string title{};
    std::vector<std::string> attendees{};
    std::optional<std::string> location{};
    bool is_recurring{};
};


struct MeetingTime {
    std::string title{};
    std::chrono::system_clock::time_point start_time{};
    std::vector<std::string> attendees{};
    std::optional<std::string> location{};
    bool is_recurring{};
};


namespace simdjson {
template <typename simdjson_value>
auto tag_invoke(deserialize_tag, simdjson_value &val, MyDate& date) {
    std::string_view str;
    auto error = val.get_string().get(str);
    if(error) { return error; }
    date.assign(str);
  return simdjson::SUCCESS;
}
} // namespace simdjson

struct complicated_weather_data {
    std::vector<MyDate> time{};
    std::vector<float> temperature{};
};


#endif

#ifdef __cpp_lib_ranges

namespace convert_tests {

simdjson::padded_string json_car =
    R"( {
         "make": "Toyota",
         "model": "Camry",
         "year": 2018,
         "tire_pressure": [ 40.1, 39.9 ]
       } )"_padded;
simdjson::padded_string json_cars =
    R"( [ { "make": "Toyota", "model": "Camry",  "year": 2018,
       "tire_pressure": [ 40.1, 39.9 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012,
       "tire_pressure": [ 30.1, 31.0 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999,
       "tire_pressure": [ 29.8, 30.0 ] }
])"_padded;

#if SIMDJSON_SUPPORTS_CONCEPTS
  struct Car {
    std::string make{};
    std::string model{};
    int year{};
    std::vector<double> tire_pressure{};

    friend simdjson::error_code tag_invoke(simdjson::deserialize_tag, auto &val,
                                          Car &car) {
      simdjson::ondemand::object obj;
      auto error = val.get_object().get(obj);
      if (error) {
        return error;
      }
      // Instead of repeatedly obj["something"], we iterate through the object
      // which we expect to be faster.
      for (auto field : obj) {
        simdjson::ondemand::raw_json_string key;
        error = field.key().get(key);
        if (error) {
          return error;
        }
        if (key == "make") {
          error = field.value().get_string(car.make);
          if (error) {
            return error;
          }
        } else if (key == "model") {
          error = field.value().get_string(car.model);
          if (error) {
            return error;
          }
        } else if (key == "year") {
          error = field.value().get(car.year);
          if (error) {
            return error;
          }
        } else if (key == "tire_pressure") {
          error = field.value().get(car.tire_pressure);
          if (error) {
            return error;
          }
        }
      }
      return simdjson::SUCCESS;
    }
  };

  static_assert(simdjson::custom_deserializable<std::unique_ptr<Car>>,
                "It should be deserializable");


  bool simple_no_except() {
    TEST_START();
    Car car;
    ASSERT_SUCCESS(simdjson::from(json_car).get(car));
    if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
      return false;
    }
    TEST_SUCCEED();
  }
#endif // SIMDJSON_SUPPORTS_CONCEPTS
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS

  bool simple() {
    TEST_START();
    Car car = simdjson::from(json_car);
    if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
      return false;
    }
    TEST_SUCCEED();
  }
  struct Player {
      std::string username{};
      int level{};
      double health{};
  };
  struct BadPlayer {
      int username{};  // Oops, should be string!
      int level{};
      double health{};
  };
  struct OptionalPlayer {
      std::string username{};
      std::optional<int> level{};
      double health{};
  };
#if SIMDJSON_STATIC_REFLECTION
  bool missing_key_player() {
    TEST_START();
    std::string json = R"({"username":"Alice","health":100.0})";
    simdjson::padded_string padded(json);
    Player p;
    simdjson::ondemand::parser parser;
    auto doc = parser.iterate(padded);
    ASSERT_ERROR(doc.get(p), simdjson::NO_SUCH_FIELD);
    TEST_SUCCEED();
  }
  bool missing_key_optional_player() {
    TEST_START();
    std::string json = R"({"username":"Alice","health":100.0})";
    simdjson::padded_string padded(json);
    OptionalPlayer p;
    simdjson::ondemand::parser parser;
    auto doc = parser.iterate(padded);
    ASSERT_SUCCESS(doc.get(p));
    TEST_SUCCEED();
  }
  bool bad_player() {
    TEST_START();
    // username is a string but we declared it as an int
    std::string json = R"({"username":"Alice","level":42,"health":100.0})";
    simdjson::padded_string padded(json);
    BadPlayer p;
    simdjson::ondemand::parser parser;
    auto doc = parser.iterate(padded);
    ASSERT_FAILURE(doc.get(p));
    TEST_SUCCEED();
  }
  bool good_player() {
    TEST_START();
    std::string json = R"({"username":123,"level":42,"health":100.0})";
    simdjson::padded_string padded(json);
    BadPlayer p;
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    ASSERT_SUCCESS(doc.get(p));
    TEST_SUCCEED();
  }
  bool complicated_weather_test() {
    TEST_START();
    std::string json = R"({"time":["2023-03-15T12:00:00Z"],"temperature":[42]})";
    simdjson::padded_string padded(json);
    complicated_weather_data p;
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(padded).get(doc));
    ASSERT_SUCCESS(doc.get(p));
    TEST_SUCCEED();
  }


  bool meeting_test() {
    TEST_START();
    std::string json = simdjson::to_json(Meeting{
        .title = "CppCon Planning",
        .attendees = {"Alice", "Bob", "Charlie"},
        .location = "Denver",
        .is_recurring = true
    });
    Meeting m2 = simdjson::from(simdjson::pad(json));
    std::cout << m2.title << std::endl;
    TEST_SUCCEED();
  }
  bool meeting_time_test() {
    TEST_START();
    auto start_time = std::chrono::system_clock::now();
    std::string json = simdjson::to_json(MeetingTime{
        .title = "CppCon Planning",
        .start_time = start_time,
        .attendees = {"Alice", "Bob", "Charlie"},
        .location = "Denver",
        .is_recurring = true
    });
    std::cout << json << std::endl;
    MeetingTime m2 = simdjson::from(simdjson::pad(json));
    //ASSERT_EQUAL(m2.start_time, start_time);
    std::cout << m2.title << std::endl;
    TEST_SUCCEED();
  }


#endif // SIMDJSON_STATIC_REFLECTION
bool broken() {
  TEST_START();
  simdjson::padded_string short_json_cars = R"( { "make )"_padded;
  try {
    Car car = simdjson::from(json_cars);
    TEST_FAIL("Should not have succeeded");
  } catch (...) {
    TEST_SUCCEED();
  }
  TEST_SUCCEED();
}

bool simple_optional() {
  TEST_START();
  auto car = simdjson::from(json_car).optional<Car>();
  if (!car.has_value() || car->make != "Toyota" || car->model != "Camry" ||
      car->year != 2018) {
    return false;
  }
  TEST_SUCCEED();
}

bool example_with_parser() {
  TEST_START();
  simdjson::ondemand::parser parser;
  std::map<std::string, std::string> obj = simdjson::from(parser, R"({"key": "value"})"_padded);
  ASSERT_EQUAL(obj["key"], "value");
  TEST_SUCCEED();
}

bool with_parser() {
  TEST_START();
  simdjson::ondemand::parser parser;
  Car car = simdjson::from(parser, json_car);
  if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
    return false;
  }
  TEST_SUCCEED();
}

bool to_array() {
  TEST_START();
  auto parser = simdjson::from(json_cars);
  for (auto val : parser.array()) {
    Car car{};
    if (auto const error = val.get(car)) {
      std::cerr << simdjson::error_message(error) << std::endl;
      return false;
    }
    if (car.year < 1998) {
      std::cerr << car.make << " " << car.model << " " << car.year << std::endl;
      return false;
    }
  }
  TEST_SUCCEED();
}

bool to_bad_array() {
  TEST_START();
  auto parser = simdjson::from(json_car);
  try {
    auto array_result = parser.array();
    // Check if array_result has an error
    if (array_result.error() != simdjson::SUCCESS) {
      // This is expected - trying to get array from an object should fail
      if (array_result.error() != simdjson::INCORRECT_TYPE) {
        std::cerr << "Expected INCORRECT_TYPE but got: " << array_result.error()
                  << " (" << simdjson::error_message(array_result.error()) << ")" << std::endl;
        return false;
      }
      // Got expected error, test passes
      TEST_SUCCEED();
    }

    // If we get here without error, try to iterate
    // This might throw when we try to use the array
    for (auto val : array_result) {
      static_cast<void>(val);
      // Should not reach here - the JSON is an object, not an array
      std::cerr << "Unexpectedly succeeded in iterating over non-array JSON" << std::endl;
      return false;
    }
    // Also should not reach here
    std::cerr << "array() succeeded on object JSON without throwing" << std::endl;
    return false;
  } catch (simdjson::simdjson_error &e) {
    if (e.error() != simdjson::INCORRECT_TYPE) {
      std::cerr << "Expected INCORRECT_TYPE but got: " << e.error() << " (" << simdjson::error_message(e.error()) << ")" << std::endl;
      return false;
    }
    // Got expected exception, test passes
  } catch (...) {
    std::cerr << "Unexpected exception type" << std::endl;
    return false;
  }
  TEST_SUCCEED();
}

#if SIMDJSON_SUPPORTS_RANGES_FROM
// currently not active (SIMDJSON_SUPPORTS_RANGES_FROM)
bool to_array_shortcut() {
  TEST_START();
  simdjson::ondemand::parser parser;
  for (auto val : simdjson::from(parser, json_cars)) {
    Car car{};
    if (auto const error = val.get(car)) {
      std::cerr << simdjson::error_message(error) << std::endl;
      return false;
    }
    if (car.year < 1998) {
      std::cerr << car.make << " " << car.model << " " << car.year << std::endl;
      return false;
    }
  }
  TEST_SUCCEED();
}
// currently not active (SIMDJSON_SUPPORTS_RANGES_FROM)
bool test_basic_adaptor() {
  TEST_START();
  int64_t sum_year = 0;
  for (Car car : simdjson::from(json_cars) | simdjson::as<Car>()) {
    printf("Car: %s %s %d\n", car.make.c_str(), car.model.c_str(), car.year);
    if (car.year < 1998) {
      return false;
    }
    sum_year += car.year;
  }
  ASSERT_EQUAL(sum_year, 2018 + 2012 + 1999);
  TEST_SUCCEED();
}

// currently not active (SIMDJSON_SUPPORTS_RANGES_FROM)
bool test_basic_adaptor_with_parser() {
  TEST_START();
  simdjson::ondemand::parser parser;
  int64_t sum_year = 0;
  for (Car car : simdjson::from(parser, json_cars) | simdjson::as<Car>()) {
    if (car.year < 1998) {
      return false;
    }
    sum_year += car.year;
  }
  ASSERT_EQUAL(sum_year, 2018 + 2012 + 1999);
  TEST_SUCCEED();
}

// currently not active (SIMDJSON_SUPPORTS_RANGES_FROM)
bool test_no_errors() {
  TEST_START();
  int64_t sum_year = 0;
  auto cars = simdjson::from(json_cars) | simdjson::no_errors;
  for (auto val : cars) {
    Car car = val.get<Car>();
    if (car.year < 1998) {
      return false;
    }
    printf("-- year: %d\n", car.year);
    sum_year += car.year;
  }
  ASSERT_EQUAL(sum_year, 2018 + 2012 + 1999);
  TEST_SUCCEED();
}

// currently not active (SIMDJSON_SUPPORTS_RANGES_FROM)
bool to_clean_array() {
  TEST_START();
  int64_t sum_year = 0;
  for (auto val : simdjson::from(json_cars) | simdjson::no_errors) {
    Car car = val.get<Car>();
    if (car.year < 1998) {
      std::cerr << car.make << " " << car.model << " " << car.year << std::endl;
      return false;
    }
    sum_year += car.year;
  }
  ASSERT_EQUAL(sum_year, 2018 + 2012 + 1999);
  TEST_SUCCEED();
}

// currently not active (SIMDJSON_SUPPORTS_RANGES_FROM)
bool test_to_adaptor_basic() {
  TEST_START();
  // Test 1: Basic usage of to<T> with a value reference
  simdjson::ondemand::parser parser;
  auto doc_result = parser.iterate(json_car);
  if (doc_result.error()) {
    return false;
  }
  simdjson::ondemand::document doc = std::move(doc_result.value());
  simdjson::simdjson_result<simdjson::ondemand::value> val = doc.get_value();

  // to<T> converts a simdjson_result<value>& to T
  Car car = simdjson::to<Car>(val);
  if (car.make != "Toyota" || car.model != "Camry" || car.year != 2018) {
    return false;
  }
  TEST_SUCCEED();
}

#endif // SIMDJSON_SUPPORTS_RANGES_FROM

bool test_to_adaptor_with_single_value() {
  TEST_START();
  // Test 2: Using to<T> to convert individual values
  simdjson::ondemand::parser parser;
  auto doc_result = parser.iterate(json_car);
  if (doc_result.error()) {
    return false;
  }
  simdjson::ondemand::document doc = std::move(doc_result.value());

  // Get individual field and convert it
  auto obj_result = doc.get_object();
  if (obj_result.error()) {
    return false;
  }

  TEST_SUCCEED();
}

bool test_direct_from_atoms() {
  TEST_START();
  // Single-pass from<> for atoms, int64 arrays, and plain strings. Invalid
  // input must surface the same error as On Demand.
  {
    simdjson::padded_string j(std::string("-12345"));
    int64_t v = simdjson::from(j);
    ASSERT_EQUAL(v, int64_t(-12345));
  }
  {
    simdjson::padded_string j(std::string("true"));
    bool v = simdjson::from(j);
    ASSERT_TRUE(v);
  }
  {
    simdjson::padded_string j(std::string("\"plain\""));
    std::string v = simdjson::from(j);
    ASSERT_EQUAL(v, std::string("plain"));
  }
  {
    simdjson::padded_string j(std::string("\"a\\\"b\""));
    std::string v = simdjson::from(j);
    ASSERT_EQUAL(v, std::string("a\"b"));
  }
  {
    simdjson::padded_string j(std::string(R"(["a","bb",""])"));
    std::vector<std::string> v = simdjson::from(j);
    ASSERT_EQUAL(v.size(), 3);
    ASSERT_EQUAL(v[0], std::string("a"));
    ASSERT_EQUAL(v[1], std::string("bb"));
    ASSERT_EQUAL(v[2], std::string(""));
  }
  {
    simdjson::padded_string j(std::string(R"(["a","b\"c"])"));
    std::vector<std::string> v = simdjson::from(j);
    ASSERT_EQUAL(v.size(), 2);
    ASSERT_EQUAL(v[1], std::string("b\"c"));
  }
  {
    simdjson::padded_string j(std::string("[1,-2,3]"));
    std::vector<int64_t> v = simdjson::from(j);
    ASSERT_EQUAL(v.size(), 3);
    ASSERT_EQUAL(v[0], int64_t(1));
    ASSERT_EQUAL(v[1], int64_t(-2));
    ASSERT_EQUAL(v[2], int64_t(3));
  }
  {
    simdjson::padded_string j(std::string("-0"));
    ASSERT_ERROR(simdjson::from(j).result<uint64_t>().error(), simdjson::INCORRECT_TYPE);
    int64_t v = simdjson::from(j);
    ASSERT_EQUAL(v, int64_t(0));
  }
  {
    simdjson::padded_string j(std::string("[\"caf\\u00e9 \\ud83d\\ude00\",\"na\xc3\xafve\",\"a\\/b\"]"));
    std::vector<std::string> v = simdjson::from(j);
    ASSERT_EQUAL(v.size(), 3);
    ASSERT_EQUAL(v[0], std::string("caf\xc3\xa9 \xf0\x9f\x98\x80"));
    ASSERT_EQUAL(v[1], std::string("na\xc3\xafve"));
    ASSERT_EQUAL(v[2], std::string("a/b"));
  }
  {
    simdjson::padded_string j(std::string("01"));
    ASSERT_ERROR(simdjson::from(j).result<int64_t>().error(), simdjson::NUMBER_ERROR);
  }
  {
    // Unsigned values above INT64_MAX are parsed directly, not via On Demand.
    const char *ok[] = {"9223372036854775807", "9223372036854775808",
                        "10000000000000000000", "18446744073709551615", " 18446744073709551615 "};
    const uint64_t ok_values[] = {9223372036854775807ULL, 9223372036854775808ULL,
                                  10000000000000000000ULL, 18446744073709551615ULL, 18446744073709551615ULL};
    for (size_t i = 0; i < sizeof(ok) / sizeof(ok[0]); i++) {
      const std::string json(ok[i]);
      uint64_t direct = 0;
      ASSERT_SUCCESS(simdjson::internal::direct::parse_document<uint64_t>(
          reinterpret_cast<const uint8_t *>(json.data()), json.size(), direct));
      ASSERT_EQUAL(direct, ok_values[i]);
      simdjson::padded_string j(json);
      uint64_t v = simdjson::from(j);
      ASSERT_EQUAL(v, ok_values[i]);
    }
    const char *out_of_range[] = {"18446744073709551616", "20000000000000000000",
                                  "99999999999999999999", "100000000000000000000"};
    for (const char *s : out_of_range) {
      const std::string json(s);
      uint64_t direct = 0;
      ASSERT_ERROR(simdjson::internal::direct::parse_document<uint64_t>(
          reinterpret_cast<const uint8_t *>(json.data()), json.size(), direct), simdjson::UNINITIALIZED);
      simdjson::padded_string j(json);
      simdjson::ondemand::parser parser;
      simdjson::ondemand::document doc;
      uint64_t expected = 0;
      simdjson::error_code expected_error = parser.iterate(j).get(doc);
      if (!expected_error) { expected_error = doc.get_uint64().get(expected); }
      ASSERT_TRUE(expected_error != simdjson::SUCCESS);
      ASSERT_ERROR(simdjson::from(j).result<uint64_t>().error(), expected_error);
    }
  }
  TEST_SUCCEED();
}

// from() into the container C: the array ["x", <string>] must give the error that On Demand gave for the
// string alone, and contain that string.
template <typename C>
bool array_of_strings_matches(const simdjson::padded_string &array, simdjson::error_code expected_error,
                              const std::string &expected_second) {
  C got;
  ASSERT_EQUAL(simdjson::from(array).get(got), expected_error);
  if (!expected_error) {
    // A set keeps one of two equal strings and has its own order.
    const bool merged = expected_second == "x" &&
        (std::is_same_v<C, std::set<std::string>> || std::is_same_v<C, std::unordered_set<std::string>>);
    ASSERT_EQUAL(got.size(), merged ? 1 : 2);
    ASSERT_TRUE(std::ranges::find(got, expected_second) != got.end());
  }
  return true;
}

bool test_direct_from_string_scan() {
  TEST_START();
  // The direct string scan reads eight bytes at a time. Every length, every
  // position of a byte that needs the full parser, and bytes next to the
  // scan's thresholds must give the On Demand result.
  auto ondemand_string = [](const std::string &json, std::string &out) {
    simdjson::ondemand::parser parser;
    simdjson::padded_string padded(json);
    simdjson::ondemand::document doc;
    auto error = parser.iterate(padded).get(doc);
    if (error) { return error; }
    return doc.get_string(out);
  };
  const char edge[] = {' ', '!', '#', '[', ']', '\x7f', 'a', 'Z'};
  uint32_t seed = 12345;
  for (size_t n = 0; n <= 80; n++) {
    std::string body;
    for (size_t i = 0; i < n; i++) {
      seed = seed * 1103515245 + 12345;
      body += edge[(seed >> 16) % sizeof(edge)];
    }
    simdjson::padded_string single("\"" + body + "\"");
    std::string s = simdjson::from(single);
    ASSERT_EQUAL(s, body);
    simdjson::padded_string pair("[\"" + body + "\", \"" + body + "x\"]");
    std::vector<std::string> v = simdjson::from(pair);
    ASSERT_EQUAL(v.size(), 2);
    ASSERT_EQUAL(v[0], body);
    ASSERT_EQUAL(v[1], body + "x");
  }
  const char *specials[] = {"\\n", "\\\"", "\\u00e9", "\xc3\xa9", "\x01", "\x1f", "\xff",
                            "\\/", "\\\\", "\\b\\f\\r\\t", "\\u0000", "\\uD83D\\uDE00", "\\ud83d",
                            "\\udc00", "\\ud83d\\u0041", "\\ud83dx", "\\uzzzz", "\\u12", "\\x", "\\\x01",
                            "\xf0\x9f\x98\x80", "\xe2\x82", "\xc0\xaf", "\xed\xa0\x80", "\\\xc3\xa9"};
  for (size_t n = 1; n <= 40; n++) {
    for (size_t k = 0; k < n; k++) {
      for (const char *special : specials) {
        std::string json = "\"";
        json.append(k, 'a');
        json.append(special);
        json.append(n - k - 1, 'b');
        json.append("\"");
        std::string expected;
        auto expected_error = ondemand_string(json, expected);
        simdjson::padded_string padded(json);
        std::string got;
        auto got_error = simdjson::from(padded).get(got);
        ASSERT_EQUAL(got_error, expected_error);
        if (!expected_error) { ASSERT_EQUAL(got, expected); }
        simdjson::padded_string array("[\"x\"," + json + "]");
        ASSERT_TRUE(array_of_strings_matches<std::vector<std::string>>(array, expected_error, expected));
        ASSERT_TRUE(array_of_strings_matches<std::deque<std::string>>(array, expected_error, expected));
        ASSERT_TRUE(array_of_strings_matches<std::list<std::string>>(array, expected_error, expected));
        ASSERT_TRUE(array_of_strings_matches<std::set<std::string>>(array, expected_error, expected));
        ASSERT_TRUE(array_of_strings_matches<std::unordered_set<std::string>>(array, expected_error, expected));
      }
    }
  }
  TEST_SUCCEED();
}

// from() into the container C must return the same values (bit for bit) and the same error as On Demand.
template <typename C>
bool direct_double_array_matches(const simdjson::padded_string_view view) {
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  C expected;
  auto expected_error = parser.iterate(view).get(doc);
  if (!expected_error) {
    expected_error = doc.get<C>().get(expected);
  }

  C got;
  auto got_error = simdjson::from(view).get(got);
  if (got_error != expected_error) {
    std::cerr << "error mismatch on " << std::string(view.data(), view.size()) << ": " << got_error << " vs " << expected_error << std::endl;
    return false;
  }

  const auto same_bits = [](double a, double b) { return std::memcmp(&a, &b, sizeof(double)) == 0; };
  if (!expected_error && !std::ranges::equal(got, expected, same_bits)) {
    std::cerr << "value mismatch on " << std::string(view.data(), view.size()) << std::endl;
    return false;
  }
  return true;
}

bool test_direct_from_double_array() {
  TEST_START();
  // from<vector<double>> parses the array directly, and so does every other container On Demand can append
  // to. It must return the same values (bit for bit) and the same errors as On Demand.
  auto check = [](const simdjson::padded_string_view view) {
    return direct_double_array_matches<std::vector<double>>(view) &&
           direct_double_array_matches<std::deque<double>>(view) &&
           direct_double_array_matches<std::list<double>>(view) &&
           direct_double_array_matches<std::set<double>>(view) &&
           direct_double_array_matches<std::multiset<double>>(view);
  };
  std::vector<std::string> numbers = {
      "0", "-0", "0.0", "-0.0", "1", "-1", "1.5", "1e2", "1E2", "1e+2", "1e-2", "1.25e-3",
      "9007199254740993", "-9223372036854775809", "123456789012345678901234567890",
      "0.000000000000000000000000000001234567890123456789", "1.7976931348623157e308",
      "1.7976931348623159e308", "1e309", "-1e309", "4.9e-324", "2.2250738585072014e-308",
      "1e-400", "1e00000000000000000000001", "0.1e-99999999999999999999",
      "01", "1.", ".5", "-", "1e", "1e+", "+1", "0x10", "--1", "NaN", "Infinity", "1.5.5",
      "true", "\"1\"", "[1]", "{}", "null"};
  uint32_t seed = 2024;
  auto next = [&seed](uint32_t n) { seed = seed * 1103515245 + 12345; return (seed >> 8) % n; };
  for (int r = 0; r < 3000; r++) {
    std::string s;
    if (next(3) == 0) { s += '-'; }
    size_t int_digits = next(5) == 0 ? 1 + next(25) : 1 + next(4);
    if (next(6) == 0) {
      s += '0';
    } else {
      s += char('1' + next(9));
      for (size_t i = 1; i < int_digits; i++) { s += char('0' + next(10)); }
    }
    if (next(4) != 0) {
      s += '.';
      size_t zeros = next(4) == 0 ? next(12) : 0;
      s += std::string(zeros, '0');
      size_t frac = 1 + next(next(4) == 0 ? 30 : 18);
      for (size_t i = 0; i < frac; i++) { s += char('0' + next(10)); }
    }
    if (next(3) == 0) {
      s += next(2) ? 'e' : 'E';
      if (next(2)) { s += next(2) ? '-' : '+'; }
      size_t exp_digits = 1 + next(3);
      for (size_t i = 0; i < exp_digits; i++) { s += char('0' + next(10)); }
    }
    numbers.push_back(s);
  }
  for (const std::string &n : numbers) {
    const std::string docs[] = {"[" + n + "]", " [ 1.5 , " + n + " ,2 ] ", "[" + n + ",-0.5e1]"};
    for (const std::string &json : docs) {
      simdjson::padded_string padded(json);
      ASSERT_TRUE(check(padded));
    }
  }
  std::string all = "[";
  for (size_t i = 0; i < numbers.size(); i++) {
    if (numbers[i].find_first_not_of("0123456789.eE+-") != std::string::npos) { continue; }
    std::string candidate = "[" + numbers[i] + "]";
    simdjson::padded_string probe(candidate);
    std::vector<double> ok;
    if (simdjson::from(probe).get(ok)) { continue; }
    if (all.size() > 1) { all += next(2) ? "," : " ,\n"; }
    all += numbers[i];
  }
  all += "]";
  simdjson::padded_string big(all);
  ASSERT_TRUE(check(big));
  for (const char *json : {"", "  \n", "[]", " [ ] ", "[", "]", "[1,]", "[,1]", "[1 2]", "[1]x", "[1] ]", "[[1]]"}) {
    simdjson::padded_string padded{std::string(json)};
    ASSERT_TRUE(check(padded));
  }
  // Too little padding: the direct parser is skipped, On Demand reports it.
  std::string tight = "[1.5,2.5]";
  ASSERT_TRUE(check(simdjson::padded_string_view(tight.data(), tight.size(), tight.size())));
  // A cut document whose padding is all digits must not be read past.
  std::string cut = "[1.5" + std::string(simdjson::SIMDJSON_PADDING, '9');
  ASSERT_TRUE(check(simdjson::padded_string_view(cut.data(), 4, cut.size())));
  TEST_SUCCEED();
}

bool test_direct_from_reservation() {
  TEST_START();
  // Containers are sized from their first element. A short first element must
  // not leave a large unused reservation in the returned container.
  const std::string long_string(1 << 20, 'a');
  {
    simdjson::padded_string j("[\"\", \"" + long_string + "\"]");
    std::vector<std::string> v = simdjson::from(j);
    ASSERT_EQUAL(v.size(), 2);
    ASSERT_EQUAL(v[1], long_string);
    ASSERT_TRUE(v.capacity() < 64);
  }
#ifdef __cpp_lib_inplace_vector
  {
    // reserve() of a std::inplace_vector throws beyond its capacity, so it is not called.
    simdjson::padded_string j(std::string("[1.5, 2.5]"));
    std::inplace_vector<double, 2> v;
    ASSERT_SUCCESS(simdjson::from(j).get(v));
    ASSERT_EQUAL(v.size(), 2);
  }
#endif
  const std::string spaces(1 << 20, ' ');
  {
    simdjson::padded_string j("[0," + spaces + "1]");
    std::vector<int64_t> v = simdjson::from(j);
    ASSERT_EQUAL(v.size(), 2);
    ASSERT_TRUE(v.capacity() < 64);
  }
  {
    simdjson::padded_string j("[0," + spaces + "1.5]");
    std::vector<double> v = simdjson::from(j);
    ASSERT_EQUAL(v.size(), 2);
    ASSERT_TRUE(v.capacity() < 64);
  }
  TEST_SUCCEED();
}

// Types derived from std containers, with their own deserializers.
struct upper_strings : std::vector<std::string> {
  friend simdjson::error_code tag_invoke(simdjson::deserialize_tag, auto &val, upper_strings &out) {
    simdjson::ondemand::array array;
    SIMDJSON_TRY(val.get_array().get(array));
    for (auto element : array) {
      std::string_view s;
      SIMDJSON_TRY(element.get_string().get(s));
      std::string upper(s);
      for (char &c : upper) { if (c >= 'a' && c <= 'z') { c = char(c - 'a' + 'A'); } }
      out.push_back(std::move(upper));
    }
    return simdjson::SUCCESS;
  }
};

struct scaled_doubles : std::deque<double> {
  friend simdjson::error_code tag_invoke(simdjson::deserialize_tag, auto &val, scaled_doubles &out) {
    simdjson::ondemand::array array;
    SIMDJSON_TRY(val.get_array().get(array));
    for (auto element : array) {
      double d;
      SIMDJSON_TRY(element.get_double().get(d));
      out.push_back(d * 100);
    }
    return simdjson::SUCCESS;
  }
};

bool test_direct_from_custom_container() {
  TEST_START();
  // from() must use the deserializer of a user type, as doc.get<T>() does,
  // even when the type is derived from a container that from() fills directly.
  static_assert(!simdjson::internal::direct::appendable_container_of<upper_strings, std::string>);
  static_assert(!simdjson::internal::direct::appendable_container_of<scaled_doubles, double>);
  simdjson::padded_string strings(std::string(R"(["abc","def"])"));
  upper_strings u;
  ASSERT_SUCCESS(simdjson::from(strings).get(u));
  ASSERT_EQUAL(u.size(), 2);
  ASSERT_EQUAL(u[0], "ABC");
  ASSERT_EQUAL(u[1], "DEF");
  simdjson::padded_string numbers(std::string("[1.5,2]"));
  scaled_doubles s;
  ASSERT_SUCCESS(simdjson::from(numbers).get(s));
  ASSERT_EQUAL(s.size(), 2);
  ASSERT_EQUAL(s[0], 150.0);
  ASSERT_EQUAL(s[1], 200.0);
  TEST_SUCCEED();
}

bool test_inplace_vector_capacity() {
  TEST_START();
#ifdef __cpp_lib_inplace_vector
  // Too many elements for a std::inplace_vector give CAPACITY, not an exception.
  {
    auto json = R"([1, 2, 3, 4])"_padded;
    std::inplace_vector<uint64_t, 2> v;
    ASSERT_EQUAL(simdjson::from(json).get(v), simdjson::CAPACITY);
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc;
    ASSERT_SUCCESS(parser.iterate(json).get(doc));
    using small_vector = std::inplace_vector<uint64_t, 2>;
    small_vector w;
    ASSERT_EQUAL(doc.get<small_vector>().get(w), simdjson::CAPACITY);
    std::inplace_vector<uint64_t, 4> full;
    ASSERT_SUCCESS(simdjson::from(json).get(full));
    ASSERT_EQUAL(full.size(), 4);
  }
  {
    auto json = R"(["a", "b\n", "c"])"_padded;
    std::inplace_vector<std::string, 2> v;
    ASSERT_EQUAL(simdjson::from(json).get(v), simdjson::CAPACITY);
    std::inplace_vector<std::string, 0> none;
    ASSERT_EQUAL(simdjson::from(json).get(none), simdjson::CAPACITY);
    std::inplace_vector<std::string, 3> full;
    ASSERT_SUCCESS(simdjson::from(json).get(full));
    ASSERT_EQUAL(full[1], "b\n");
  }
  {
    auto json = R"([1.5, 2.5, 3.5])"_padded;
    std::inplace_vector<double, 2> v;
    ASSERT_EQUAL(simdjson::from(json).get(v), simdjson::CAPACITY);
    std::inplace_vector<double, 0> none;
    ASSERT_EQUAL(simdjson::from(json).get(none), simdjson::CAPACITY);
  }
  {
    // An empty array fits in a zero-capacity vector.
    auto json = R"([])"_padded;
    std::inplace_vector<double, 0> none;
    ASSERT_SUCCESS(simdjson::from(json).get(none));
  }
#endif
  TEST_SUCCEED();
}

bool test_to_vs_from_equivalence() {
  TEST_START();
  // Test 3: Verify that simdjson::to<> and simdjson::from behave equivalently
  // Both are instances of to_adaptor - from is just to<void>

  // These should produce identical auto_parser objects
  auto parser1 = simdjson::from(json_car);
  // simdjson::from is an alias for simdjson::to<void>
  auto parser2 = simdjson::from(json_car); // Same as parser1

  // Both should parse the same way
  Car car1 = parser1;
  Car car2 = parser2;

  if (car1.make != car2.make || car1.model != car2.model || car1.year != car2.year) {
    return false;
  }

  TEST_SUCCEED();
}

#endif // SIMDJSON_EXCEPTIONS
bool run() {
  return
#if SIMDJSON_STATIC_REFLECTION && SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
      missing_key_optional_player() && missing_key_player() && meeting_time_test() && meeting_test() && bad_player() && good_player() && complicated_weather_test() &&
#endif
#if SIMDJSON_SUPPORTS_CONCEPTS
      simple_no_except() &&
#endif
#if SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
      broken() && simple() && simple_optional() && with_parser() && to_array() &&
      to_bad_array() &&
      test_to_adaptor_with_single_value() && test_to_vs_from_equivalence() &&
      test_direct_from_atoms() && test_direct_from_string_scan() && test_direct_from_double_array() &&
      test_direct_from_reservation() && test_direct_from_custom_container() && test_inplace_vector_capacity() &&
      example_with_parser() &&
#if SIMDJSON_SUPPORTS_RANGES_FROM
      test_basic_adaptor_with_parser() && test_no_errors() && test_basic_adaptor()
      && test_to_adaptor_basic() && to_clean_array() && to_array_shortcut() &&
#endif // SIMDJSON_SUPPORTS_RANGES_FROM
#endif // SIMDJSON_EXCEPTIONS && SIMDJSON_SUPPORTS_CONCEPTS
      true;
}

} // namespace convert_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, convert_tests::run);
}
#else
int main() { return 0; }
#endif
