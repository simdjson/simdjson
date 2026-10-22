/**
 * refer to pathcheck.cpp
 */

#include <iostream>
#include <string>
#include <vector>

using namespace std::string_literals;

#include "simdjson.h"
#include "test_macros.h"

// we define our own asserts to get around NDEBUG
#ifndef ASSERT
#define ASSERT(x)                                                              \
  {                                                                            \
    if (!(x)) {                                                                \
      std::cerr << "Failed assertion " << #x << std::endl;                     \
      return false;                                                            \
    }                                                                          \
  }
#endif

using namespace simdjson;

bool demo() {
#if SIMDJSON_EXCEPTIONS
  std::cout << "demo test" << std::endl;
  auto cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;
  dom::parser parser;
  dom::element cars = parser.parse(cars_json);
  double x = cars.at_path("$[0].tire_pressure[1]");
  if (x != 39.9)
    return false;
  // Iterating through an array of objects
  std::vector<double> measured;
  for (dom::element car_element : cars) {
    dom::object car;
    simdjson::error_code error;
    if ((error = car_element.get(car))) {
      std::cerr << error << std::endl;
      return false;
    }
    double x3 = car.at_path("$.tire_pressure[1]");
    measured.push_back(x3);
  }
  std::vector<double> expected = {39.9, 31, 30};
  if (measured != expected) {
    return false;
  }
#endif
  return true;
}

const padded_string TEST_JSON = R"(
  {
    "/~01abc": [
      0,
      {
        "\\\" 0": [
          "value0",
          "value1"
        ]
      }
    ],
    "0": "0 ok",
    "01": "01 ok",
    "": "empty ok",
    "arr": []
  }
)"_padded;

const padded_string TEST_RFC_JSON = R"(
   {
      "foo": ["bar", "baz"],
      "": 0,
      "a/b": 1,
      "c%d": 2,
      "e^f": 3,
      "g|h": 4,
      "i\\j": 5,
      "k\"l": 6,
      " ": 7,
      "m~n": 8
   }
)"_padded;

bool run_success_test(const padded_string &source, const char *json_path,
                      std::string_view expected_value) {
  std::cout << "Running successful JSONPath test '" << json_path << "' ..."
            << std::endl;
  dom::parser parser;
  dom::element doc;
  auto error = parser.parse(source).get(doc);
  if (error) {
    std::cerr << "cannot parse: " << error << std::endl;
    return false;
  }
  dom::element answer;
  error = doc.at_path(json_path).get(answer);
  if (error) {
    std::cerr << "cannot access pointer: " << error << std::endl;
    return false;
  }
  std::string str_answer = simdjson::minify(answer);
  if (str_answer != expected_value) {
    std::cerr << "They differ!!!" << std::endl;
    std::cerr << "   found    '" << str_answer << "'" << std::endl;
    std::cerr << "   expected '" << expected_value << "'" << std::endl;
  }
  ASSERT_EQUAL(str_answer, expected_value);
  return true;
}

bool run_failure_test(const padded_string &source, const char *json_path,
                      error_code expected_error) {
  std::cout << "Running invalid JSONPath test '" << json_path << "' ..."
            << std::endl;
  dom::parser parser;
  ASSERT_ERROR(parser.parse(source).at_path(json_path).error(), expected_error);
  return true;
}

bool demo_relative_path() {
  TEST_START();
  auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;

  dom::parser parser;
  dom::element cars;
  std::vector<double> measured;
  auto error = parser.parse(cars_json).get(cars);
  if (error) {
    std::cerr << "cannot parse: " << error << std::endl;
    return false;
  }
  dom::array cars_array;
  error = cars.get(cars_array);
  if (error) {
    std::cerr << "cannot get array: " << error << std::endl;
    return false;
  }
  for (auto car_element : cars_array) {
    double x;
    ASSERT_SUCCESS(car_element.at_path(".tire_pressure[1]").get(x));
    measured.push_back(x);
  }

  std::vector<double> expected = {39.9, 31, 30};
  if (measured != expected) {
    return false;
  }
  TEST_SUCCEED();
}

