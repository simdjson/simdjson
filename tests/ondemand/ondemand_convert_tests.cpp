#include "simdjson.h"
#include "simdjson/convert.h"
#include "test_ondemand.h"

#include <chrono>
#include <cstring>
#include <map>
#include <ranges>
#include <string>
#include <vector>

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
    std::string title;
    std::vector<std::string> attendees;
    std::optional<std::string> location;
    bool is_recurring;
};


struct MeetingTime {
    std::string title;
    std::chrono::system_clock::time_point start_time;
    std::vector<std::string> attendees;
    std::optional<std::string> location;
    bool is_recurring;
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
  // Single-pass from<> for atoms, int64 arrays, plain strings, and flat
  // int64 objects. Invalid input must surface the same error as On Demand.
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
    simdjson::padded_string j(std::string(R"({"a":1,"b":2,"a":3})"));
    std::map<std::string, int64_t> m = simdjson::from(j);
    ASSERT_EQUAL(m.size(), 2);
    ASSERT_EQUAL(m["a"], int64_t(1)); // first key wins
    ASSERT_EQUAL(m["b"], int64_t(2));
  }
  {
    simdjson::padded_string j(std::string("01"));
    ASSERT_ERROR(simdjson::from(j).result<int64_t>().error(), simdjson::NUMBER_ERROR);
  }
  TEST_SUCCEED();
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
  const char *specials[] = {"\\n", "\\\"", "\\u00e9", "\xc3\xa9", "\x01", "\x1f", "\xff"};
  for (size_t n = 1; n <= 40; n++) {
    for (size_t k = 0; k < n; k++) {
      for (const char *special : specials) {
        const std::string json = "\"" + std::string(k, 'a') + special + std::string(n - k - 1, 'b') + "\"";
        std::string expected;
        auto expected_error = ondemand_string(json, expected);
        simdjson::padded_string padded(json);
        std::string got;
        auto got_error = simdjson::from(padded).get(got);
        ASSERT_EQUAL(got_error, expected_error);
        if (!expected_error) { ASSERT_EQUAL(got, expected); }
        simdjson::padded_string array("[\"x\"," + json + "]");
        std::vector<std::string> got_vec;
        auto vec_error = simdjson::from(array).get(got_vec);
        ASSERT_EQUAL(vec_error, expected_error);
        if (!expected_error) {
          ASSERT_EQUAL(got_vec.size(), 2);
          ASSERT_EQUAL(got_vec[1], expected);
        }
      }
    }
  }
  TEST_SUCCEED();
}

bool test_direct_from_double_array() {
  TEST_START();
  // from<vector<double>> parses the array directly. It must return the same
  // values (bit for bit) and the same errors as On Demand.
  auto check = [](const simdjson::padded_string_view view) {
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document doc;
    std::vector<double> expected;
    auto expected_error = parser.iterate(view).get(doc);
    if (!expected_error) { expected_error = doc.get<std::vector<double>>().get(expected); }
    std::vector<double> got;
    auto got_error = simdjson::from(view).get(got);
    if (got_error != expected_error) {
      std::cerr << "error mismatch on " << std::string(view.data(), view.size()) << ": " << got_error << " vs " << expected_error << std::endl;
      return false;
    }
    if (!expected_error && (got.size() != expected.size() ||
        (!got.empty() && std::memcmp(got.data(), expected.data(), got.size() * sizeof(double)) != 0))) {
      std::cerr << "value mismatch on " << std::string(view.data(), view.size()) << std::endl;
      return false;
    }
    return true;
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
