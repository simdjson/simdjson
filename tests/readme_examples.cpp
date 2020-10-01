#include <iostream>
#include "simdjson.h"

using namespace std;
using namespace simdjson;
using error_code=simdjson::error_code;

void basics_1() {
  const char *filename = "x.txt";

  dom::parser parser;
  dom::element doc = parser.load(filename); // load and parse a file

  cout << doc;
}

void basics_2() {
  dom::parser parser;
  dom::element doc = parser.parse("[1,2,3]"_padded); // parse a string

  cout << doc;
}

void basics_dom_1() {
  auto cars_json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;
  dom::parser parser;

  // Parse and iterate through each car
  for (dom::object car : parser.parse(cars_json)) {
    // Accessing a field by name
    cout << "Make/Model: " << car["make"] << "/" << car["model"] << endl;

    // Casting a JSON element to an integer
    uint64_t year = car["year"];
    cout << "- This car is " << 2020 - year << "years old." << endl;

    // Iterating through an array of floats
    double total_tire_pressure = 0;
    for (double tire_pressure : car["tire_pressure"]) {
      total_tire_pressure += tire_pressure;
    }
    cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;

    // Writing out all the information about the car
    for (auto field : car) {
      cout << "- " << field.key << ": " << field.value << endl;
    }
  }
}



void basics_dom_2() {
  auto cars_json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;
  dom::parser parser;
  dom::element cars = parser.parse(cars_json);
  cout << cars.at_pointer("/0/tire_pressure/1") << endl; // Prints 39.9
  for (dom::element car_element : cars) {
    dom::object car;
    simdjson::error_code error;
    if ((error = car_element.get(car))) { std::cerr << error << std::endl; return; }
    double x = car.at_pointer("/tire_pressure/1");
    cout << x << endl; // Prints 39.9, 31 and 30
  }
}

void basics_dom_3() {
  auto abstract_json = R"( [
    {  "12345" : {"a":12.34, "b":56.78, "c": 9998877}   },
    {  "12545" : {"a":11.44, "b":12.78, "c": 11111111}  }
  ] )"_padded;
  dom::parser parser;

  // Parse and iterate through an array of objects
  for (dom::object obj : parser.parse(abstract_json)) {
    for(const auto& key_value : obj) {
      cout << "key: " << key_value.key << " : ";
      dom::object innerobj = key_value.value;
      cout << "a: " << double(innerobj["a"]) << ", ";
      cout << "b: " << double(innerobj["b"]) << ", ";
      cout << "c: " << int64_t(innerobj["c"]) << endl;
    }
  }
}

void basics_dom_4() {
  auto abstract_json = R"(
    {  "str" : { "123" : {"abc" : 3.14 } } } )"_padded;
  dom::parser parser;
  double v = parser.parse(abstract_json)["str"]["123"]["abc"];
  cout << "number: " << v << endl;
}


namespace treewalk_1 {
  void print_json(dom::element element) {
    switch (element.type()) {
      case dom::element_type::ARRAY:
        cout << "[";
        for (dom::element child : dom::array(element)) {
          print_json(child);
          cout << ",";
        }
        cout << "]";
        break;
      case dom::element_type::OBJECT:
        cout << "{";
        for (dom::key_value_pair field : dom::object(element)) {
          cout << "\"" << field.key << "\": ";
          print_json(field.value);
        }
        cout << "}";
        break;
      case dom::element_type::INT64:
        cout << int64_t(element) << endl;
        break;
      case dom::element_type::UINT64:
        cout << uint64_t(element) << endl;
        break;
      case dom::element_type::DOUBLE:
        cout << double(element) << endl;
        break;
      case dom::element_type::STRING:
        cout << std::string_view(element) << endl;
        break;
      case dom::element_type::BOOL:
        cout << bool(element) << endl;
        break;
      case dom::element_type::NULL_VALUE:
        cout << "null" << endl;
        break;
    }
  }

  void basics_treewalk_1() {
    dom::parser parser;
    print_json(parser.load("twitter.json"));
  }
}

#ifdef SIMDJSON_CPLUSPLUS17
void basics_cpp17_1() {
  padded_string json = R"(  { "foo": 1, "bar": 2 }  )"_padded;
  dom::parser parser;
  dom::object object;
  auto error = parser.parse(json).get(object);
  if (error) { cerr << error << endl; return; }
  for (auto [key, value] : object) {
    cout << key << " = " << value << endl;
  }
}
#endif

void basics_cpp17_2() {
  // C++ 11 version for comparison
  padded_string json = R"(  { "foo": 1, "bar": 2 }  )"_padded;
  dom::parser parser;
  dom::object object;
  auto error = parser.parse(json).get(object);
  if (error) { cerr << error << endl; return; }
  for (dom::key_value_pair field : object) {
    cout << field.key << " = " << field.value << endl;
  }
}

void basics_ndjson() {
  dom::parser parser;
  for (dom::element doc : parser.load_many("x.txt")) {
    cout << doc["foo"] << endl;
  }
  // Prints 1 2 3
}

void basics_ndjson_parse_many() {
  dom::parser parser;
  auto json = R"({ "foo": 1 }
{ "foo": 2 }
{ "foo": 3 })"_padded;
  dom::document_stream docs = parser.parse_many(json);
  for (dom::element doc : docs) {
    cout << doc["foo"] << endl;
  }
}
void implementation_selection_1() {
  cout << "simdjson v" << STRINGIFY(SIMDJSON_VERSION) << endl;
  cout << "Detected the best implementation for your machine: " << simdjson::active_implementation->name();
  cout << "(" << simdjson::active_implementation->description() << ")" << endl;
}