bool many_json_paths() {
  TEST_START();
  auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;

  dom::parser parser;
  dom::element cars;
  std::vector<double> measured;
  ASSERT_SUCCESS(parser.parse(cars_json).get(cars));
  for (int i = 0; i < 3; i++) {
    double x;
    std::string json_path = std::string("$[") + std::to_string(i) +
                            std::string("].tire_pressure[1]");
    ASSERT_SUCCESS(cars.at_path(json_path).get(x));
    measured.push_back(x);
  }

  std::vector<double> expected = {39.9, 31, 30};
  if (measured != expected) {
    return false;
  }
  TEST_SUCCEED();
}

bool many_json_paths_object_array() {
  TEST_START();
  auto dogcatpotato =
      R"( { "dog" : [1,2,3], "cat" : [5, 6, 7], "potato" : [1234]})"_padded;

  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(dogcatpotato).get(doc));
  dom::object obj;
  ASSERT_SUCCESS(doc.get_object().get(obj));
  int64_t x;
  ASSERT_SUCCESS(obj.at_path("$.dog[1]").get(x));
  ASSERT_EQUAL(x, 2);
  ASSERT_SUCCESS(obj.at_path("$.potato[0]").get(x));
  ASSERT_EQUAL(x, 1234);
  TEST_SUCCEED();
}
bool many_json_paths_object() {
  TEST_START();
  auto cfoofoo2 =
      R"( { "c" :{ "foo": { "a": [ 10, 20, 30 ] }}, "d": { "foo2": { "a": [ 10, 20, 30 ] }} , "e": 120 })"_padded;
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(cfoofoo2).get(doc));
  dom::object obj;
  ASSERT_SUCCESS(doc.get_object().get(obj));
  int64_t x;
  ASSERT_SUCCESS(obj.at_path("$.c.foo.a[1]").get(x));
  ASSERT_EQUAL(x, 20);
  ASSERT_SUCCESS(obj.at_path("$.d.foo2.a.2").get(x));
  ASSERT_EQUAL(x, 30);
  ASSERT_SUCCESS(obj.at_path("$.e").get(x));
  ASSERT_EQUAL(x, 120);
  TEST_SUCCEED();
}
bool many_json_paths_array() {
  TEST_START();
  auto cfoofoo2 =
      R"( [ 111, 2, 3, { "foo": { "a": [ 10, 20, 33 ] }}, { "foo2": { "a": [ 10, 20, 30 ] }}, 1001 ])"_padded;
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(cfoofoo2).get(doc));
  dom::array arr;
  ASSERT_SUCCESS(doc.get_array().get(arr));
  int64_t x;
  ASSERT_SUCCESS(arr.at_path("$[3].foo.a[1]").get(x));
  ASSERT_EQUAL(x, 20);
  TEST_SUCCEED();
}
struct car_type {
  std::string make;
  std::string model;
  uint64_t year;
  std::vector<double> tire_pressure;
  car_type(std::string_view _make, std::string_view _model, uint64_t _year,
           std::vector<double> &&_tire_pressure)
      : make{_make}, model{_model}, year(_year), tire_pressure(_tire_pressure) {
  }
};

bool json_path_invalidation() {
  TEST_START();
  auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;

  dom::parser parser;
  dom::element cars;
  std::vector<double> measured;
  ASSERT_SUCCESS(parser.parse(cars_json).get(cars));
  std::vector<car_type> content;
  for (int i = 0; i < 3; i++) {
    dom::object obj;
    std::string json_path =
        std::string("$[") + std::to_string(i) + std::string("]");
    // Each successive at_path call invalidates
    // previously parsed values, strings, objects and array.
    ASSERT_SUCCESS(cars.at_path(json_path).get(obj));
    // We materialize the object.
    std::string_view make;
    ASSERT_SUCCESS(obj["make"].get(make));
    std::string_view model;
    ASSERT_SUCCESS(obj["model"].get(model));
    uint64_t year;
    ASSERT_SUCCESS(obj["year"].get(year));
    // We materialize the array.
    dom::array arr;
    ASSERT_SUCCESS(obj["tire_pressure"].get(arr));
    std::vector<double> values;
    for (auto x : arr) {
      double value_double;
      ASSERT_SUCCESS(x.get(value_double));
      values.push_back(value_double);
    }
    content.emplace_back(make, model, year, std::move(values));
  }
  std::string expected[] = {"Toyota", "Kia", "Toyota"};
  int i = 0;
  for (car_type c : content) {
    std::cout << c.make << " " << c.model << " " << c.year << "\n";
    ASSERT_EQUAL(expected[i++], c.make);
  }
  TEST_SUCCEED();
}

bool json_path_with_wildcard() {
  TEST_START();

  simdjson::padded_string json_string = R"(
  {
    "firstName": "John",
    "lastName" : "doe",
    "age"      : 26,
    "address"  : {
      "streetAddress": "naist street",
      "city"         : "Nara",
      "postalCode"   : "630-0192"
    },
    "phoneNumbers": [
      {
        "type"  : "iPhone",
        "numbers": [
          "0123-4567-8888",
          "0123-4567-8788",
          "0123-4567-8887"
        ]
      },
      {
        "type"  : "home",
        "numbers": [
          "0123-4567-8910",
          "0123-4267-8910",
          "0103-4567-8910"
        ]
      },
      { },
      {
        "type": "office",
        "numbers": [ ]
      }
    ],
    "empty_object": { },
    "empty_array": [ ]
  })"_padded;

  dom::parser parser;
  dom::element parsed_json;
  ASSERT_SUCCESS(parser.parse(json_string).get(parsed_json));
  std::vector<dom::element> values;


  std::string_view string_value;
  std::uint64_t num_value;
  dom::object obj;
  ASSERT_EQUAL(parsed_json.at_path_with_wildcard("$").error(), INVALID_JSON_POINTER);
  ASSERT_EQUAL(parsed_json.at_path_with_wildcard("1").error(), INVALID_JSON_POINTER);
  ASSERT_EQUAL(parsed_json.at_path_with_wildcard("2").error(), INVALID_JSON_POINTER);
  ASSERT_EQUAL(parsed_json.at_path_with_wildcard("a").error(), INVALID_JSON_POINTER);
  ASSERT_EQUAL(parsed_json.at_path_with_wildcard("$2").error(), INVALID_JSON_POINTER);
  ASSERT_EQUAL(parsed_json.at_path_with_wildcard("$a").error(), INVALID_JSON_POINTER);
  // $.*
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.*").get(values));

  ASSERT_SUCCESS(values[0].get(string_value));
  ASSERT_EQUAL(string_value, "John");

  ASSERT_SUCCESS(values[1].get(string_value));
  ASSERT_EQUAL(string_value, "doe");

  ASSERT_SUCCESS(values[2].get(num_value));
  ASSERT_EQUAL(num_value, 26);

  ASSERT_SUCCESS(values[3].get(obj));
  ASSERT_SUCCESS(obj["streetAddress"].get(string_value));
  ASSERT_EQUAL(string_value, "naist street");

  ASSERT_SUCCESS(obj["city"].get(string_value));
  ASSERT_EQUAL(string_value, "Nara");

  // $[*]
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$[*]").get(values));

  ASSERT_SUCCESS(values[0].get(string_value));
  ASSERT_EQUAL(string_value, "John");

  ASSERT_SUCCESS(values[1].get(string_value));
  ASSERT_EQUAL(string_value, "doe");

  ASSERT_SUCCESS(values[2].get(num_value));
  ASSERT_EQUAL(num_value, 26);

  ASSERT_SUCCESS(values[3].get(obj));
  ASSERT_SUCCESS(obj["streetAddress"].get(string_value));
  ASSERT_EQUAL(string_value, "naist street");

  ASSERT_SUCCESS(obj["city"].get(string_value));
  ASSERT_EQUAL(string_value, "Nara");

  // $.address.*
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.address.*").get(values));

  std::vector<std::string> expected = {"naist street", "Nara", "630-0192"};
  for (int i = 0; i < 3; i++) {
    ASSERT_SUCCESS(values[i].get(string_value));
    ASSERT_EQUAL(string_value, expected[i]);
  }

  // $.*.streetAddress
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.*.streetAddress").get(values));

  ASSERT_SUCCESS(values[0].get(string_value));
  ASSERT_EQUAL(string_value, "naist street");

  // $.phoneNumbers[*].numbers[*]
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.phoneNumbers[*].numbers[*]").get(values));

  std::vector<std::string> expected_numbers = {
    "0123-4567-8888",
    "0123-4567-8788",
    "0123-4567-8887",
    "0123-4567-8910",
    "0123-4267-8910",
    "0103-4567-8910"
  };

  for (int i = 0; i < 6; i++) {
    ASSERT_SUCCESS(values[i].get(string_value));
    ASSERT_EQUAL(string_value, expected_numbers[i]);
  }

  // $.phoneNumbers[*].numbers[1]
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.phoneNumbers[*].numbers[1]").get(values));

  ASSERT_SUCCESS(values[0].get(string_value));
  ASSERT_EQUAL(string_value, expected_numbers[1]);

  ASSERT_SUCCESS(values[1].get(string_value));
  ASSERT_EQUAL(string_value, expected_numbers[4]);

  // $.empty_object.*
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.empty_object.*").get(values));

  ASSERT_EQUAL(values.size(), 0);

  // $.empty_array.*
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.empty_array.*").get(values));
  ASSERT_EQUAL(values.size(), 0);

  // $.phoneNumbers.*.numbers[3]
  ASSERT_SUCCESS(parsed_json.at_path_with_wildcard("$.phoneNumbers.*.numbers[3]").get(values));

  ASSERT_EQUAL(values.size(), 0);

  TEST_SUCCEED();
}