void implementation_selection_2() {
  for (auto implementation : simdjson::available_implementations) {
    cout << implementation->name() << ": " << implementation->description() << endl;
  }
}

void implementation_selection_2_safe() {
  for (auto implementation : simdjson::available_implementations) {
    if(implementation->supported_by_runtime_system()) {
      cout << implementation->name() << ": " << implementation->description() << endl;
    }
  }
}
void implementation_selection_3() {
  cout << simdjson::available_implementations["fallback"]->description() << endl;
}

void implementation_selection_safe() {
  auto my_implementation = simdjson::available_implementations["haswell"];
  if(! my_implementation) { exit(1); }
  if(! my_implementation->supported_by_runtime_system()) { exit(1); }
  simdjson::active_implementation = my_implementation;
}

void implementation_selection_4() {
  // Use the fallback implementation, even though my machine is fast enough for anything
  simdjson::active_implementation = simdjson::available_implementations["fallback"];
}

void performance_1() {
  dom::parser parser;

  // This initializes buffers and a document big enough to handle this JSON.
  dom::element doc = parser.parse("[ true, false ]"_padded);
  cout << doc << endl;

  // This reuses the existing buffers, and reuses and *overwrites* the old document
  doc = parser.parse("[1, 2, 3]"_padded);
  cout << doc << endl;

  // This also reuses the existing buffers, and reuses and *overwrites* the old document
  dom::element doc2 = parser.parse("true"_padded);
  // Even if you keep the old reference around, doc and doc2 refer to the same document.
  cout << doc << endl;
  cout << doc2 << endl;
}

#ifdef SIMDJSON_CPLUSPLUS17
SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
// The web_request part of this is aspirational, so we compile as much as we can here
void performance_2() {
  dom::parser parser(1000*1000); // Never grow past documents > 1MB
  /* for (web_request request : listen()) */ {
    dom::element doc;
    auto error = parser.parse("1"_padded/*request.body*/).get(doc);
    // If the document was above our limit, emit 413 = payload too large
    if (error == CAPACITY) { /* request.respond(413); continue; */ }
    // ...
  }
}

// The web_request part of this is aspirational, so we compile as much as we can here
void performance_3() {
  dom::parser parser(0); // This parser will refuse to automatically grow capacity
  auto error = parser.allocate(1000*1000); // This allocates enough capacity to handle documents <= 1MB
  if (error) { cerr << error << endl; exit(1); }

  /* for (web_request request : listen()) */ {
    dom::element doc;
    auto error = parser.parse("1"_padded/*request.body*/).get(doc);
    // If the document was above our limit, emit 413 = payload too large
    if (error == CAPACITY) { /* request.respond(413); continue; */ }
    // ...
  }
}
SIMDJSON_POP_DISABLE_WARNINGS
#endif

void minify() {
  const char * some_string = "[ 1, 2, 3, 4] ";
  size_t length = std::strlen(some_string);
  std::unique_ptr<char[]> buffer{new char[length]};
  size_t new_length{};
  auto error = simdjson::minify(some_string, length, buffer.get(), new_length);
  if(error != simdjson::SUCCESS) {
    std::cerr << "error " << error << std::endl;
    abort();
  } else {
    const char * expected_string = "[1,2,3,4]";
    size_t expected_length = std::strlen(expected_string);
    if(expected_length != new_length) {
      std::cerr << "mismatched length (error) " << std::endl;
      abort();
    }
    for(size_t i = 0; i < new_length; i++) {
      if(expected_string[i] != buffer.get()[i]) {
        std::cerr << "mismatched content (error) " << std::endl;
        abort();
      }
    }
  }
}

bool is_correct() {
  const char * some_string = "[ 1, 2, 3, 4] ";
  size_t length = std::strlen(some_string);
  bool is_ok = simdjson::validate_utf8(some_string, length);
  return is_ok;
}

bool is_correct_string_view() {
  const char * some_string = "[ 1, 2, 3, 4] ";
  size_t length = std::strlen(some_string);
  std::string_view v(some_string, length);
  bool is_ok = simdjson::validate_utf8(v);
  return is_ok;
}

bool is_correct_string() {
  const std::string some_string = "[ 1, 2, 3, 4] ";
  bool is_ok = simdjson::validate_utf8(some_string);
  return is_ok;
}

void parse_documentation() {
  const char *json      = R"({"key":"value"})";
  const size_t json_len = std::strlen(json);
  simdjson::dom::parser parser;
  simdjson::dom::element element = parser.parse(json, json_len);
  // Next line is to avoid unused warning.
  (void)element;
}


void parse_documentation_lowlevel() {
  // Such low-level code is not generally recommended. Please
  // see parse_documentation() instead.
  // Motivation: https://github.com/simdjson/simdjson/issues/1175
  const char *json      = R"({"key":"value"})";
  const size_t json_len = std::strlen(json);
  std::unique_ptr<char[]> padded_json_copy{new char[json_len + SIMDJSON_PADDING]};
  std::memcpy(padded_json_copy.get(), json, json_len);
  std::memset(padded_json_copy.get() + json_len, '\0', SIMDJSON_PADDING);
  simdjson::dom::parser parser;
  simdjson::dom::element element = parser.parse(padded_json_copy.get(), json_len, false);
  // Next line is to avoid unused warning.
  (void)element;
}

int main() {
  basics_dom_1();
  basics_dom_2();
  basics_dom_3();
  basics_dom_4();
  minify();
  return 0;
}