// https://github.com/simdjson/simdjson/pull/2800
// Unterminated bracket-quoted keys used to throw std::out_of_range from
// get_next_key_and_json_path, aborting noexcept at_path_with_wildcard.
bool unterminated_bracket_quote_wildcard() {
  std::cout << "Running unterminated_bracket_quote_wildcard test" << std::endl;
  auto json = R"({"a": [1, 2, 3], "address": {"city": "Nara"}, "ok": 1})"_padded;
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(json).get(doc));

  // Paths that contain '*' take the wildcard branch and call
  // get_next_key_and_json_path. Malformed first-segment bracket-quotes must
  // surface INVALID_JSON_POINTER (and must not abort).
  const char *first_segment_malformed[] = {
      R"($["a*)",
      R"($['a*)",
      R"($["a"*)",
      R"($['a'*)",
      R"($["unterminated*)",
      R"($['unterminated*)",
      R"($["*)",
      R"($['*)",
      R"($["a"*)", // quote closed but ']' missing before '*'
      // A malformed later segment is reported too, not skipped as a mismatch.
      R"($["a"]["b*)",
      R"($["a"]['b*)",
  };
  for (const char *path : first_segment_malformed) {
    std::cout << "  malformed path: " << path << std::endl;
    ASSERT_EQUAL(doc.at_path_with_wildcard(path).error(), INVALID_JSON_POINTER);
  }

  // ok is a scalar: the wildcard suffix yields an empty result.
  {
    std::vector<dom::element> values;
    ASSERT_SUCCESS(doc.at_path_with_wildcard(R"($["ok"].*)").get(values));
    ASSERT_EQUAL(values.size(), 0);
  }

  // Well-formed bracket-quoted segments still work with wildcards.
  std::vector<dom::element> values;
  ASSERT_SUCCESS(doc.at_path_with_wildcard(R"($["address"].*)").get(values));
  ASSERT_EQUAL(values.size(), 1);
  std::string_view city;
  ASSERT_SUCCESS(values[0].get(city));
  ASSERT_EQUAL(city, "Nara");

  ASSERT_SUCCESS(doc.at_path_with_wildcard(R"($['address'].*)").get(values));
  ASSERT_EQUAL(values.size(), 1);
  ASSERT_SUCCESS(values[0].get(city));
  ASSERT_EQUAL(city, "Nara");

  // Non-wildcard at_path uses json_path_to_pointer_conversion (already guarded
  // for a missing ']'; quotes are not stripped there, so unterminated
  // forms are still INVALID_JSON_POINTER).
  ASSERT_EQUAL(doc.at_path(R"($["a")").error(), INVALID_JSON_POINTER);
  ASSERT_EQUAL(doc.at_path(R"($['a')").error(), INVALID_JSON_POINTER);
  int64_t ok_val = 0;
  ASSERT_SUCCESS(doc.at_path("$.ok").get(ok_val));
  ASSERT_EQUAL(ok_val, 1);

  TEST_SUCCEED();
}

// Keys containing '/' or '~' must be escaped when the segments before a
// wildcard become JSON Pointer steps, as at_path already does for the same
// keys (see the RFC 6901 cases "$.a/b" and "$.m~n" below): an unescaped '/'
// made $.a/b.* resolve through "a" then "b" instead of the key "a/b", and an
// unescaped '~' made the step an invalid pointer, silently matching nothing.
bool wildcard_key_with_slash_or_tilde() {
  TEST_START();
  auto json = R"({"a":{"b":{"nested":1}},"a/b":{"x":2},"m~n":{"y":3}})"_padded;
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(json).get(doc));
  std::vector<dom::element> values;
  int64_t value;

  // Must match the key "a/b", not descend into {"a":{"b":...}}.
  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.a/b.*").get(values));
  ASSERT_EQUAL(values.size(), 1);
  ASSERT_SUCCESS(values[0].get(value));
  ASSERT_EQUAL(value, 2);

  ASSERT_SUCCESS(doc.at_path_with_wildcard(R"($['a/b'].*)").get(values));
  ASSERT_EQUAL(values.size(), 1);
  ASSERT_SUCCESS(values[0].get(value));
  ASSERT_EQUAL(value, 2);

  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.m~n.*").get(values));
  ASSERT_EQUAL(values.size(), 1);
  ASSERT_SUCCESS(values[0].get(value));
  ASSERT_EQUAL(value, 3);

  TEST_SUCCEED();
}

// A '*' inside the final segment is part of the key, as in at_path and
// On-Demand. It used to be reported as INVALID_JSON_POINTER because the
// parser looped once more on the empty remainder.
bool star_in_final_segment() {
  std::cout << "Running star_in_final_segment test" << std::endl;
  auto json = R"({"a":{"b*":{"c":7}},"foo*":9,"d":[{"b*":2},{"b*":3},{"x":4}]})"_padded;
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(json).get(doc));

  int64_t scalar = 0;
  ASSERT_SUCCESS(doc.at_path("$.a.b*.c").get(scalar));
  ASSERT_EQUAL(scalar, 7);
  ASSERT_SUCCESS(doc.at_path("$.foo*").get(scalar));
  ASSERT_EQUAL(scalar, 9);

  std::vector<dom::element> values;
  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.a.b*").get(values));
  ASSERT_EQUAL(values.size(), 1);
  ASSERT_SUCCESS(values[0].at_path("$.c").get(scalar));
  ASSERT_EQUAL(scalar, 7);

  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.a['b*']").get(values));
  ASSERT_EQUAL(values.size(), 1);
  ASSERT_SUCCESS(values[0].at_path("$.c").get(scalar));
  ASSERT_EQUAL(scalar, 7);

  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.a.b*.c").get(values));
  ASSERT_EQUAL(values.size(), 1);
  ASSERT_SUCCESS(values[0].get(scalar));
  ASSERT_EQUAL(scalar, 7);

  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.foo*").get(values));
  ASSERT_EQUAL(values.size(), 1);
  ASSERT_SUCCESS(values[0].get(scalar));
  ASSERT_EQUAL(scalar, 9);

  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.d[*]['b*']").get(values));
  ASSERT_EQUAL(values.size(), 2);
  ASSERT_SUCCESS(values[0].get(scalar));
  ASSERT_EQUAL(scalar, 2);
  ASSERT_SUCCESS(values[1].get(scalar));
  ASSERT_EQUAL(scalar, 3);

  // A quoted '*' is the wildcard, matching On-Demand, so this is every child.
  ASSERT_SUCCESS(doc.at_path_with_wildcard("$['*']").get(values));
  ASSERT_EQUAL(values.size(), 3);
  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.*").get(values));
  ASSERT_EQUAL(values.size(), 3);

  // A key that is not present is an empty match, not a parse error.
  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.missing*").get(values));
  ASSERT_EQUAL(values.size(), 0);

  // A truncated bracket after a real key is still a parse error.
  ASSERT_EQUAL(doc.at_path_with_wildcard("$.d[*]['b*'][").error(), INVALID_JSON_POINTER);
  ASSERT_EQUAL(doc.at_path_with_wildcard(R"($.d[*]['b)").error(), INVALID_JSON_POINTER);

  TEST_SUCCEED();
}

// A path that cannot be parsed after a wildcard is reported, whatever the
// document, instead of silently matching nothing.
bool malformed_path_after_wildcard() {
  std::cout << "Running malformed_path_after_wildcard test" << std::endl;
  const char *malformed[] = {
      R"($.a[*]['b)",
      R"($.a[*][1)",
      R"($.a[*].b[)",
      R"($.a[*]b)",
      R"($.*.b[0)",
      R"($.a[*][*)",
      R"($[*]x)",
  };
  for (const char *source : {R"({"a":[{"b":[1]},{"b":[2]}]})", R"([[{"b":[1]}]])", "[]", "5"}) {
    const padded_string json(std::string_view{source});
    dom::parser parser;
    dom::element doc;
    ASSERT_SUCCESS(parser.parse(json).get(doc));
    for (const char *path : malformed) {
      std::cout << "  " << json << " " << path << std::endl;
      ASSERT_EQUAL(doc.at_path_with_wildcard(path).error(), INVALID_JSON_POINTER);
    }
  }
  // As in at_path, a bracketed segment after the last wildcard is read as a
  // JSON Pointer token: [b] is the key "b".
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(R"({"a":[{"b":1},{"b":2}]})"_padded).get(doc));
  std::vector<dom::element> values;
  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.a[*][b]").get(values));
  ASSERT_EQUAL(values.size(), 2);
  TEST_SUCCEED();
}

// Deterministic fuzz-style sweep of truncated / malformed JSONPath strings.
// Ensures at_path_with_wildcard never aborts on incomplete bracket-quotes.
bool json_path_malformed_deterministic_fuzz() {
  std::cout << "Running json_path_malformed_deterministic_fuzz test" << std::endl;
  auto json = R"({"a":{"b":[1,2,3]},"x":1,"*":true})"_padded;
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(json).get(doc));

  // Seeds that exercise quote / bracket / wildcard combinations.
  const char *seeds[] = {
      R"($["a*)",
      R"($['a*)",
      R"($["a"][*])",
      R"($['a'][*])",
      R"($[*]["b*)",
      R"($[*]['b*)",
      R"($["a"]["b*)",
      R"($["*")",
      R"($["*"]*)",
      R"($[*].*)",
      R"($["a"*].*)",
      R"($.a["b*)",
      R"($[")",
      R"($[')",
      R"($["])",
      R"($['])",
      R"($[""])",
      R"($[''])",
      R"($["a])",
      R"($['a])",
  };

  for (const char *seed : seeds) {
    const std::string full(seed);
    // Try every non-empty prefix so truncation of a once-valid path is covered.
    for (size_t len = 1; len <= full.size(); len++) {
      const std::string path = full.substr(0, len);
      // Must return an error code (or empty success), never throw/abort.
      auto result = doc.at_path_with_wildcard(path);
      (void)result.error();
    }
  }

  // A few fully valid paths mixed in to keep the happy path warm.
  std::vector<dom::element> values;
  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.*").get(values));
  ASSERT_TRUE(values.size() >= 1);
  ASSERT_SUCCESS(doc.at_path_with_wildcard(R"($["a"].*)").get(values));
  ASSERT_TRUE(values.size() >= 1);

  TEST_SUCCEED();
}

// for 0.5 version and following (standard compliant)
bool modern_support() {
#if SIMDJSON_EXCEPTIONS
  std::cout << "modern test" << std::endl;
  auto example_json = R"({"key": "value", "array": [0, 1, 2]})"_padded;
  dom::parser parser;
  dom::element example = parser.parse(example_json);
  std::string_view value_str = example.at_path("$.key");
  ASSERT_EQUAL(value_str, "value");
  int64_t array0 = example.at_path("$.array[0]");
  ASSERT_EQUAL(array0, 0);
  array0 = example.at_path("$.array").at_path("$[0]");
  ASSERT_EQUAL(array0, 0);
  ASSERT_ERROR(example.at_path("$.no_such_key").error(), NO_SUCH_FIELD);
  ASSERT_ERROR(example.at_path("$.array[9]").error(), INDEX_OUT_OF_BOUNDS);
  ASSERT_ERROR(example.at_path("$.array.not_a_num").error(), INCORRECT_TYPE);
  ASSERT_ERROR(example.at_path("$.array.").error(), INVALID_JSON_POINTER);
#endif
  return true;
}

bool at_path_error_propagation() {
  TEST_START();
  const auto json = R"({"array": [1], "object": {"key": 2}})"_padded;
  dom::parser parser;
  auto doc = parser.parse(json);
  ASSERT_SUCCESS(doc.error());

  // Preserve the original error regardless of whether the next path is valid.
  for (std::string_view path : {"$.key", "$[0]", "invalid", "$["}) {
    ASSERT_ERROR(doc["missing"].at_path(path), NO_SUCH_FIELD);
    ASSERT_ERROR(doc["missing"].get_array().at_path(path), NO_SUCH_FIELD);
    ASSERT_ERROR(doc["missing"].get_object().at_path(path), NO_SUCH_FIELD);
    ASSERT_ERROR(doc.get_array().at_path(path), INCORRECT_TYPE);
    ASSERT_ERROR(doc["array"].get_object().at_path(path), INCORRECT_TYPE);
    ASSERT_ERROR(doc["array"].at(1).at_path(path), INDEX_OUT_OF_BOUNDS);
  }

  // Successful results must still validate paths and return selected values.
  ASSERT_ERROR(doc.at_path("invalid"), INVALID_JSON_POINTER);
  ASSERT_ERROR(doc["array"].get_array().at_path("invalid"), INVALID_JSON_POINTER);
  ASSERT_ERROR(doc["object"].get_object().at_path("invalid"), INVALID_JSON_POINTER);
  int64_t value;
  ASSERT_SUCCESS(doc.at_path("$.object.key").get_int64().get(value));
  ASSERT_EQUAL(value, 2);
  ASSERT_SUCCESS(doc["array"].get_array().at_path("$[0]").get_int64().get(value));
  ASSERT_EQUAL(value, 1);
  ASSERT_SUCCESS(doc["object"].get_object().at_path("$.key").get_int64().get(value));
  ASSERT_EQUAL(value, 2);

  const auto invalid_json = "["_padded;
  auto invalid_doc = parser.parse(invalid_json);
  const auto parse_error = invalid_doc.error();
  ASSERT_TRUE(parse_error != SUCCESS);
  ASSERT_ERROR(invalid_doc.at_path("invalid"), parse_error);
  ASSERT_ERROR(invalid_doc.get_array().at_path("invalid"), parse_error);
  ASSERT_ERROR(invalid_doc.get_object().at_path("invalid"), parse_error);
  TEST_SUCCEED();
}

// The path is parsed once, not again at every node the wildcards reach: a long
// path after a couple of wildcards on a large document used to take
// O(nodes * path length) time (OSS-Fuzz 6040346321289216).
bool long_path_after_wildcards() {
  TEST_START();
  std::string json = "[";
  for (size_t i = 0; i < 2000; i++) {
    if (i > 0) { json += ","; }
    json += R"([{"k":[7]},{"k":[8]},{"j":0},3,{"k":[]}])";
  }
  json += "]";
  dom::parser parser;
  dom::element doc;
  ASSERT_SUCCESS(parser.parse(json).get(doc));
  std::vector<dom::element> values;
  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.*.*.k[0]").get(values));
  ASSERT_EQUAL(values.size(), 4000);
  int64_t value;
  ASSERT_SUCCESS(values[0].get_int64().get(value));
  ASSERT_EQUAL(value, 7);
  ASSERT_SUCCESS(values[1].get_int64().get(value));
  ASSERT_EQUAL(value, 8);
  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.*.*.k.*").get(values));
  ASSERT_EQUAL(values.size(), 4000);
  const std::string long_key(1000000, 'x');
  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.*.*." + long_key).get(values));
  ASSERT_EQUAL(values.size(), 0);
  ASSERT_SUCCESS(doc.at_path_with_wildcard("$.*.*.k." + long_key + ".*").get(values));
  ASSERT_EQUAL(values.size(), 0);
  TEST_SUCCEED();
}

int main() {
  if (true && json_path_with_wildcard() && unterminated_bracket_quote_wildcard() &&
      star_in_final_segment() &&
      malformed_path_after_wildcard() &&
      wildcard_key_with_slash_or_tilde() &&
      long_path_after_wildcards() &&
      json_path_malformed_deterministic_fuzz() && demo() && modern_support() &&
      at_path_error_propagation() &&
      run_success_test(TEST_RFC_JSON, "$.foo", "[\"bar\",\"baz\"]") &&
      run_success_test(TEST_RFC_JSON, "$.foo[0]", "\"bar\"") &&
      run_success_test(TEST_RFC_JSON, "$.", "0") &&
      run_success_test(TEST_RFC_JSON, "$.a/b", "1") &&
      run_success_test(TEST_RFC_JSON, "$.c%d", "2") &&
      run_success_test(TEST_RFC_JSON, "$.e^f", "3") &&
      run_success_test(TEST_RFC_JSON, "$.g|h", "4") &&
      run_success_test(TEST_RFC_JSON, "$.i\\j", "5") &&
      run_success_test(TEST_RFC_JSON, "$.k\"l", "6") &&
      run_success_test(TEST_RFC_JSON, "$. ", "7") &&
      run_success_test(TEST_RFC_JSON, "$.m~n", "8") &&
      run_success_test(TEST_JSON, "$./~01abc",
                       R"([0,{"\\\" 0":["value0","value1"]}])") &&
      run_success_test(TEST_JSON, "$./~01abc[1]",
                       R"({"\\\" 0":["value0","value1"]})") &&
      run_success_test(TEST_JSON, "$./~01abc[1].\\\" 0",
                       R"(["value0","value1"])") &&
      run_success_test(TEST_JSON, "$.arr", R"([])") // get array
      &&
      run_failure_test(TEST_JSON, R"($./~01abc[1].\\\" 0[2])", NO_SUCH_FIELD) &&
      run_failure_test(TEST_JSON, "$.arr[0]", INDEX_OUT_OF_BOUNDS) &&
      run_failure_test(TEST_JSON, "/~01abc", INVALID_JSON_POINTER) &&
      run_failure_test(TEST_JSON, ".~1abc", NO_SUCH_FIELD) &&
      run_failure_test(TEST_JSON, "./~01abc.01", INVALID_JSON_POINTER) &&
      run_failure_test(TEST_JSON, "./~01abc.", INVALID_JSON_POINTER) &&
      run_failure_test(TEST_JSON, "./~01abc.-", INDEX_OUT_OF_BOUNDS)) {
    std::cout << "Success!" << std::endl;
    return 0;
  } else {
    std::cerr << "Failed!" << std::endl;
    return 1;
  }
}
