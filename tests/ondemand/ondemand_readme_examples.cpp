#include "simdjson.h"
#include "test_ondemand.h"
#if __cpp_lib_optional >= 201606L
#include <optional>
#endif
using namespace std;
using namespace simdjson;
using error_code=simdjson::error_code;

bool string1() {
  const char * data = "my data"; // 7 bytes
  simdjson::padded_string my_padded_data(data, 7); // copies to a padded buffer
  std::cout << my_padded_data << std::endl;
  return true;
}

bool string2() {
  std::string data = "my data";
  simdjson::padded_string my_padded_data(data); // copies to a padded buffer
  std::cout << my_padded_data << std::endl;
  return true;
}


bool to_string_example_no_except() {
    TEST_START();
  auto json = R"({
  "name": "Daniel",
  "age": 42
})"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  auto err = parser.iterate(json).get(doc);
  if(err) { return false; }
  std::string name;
  err = doc["name"].get_string(name);
  if(err) { return false; }
  TEST_SUCCEED();
}

#if SIMDJSON_EXCEPTIONS


bool to_string_example() {
    TEST_START();
  auto json = R"({
  "name": "Daniel",
  "age": 42
})"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  std::string name;
  doc["name"].get_string(name);
  ASSERT_EQUAL(name, "Daniel");
  TEST_SUCCEED();
}

bool gen_raw1() {
  TEST_START();
  simdjson::ondemand::parser parser;
  simdjson::padded_string docdata =  R"({"value":123})"_padded;
  simdjson::ondemand::document doc = parser.iterate(docdata);
  simdjson::ondemand::object obj = doc.get_object();
  string_view token = obj.raw_json(); // gives you `{"value":123}`
  ASSERT_EQUAL(token, R"({"value":123})");
  TEST_SUCCEED();
}

bool gen_raw2() {
  TEST_START();
  simdjson::ondemand::parser parser;
  simdjson::padded_string docdata =  R"([1,2,3])"_padded;
  simdjson::ondemand::document doc = parser.iterate(docdata);
  simdjson::ondemand::array arr = doc.get_array();
  string_view token = arr.raw_json(); // gives you `[1,2,3]`
  ASSERT_EQUAL(token, R"([1,2,3])");
  TEST_SUCCEED();
}

bool gen_raw3() {
  TEST_START();
  simdjson::ondemand::parser parser;
  simdjson::padded_string docdata =  R"({"value":123})"_padded;
  simdjson::ondemand::document doc = parser.iterate(docdata);
  simdjson::ondemand::object obj = doc.get_object();
  string_view token = obj.raw_json(); // gives you `{"value":123}`
  ASSERT_EQUAL(token, R"({"value":123})");
  obj.reset(); // revise the object
  uint64_t x = obj["value"]; // gives me 123
  ASSERT_EQUAL(x, 123);

  TEST_SUCCEED();
}

bool at_end() {
  TEST_START();
  auto json = R"([1, 2] foo ])"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ondemand::array array = doc.get_array();
  for (uint64_t values : array) {
    std::cout << values << std::endl;
  }
  if(!doc.at_end()) {
    std::cerr << "trailing content at byte index " << doc.current_location() - json.data() << std::endl;
  }
  TEST_SUCCEED();
}

bool examplecrt() {
  TEST_START();
  padded_string padded_input_json = R"([
	{ "monitor": [
		{ "id": "monitor",		"type": "toggle",		"label": "monitor"			},
		{ "id": "profile",		"type": "selector",		"label": "collection"		},
		{ "id": "overlay",		"type": "selector",		"label": "overlay"			},
		{ "id": "zoom",			"type": "toggleSlider",	"label": "zoom"				}
	] },

	{ "crt": [
		{ "id": "system",		"type": "multi",		"label": "system",		"choices": "PAL, NTSC"	},
		{ "type": "spacer" },
		{ "id": "brightness",	"type": "slider",		"icon": "brightness"		},
		{ "id": "contrast",		"type": "slider",		"icon": "contrast"			},
		{ "id": "saturation",	"type": "slider",		"icon": "saturation"		},
		{ "type": "spacer" },
		{ "id": "overscan",		"type": "toggleSlider",	"label": "overscan"			},
		{ "type": "spacer" },
		{ "id": "emulation",	"type": "toggle",		"label": "CRT emulation"	},
		{ "type": "spacer" },
		{ "id": "curve",		"type": "toggleSlider",	"label": "curve"			},
		{ "id": "bleed",		"type": "toggleSlider",	"label": "bleed"			},
		{ "id": "vignette",		"type": "toggleSlider",	"label": "vignette"			},
		{ "id": "scanlines",	"type": "toggleSlider",	"label": "scanlines"		},
		{ "id": "gridlines",	"type": "toggleSlider",	"label": "gridlines"		},
		{ "id": "glow",			"type": "toggleSlider",	"label": "glow"				},
		{ "id": "flicker",		"type": "toggleSlider",	"label": "flicker"			},
		{ "id": "noise",		"type": "toggleSlider",	"label": "noise"			},
    {}
	] }
])"_padded;
  auto parser = ondemand::parser{};
  auto doc = parser.iterate(padded_input_json);
  auto root_array = doc.get_array();
  // the root should be an object, not an array, but that's the JSON we are
  // given.
  for (ondemand::object node : root_array) {
    // We know that we are going to have just one element in the object.
    for (auto field : node) {
      std::cout << "\n\ntop level:" << field.key() << std::endl;
      // You can get a proper std::string_view for the key with:
      // std::string_view key = field.unescaped_key();
      // and second for-range loop to get child-elements here
      for (ondemand::object inner_object : field.value()) {
        auto i = inner_object.begin();
        if (i == inner_object.end()) {
          std::cout << "empty object" << std::endl;
          continue;
        } else {
          for (; i != inner_object.end(); ++i) {
            auto inner_field = *i;
            std::cout << '"' << inner_field.key()
                      << "\" : " << inner_field.value() << ", ";
            // You can get proper std::string_view for the key and value with:
            // std::string_view inner_key = field.unescaped_key();
            // std::string_view value_str = field.value();
          }
        }
        std::cout << std::endl;
      }
      // You can break here if you only want just the first element.
      // break;
    }
  }
  TEST_SUCCEED();
}


bool examplecrt_realloc() {
  TEST_START();
  std::string unpadded_input_json = R"([
	{ "monitor": [
		{ "id": "monitor",		"type": "toggle",		"label": "monitor"			},
		{ "id": "profile",		"type": "selector",		"label": "collection"		},
		{ "id": "overlay",		"type": "selector",		"label": "overlay"			},
		{ "id": "zoom",			"type": "toggleSlider",	"label": "zoom"				}
	] },

	{ "crt": [
		{ "id": "system",		"type": "multi",		"label": "system",		"choices": "PAL, NTSC"	},
		{ "type": "spacer" },
		{ "id": "brightness",	"type": "slider",		"icon": "brightness"		},
		{ "id": "contrast",		"type": "slider",		"icon": "contrast"			},
		{ "id": "saturation",	"type": "slider",		"icon": "saturation"		},
		{ "type": "spacer" },
		{ "id": "overscan",		"type": "toggleSlider",	"label": "overscan"			},
		{ "type": "spacer" },
		{ "id": "emulation",	"type": "toggle",		"label": "CRT emulation"	},
		{ "type": "spacer" },
		{ "id": "curve",		"type": "toggleSlider",	"label": "curve"			},
		{ "id": "bleed",		"type": "toggleSlider",	"label": "bleed"			},
		{ "id": "vignette",		"type": "toggleSlider",	"label": "vignette"			},
		{ "id": "scanlines",	"type": "toggleSlider",	"label": "scanlines"		},
		{ "id": "gridlines",	"type": "toggleSlider",	"label": "gridlines"		},
		{ "id": "glow",			"type": "toggleSlider",	"label": "glow"				},
		{ "id": "flicker",		"type": "toggleSlider",	"label": "flicker"			},
		{ "id": "noise",		"type": "toggleSlider",	"label": "noise"			},
    {}
	] }
])";
  unpadded_input_json.shrink_to_fit();
  auto parser = ondemand::parser{};
  auto doc = parser.iterate(unpadded_input_json);
  auto root_array = doc.get_array();
  // the root should be an object, not an array, but that's the JSON we are
  // given.
  for (ondemand::object node : root_array) {
    // We know that we are going to have just one element in the object.
    for (auto field : node) {
      std::cout << "\n\ntop level:" << field.key() << std::endl;
      // You can get a proper std::string_view for the key with:
      // std::string_view key = field.unescaped_key();
      // and second for-range loop to get child-elements here
      for (ondemand::object inner_object : field.value()) {
        auto i = inner_object.begin();
        if (i == inner_object.end()) {
          std::cout << "empty object" << std::endl;
          continue;
        } else {
          for (; i != inner_object.end(); ++i) {
            auto inner_field = *i;
            std::cout << '"' << inner_field.key()
                      << "\" : " << inner_field.value() << ", ";
            // You can get proper std::string_view for the key and value with:
            // std::string_view inner_key = field.unescaped_key();
            // std::string_view value_str = field.value();
          }
        }
        std::cout << std::endl;
      }
      // You can break here if you only want just the first element.
      // break;
    }
  }
  TEST_SUCCEED();
}

bool number_tests() {
  TEST_START();
  ondemand::parser parser;
  padded_string docdata = R"([1.0, 3, 1, 3.1415,-13231232,9999999999999999999])"_padded;
  ondemand::document doc = parser.iterate(docdata);
  ondemand::array arr = doc.get_array();
  for(ondemand::value val : arr) {
    std::cout << val << " ";
    std::cout << "negative: " << val.is_negative() << " ";
    std::cout << "is_integer: " << val.is_integer() << " ";
    ondemand::number num = val.get_number();
    ondemand::number_type t = num.get_number_type();
    // direct computation without materializing the number:
    ondemand::number_type dt = val.get_number_type();
    if(t != dt) { throw std::runtime_error("bug"); }
    switch(t) {
      case ondemand::number_type::signed_integer:
        std::cout  << "integer: " << int64_t(num) << " ";
        std::cout  << "integer: " << num.get_int64() << std::endl;
        break;
      case ondemand::number_type::unsigned_integer:
        std::cout  << "large 64-bit integer: " << uint64_t(num) << " ";
        std::cout << "large 64-bit integer: " << num.get_uint64() << std::endl;
        break;
      case ondemand::number_type::floating_point_number:
        std::cout  << "float: " << double(num) << " ";
        std::cout << "float: " << num.get_double() << std::endl;
        break;
    }
  }
  TEST_SUCCEED();
}

bool recursive_print_json(ondemand::value element) {
  TEST_START();
  bool add_comma;
  switch (element.type()) {
  case ondemand::json_type::array:
    cout << "[";
    add_comma = false;
    for (auto child : element.get_array()) {
      if (add_comma) {
        cout << ",";
      }
      // We need the call to value() to get
      // an ondemand::value type.
      recursive_print_json(child.value());
      add_comma = true;
    }
    cout << "]";
    break;
  case ondemand::json_type::object:
    cout << "{";
    add_comma = false;
    for (auto field : element.get_object()) {
      if (add_comma) {
        cout << ",";
      }
      // key() returns the key as it appears in the raw
      // JSON document, if we want the unescaped key,
      // we should do field.unescaped_key().
      cout << "\"" << field.key() << "\": ";
      recursive_print_json(field.value());
      add_comma = true;
    }
    cout << "}\n";
    break;
  case ondemand::json_type::number:
    // assume it fits in a double
    cout << element.get_double();
    break;
  case ondemand::json_type::string:
    // get_string() would return escaped string, but
    // we are happy with unescaped string.
    cout << "\"" << element.get_raw_json_string() << "\"";
    break;
  case ondemand::json_type::boolean:
    cout << element.get_bool();
    break;
  case ondemand::json_type::null:
    // we check that the value is indeed null
    // otherwise: an error is thrown.
    if(element.is_null()) {
      cout << "null";
    }
    break;
  }
  TEST_SUCCEED();
}

bool basics_treewalk() {
  TEST_START();
  padded_string json[3] = {R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded, R"( {"key":"value"} )"_padded, "[12,3]"_padded};
  ondemand::parser parser;
  for(size_t i = 0 ; i < 3; i++) {
    ondemand::document doc = parser.iterate(json[i]);
    ondemand::value val = doc;
    recursive_print_json(val);
    std::cout << std::endl;
  }
  TEST_SUCCEED();
}

bool print_depth_space(ondemand::value element) {
  TEST_START();
  for(auto i = 0; i < element.current_depth(); i++) {
    cout << " ";
  }
  TEST_SUCCEED();
}

bool recursive_print_json_breakline(ondemand::value element) {
  TEST_START();
  bool add_comma;
  switch (element.type()) {
  case ondemand::json_type::array:
    cout << endl;
    print_depth_space(element);
    cout << "[";
    add_comma = false;
    for (auto child : element.get_array()) {
      if (add_comma) {
        print_depth_space(element);
        cout << ",";
      }
      // We need the call to value() to get
      // an ondemand::value type.
      recursive_print_json_breakline(child.value());
      add_comma = true;
    }
    cout << "]";
    break;
  case ondemand::json_type::object:
    cout << endl;
    print_depth_space(element);
    cout << "{";
    add_comma = false;
    for (auto field : element.get_object()) {
      if (add_comma) {
        print_depth_space(element);
        cout << ",";
      }
      // key() returns the key as it appears in the raw
      // JSON document, if we want the unescaped key,
      // we should do field.unescaped_key().
      cout << "\"" << field.key() << "\": ";
      recursive_print_json_breakline(field.value());
      add_comma = true;
    }
    cout << "}\n";
    break;
  case ondemand::json_type::number:
    // assume it fits in a double
    cout << element.get_double();
    break;
  case ondemand::json_type::string:
    // get_string() would return escaped string, but
    // we are happy with unescaped string.
    cout << "\"" << element.get_raw_json_string() << "\"";
    break;
  case ondemand::json_type::boolean:
    cout << element.get_bool();
    break;
  case ondemand::json_type::null:
    // We check that the value is indeed null
    // otherwise: an error is thrown.
    if(element.is_null()) {
      cout << "null";
    }
    break;
  }
  TEST_SUCCEED();
}

bool basics_treewalk_breakline() {
  TEST_START();
  padded_string json[3] = {R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded, R"( {"key":"value"} )"_padded, "[12,3]"_padded};
  ondemand::parser parser;
  for(size_t i = 0 ; i < 3; i++) {
    ondemand::document doc = parser.iterate(json[i]);
    ondemand::value val = doc;
    recursive_print_json_breakline(val);
    std::cout << std::endl;
  }
  TEST_SUCCEED();
}

bool basics_1() {
  TEST_START();

  ondemand::parser parser;
  auto json = padded_string::load("twitter.json");
  ondemand::document doc = parser.iterate(json); // load and parse a file

  simdjson_unused auto unused_doc = doc.get_object();

  TEST_SUCCEED();
}

bool basics_2() {
  TEST_START();

  ondemand::parser parser;
  auto json = "[1,2,3]"_padded; // The _padded suffix creates a simdjson::padded_string instance
  ondemand::document doc = parser.iterate(json); // parse a string

  simdjson_unused auto unused_doc = doc.get_array();

  TEST_SUCCEED();
}

bool basics_3() {
  TEST_START();

  ondemand::parser parser;
  char json[3+SIMDJSON_PADDING];
  strcpy(json, "[1]");
  ondemand::document doc = parser.iterate(json, strlen(json), sizeof(json));

  simdjson_unused auto unused_doc = doc.get_array();

  TEST_SUCCEED();
}


bool json_array_with_array_count() {
  TEST_START();
  ondemand::parser parser;
  auto cars_json = R"( [ 40.1, 39.9, 37.7, 40.4 ] )"_padded;
  auto doc = parser.iterate(cars_json);
  auto arr = doc.get_array();
  size_t count = arr.count_elements();
  ASSERT_EQUAL(4, count);
  std::cout << count << std::endl;
  // We deliberately do it twice:
  count = arr.count_elements();
  ASSERT_EQUAL(4, count);
  std::cout << count << std::endl;
  // Next, we check whether we can iterate normally:
  std::vector<double> values(count);
  size_t index = 0;
  for(double x : arr) { values[index++] = x; }
  ASSERT_EQUAL(index, count);
  TEST_SUCCEED();
}

bool json_value_with_array_count() {
  TEST_START();
  ondemand::parser parser;
  auto cars_json = R"( {"array":[ 40.1, 39.9, 37.7, 40.4 ]} )"_padded;
  auto doc = parser.iterate(cars_json);
  auto val = doc["array"];
  size_t count = val.count_elements();
  ASSERT_EQUAL(4, count);
  std::cout << count << std::endl;
  // We deliberately do it twice:
  count = val.count_elements();
  ASSERT_EQUAL(4, count);
  std::cout << count << std::endl;
  std::vector<double> values(count);
  // Next, we check whether we can iterate normally:
  size_t index = 0;
  for(double x : val) { values[index++] = x; }
  ASSERT_EQUAL(index, count);
  TEST_SUCCEED();
}


bool json_array_count() {
  TEST_START();
  ondemand::parser parser;
  auto cars_json = R"( [ 40.1, 39.9, 37.7, 40.4 ] )"_padded;
  auto doc = parser.iterate(cars_json);
  size_t count = doc.count_elements();
  ASSERT_EQUAL(4, count);
  std::cout << count << std::endl;
  // We deliberately do it twice:
  count = doc.count_elements();
  ASSERT_EQUAL(4, count);
  std::cout << count << std::endl;
  std::vector<double> values(count);
  size_t index = 0;
  for(double x : doc) { values[index++] = x; }
  ASSERT_EQUAL(index, count);
  TEST_SUCCEED();
}

bool json_array_count_complex() {
  TEST_START();
  ondemand::parser parser;
  auto cars_json = R"( { "test":[ { "val1":1, "val2":2 }, { "val1":1, "val2":2 }, { "val1":1, "val2":2 } ] }   )"_padded;
  auto doc = parser.iterate(cars_json);
  auto test_array = doc.find_field("test").get_array();
  size_t count = test_array.count_elements();
  std::cout << "Number of elements: " <<  count << std::endl;
  size_t c = 0;
  for(ondemand::object elem : test_array) {
    std::cout << simdjson::to_json_string(elem);
    c++;
  }
  std::cout << std::endl;
  ASSERT_EQUAL(c, count);
  TEST_SUCCEED();

}

bool json_object_count() {
  TEST_START();
  auto json = R"( { "test":{ "val1":1, "val2":2 } }   )"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  size_t count;
  ASSERT_SUCCESS(doc.count_fields().get(count));
  ASSERT_EQUAL(count,1);
  ondemand::object object;
  size_t new_count;
  ASSERT_SUCCESS(doc.find_field("test").get_object().get(object));
  ASSERT_SUCCESS(object.count_fields().get(new_count));
  ASSERT_EQUAL(new_count, 2);
  TEST_SUCCEED();
}

bool using_the_parsed_json_1() {
  TEST_START();

  try {

    ondemand::parser parser;
    auto json = R"(  { "x": 1, "y": 2 }  )"_padded;
    auto doc = parser.iterate(json);
    double y = doc.find_field("y"); // The cursor is now after the 2 (at })
    double x = doc.find_field("x"); // This fails, because there are no more fields after "y"

    cout << x << ", " << y << endl;
  } catch (...) {
    TEST_SUCCEED();
  }
  TEST_FAIL("expected an exception");
}

bool using_the_parsed_json_2() {
  TEST_START();

  ondemand::parser parser;
  auto json = R"(  { "x": 1, "y": 2 }  )"_padded;
  auto doc = parser.iterate(json);
  double y = doc["y"]; // The cursor is now after the 2 (at })
  double x = doc["x"]; // Success: [] loops back around to find "x"

  cout << x << ", " << y << endl;

  TEST_SUCCEED();
}

  bool big_integer() {
    TEST_START();
    simdjson::ondemand::parser parser;
    simdjson::padded_string docdata =  R"({"value":12321323213213213213213213213211223})"_padded;
    simdjson::ondemand::document doc = parser.iterate(docdata);
    simdjson::ondemand::object obj = doc.get_object();
    string_view token = obj["value"].raw_json_token();
    std::cout << token << std::endl;
    // token == "12321323213213213213213213213211223"
    TEST_SUCCEED();
  }

  bool big_integer_in_string() {
    TEST_START();
    simdjson::ondemand::parser parser;
    simdjson::padded_string docdata =  R"({"value":"12321323213213213213213213213211223"})"_padded;
    simdjson::ondemand::document doc = parser.iterate(docdata);
    simdjson::ondemand::object obj = doc.get_object();
    string_view token = obj["value"].raw_json_token();
    std::cout << token << std::endl;
    // token == "\"12321323213213213213213213213211223\""
    TEST_SUCCEED();
  }
bool using_the_parsed_json_3() {
  TEST_START();

  ondemand::parser parser;
  auto cars_json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;

  // Iterating through an array of objects
  for (ondemand::object car : parser.iterate(cars_json)) {
    // Accessing a field by name
    cout << "Make/Model: " << std::string_view(car["make"]) << "/" << std::string_view(car["model"]) << endl;

    // Casting a JSON element to an integer
    uint64_t year = car["year"];
    cout << "- This car is " << 2020 - year << " years old." << endl;

    // Iterating through an array of floats
    double total_tire_pressure = 0;
    for (double tire_pressure : car["tire_pressure"]) {
      total_tire_pressure += tire_pressure;
    }
    cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;
  }

  TEST_SUCCEED();
}

bool using_the_parsed_json_3b() {
  TEST_START();
  ondemand::parser parser;
  auto cars_json = R"( {
    "identifier1":{ "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    "identifier2":{ "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    "identifier3":{ "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  } )"_padded;

  // Iterating through an array of objects
  ondemand::document doc = parser.iterate(cars_json);
  for (ondemand::field key_car : doc.get_object()) {
    // If I need a string_view and/or, I can use key_car.unescaped_key() instead, but
    // key_car.key() will be more performant otherwise.
    cout << "identifier : " << key_car.key() << std::endl;
    // I can now access the subobject:
    ondemand::object car = key_car.value();
    // Accessing a field by name
    cout << "Make/Model: " << std::string_view(car["make"]) << "/" << std::string_view(car["model"]) << endl;

    // Casting a JSON element to an integer
    uint64_t year = car["year"];
    cout << "- This car is " << 2020 - year << "years old." << endl;

    // Iterating through an array of floats
    double total_tire_pressure = 0;
    for (double tire_pressure : car["tire_pressure"]) {
      total_tire_pressure += tire_pressure;
    }
    cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;
  }

  TEST_SUCCEED();
}

bool using_the_parsed_json_rewind() {
  TEST_START();

  ondemand::parser parser;
  auto cars_json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;

  auto doc = parser.iterate(cars_json);
  size_t count = 0;
  for (simdjson_unused ondemand::object car : doc) {
    if(car["make"] == "Toyota") { count++; }
  }
  std::cout << "We have " << count << " Toyota cars.\n";
  doc.rewind();
  for (ondemand::object car : doc) {
    cout << "Make/Model: " << std::string_view(car["make"]) << "/" << std::string_view(car["model"]) << endl;
  }
  TEST_SUCCEED();
}


bool using_the_parsed_json_rewind_array() {
  TEST_START();

  ondemand::parser parser;
  auto cars_json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;

  auto doc = parser.iterate(cars_json);
  ondemand::array arr = doc.get_array();
  size_t count = 0;
  for (simdjson_unused ondemand::object car : arr) {
    if(car["make"] == "Toyota") { count++; }
  }
  std::cout << "We have " << count << " Toyota cars.\n";
  arr.reset();
  for (ondemand::object car : arr) {
    cout << "Make/Model: " << std::string_view(car["make"]) << "/" << std::string_view(car["model"]) << endl;
  }
  TEST_SUCCEED();
}


bool using_the_parsed_json_4() {
  TEST_START();

  ondemand::parser parser;
  auto points_json = R"( [
      {  "12345" : {"x":12.34, "y":56.78, "z": 9998877}   },
      {  "12545" : {"x":11.44, "y":12.78, "z": 11111111}  }
    ] )"_padded;

  // Parse and iterate through an array of objects
  for (ondemand::object points : parser.iterate(points_json)) {
    // Iterating through an object, you iterate through key-value pairs (a 'field').
    for (auto point : points) {
      // Get the key corresponding the the field 'point'.
      cout << "id: " << std::string_view(point.unescaped_key()) << ": (";
      // Get the value corresponding the the field 'point'.
      ondemand::object xyz = point.value();
      cout << xyz["x"].get_double() << ", ";
      cout << xyz["y"].get_double() << ", ";
      cout << xyz["z"].get_int64() << endl;
    }
  }

  TEST_SUCCEED();
}

bool using_the_parsed_json_5() {
  TEST_START();

  auto abstract_json = R"(
    { "str" : { "123" : {"abc" : 3.14 } } }
  )"_padded;
  ondemand::parser parser;
  auto doc = parser.iterate(abstract_json);
  cout << doc["str"]["123"]["abc"].get_double() << endl; // Prints 3.14

  TEST_SUCCEED();
}

#endif // SIMDJSON_EXCEPTIONS



bool using_the_parsed_json_no_exceptions() {
  TEST_START();

  ondemand::parser parser;
  auto cars_json = R"( [
    { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
    { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
    { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
  ] )"_padded;
  ondemand::document doc;

  // Iterating through an array of objects
  auto error = parser.iterate(cars_json).get(doc);
  if(error) { std::cerr << error << std::endl; return false; }
  ondemand::array cars;
  error = doc.get_array().get(cars);

  for (auto car_value : cars) {
    ondemand::object car;
    error = car_value.get_object().get(car);
    if(error) { std::cerr << error << std::endl; return false; }

    // Accessing a field by name
    std::string_view make;
    std::string_view model;
    error = car["make"].get(make);
    if(error) { std::cerr << error << std::endl; return false; }
    error = car["model"].get(model);
    if(error) { std::cerr << error << std::endl; return false; }

    cout << "Make/Model: " << make << "/" << model << endl;

    // Casting a JSON element to an integer
    uint64_t year{};
    error = car["year"].get(year);
    if(error) { std::cerr << error << std::endl; return false; }
    cout << "- This car is " << 2020 - year << " years old." << endl;

    // Iterating through an array of floats
    double total_tire_pressure = 0;
    ondemand::array pressures;
    error = car["tire_pressure"].get_array().get(pressures);
    if(error) { std::cerr << error << std::endl; return false; }
    for (auto tire_pressure_value : pressures) {
      double tire_pressure;
      error = tire_pressure_value.get_double().get(tire_pressure);
      if(error) { std::cerr << error << std::endl; return false; }
      total_tire_pressure += tire_pressure;
    }
    cout << "- Average tire pressure: " << (total_tire_pressure / 4) << endl;
  }
  TEST_SUCCEED();
}
int using_the_parsed_json_6_process() {
  auto abstract_json = R"(
    { "str" : { "123" : {"abc" : 3.14 } } }
  )"_padded;
  ondemand::parser parser;

  double value;
  auto doc = parser.iterate(abstract_json);
  auto error = doc["str"]["123"]["abc"].get(value);
  if (error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  cout << value << endl; // Prints 3.14

  return EXIT_SUCCESS;
}

bool exception_free_object_loop() {
  auto json = R"({"k\u0065y": 1})"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  auto error = parser.iterate(json).get(doc);
  if(error) { return false; }
  ondemand::object object;
  error = doc.get_object().get(object);
  if(error) { return false; }
  for(auto field : object) {
    std::string_view keyv;
    error = field.unescaped_key().get(keyv);
    if(error) { return false; }
    if(keyv == "key") {
      uint64_t intvalue;
      error = field.value().get(intvalue);
      if(error) { return false; }
      std::cout << intvalue;
    }
  }
  return true;
}


bool exception_free_object_loop_no_escape() {
  auto json = R"({"k\u0065y": 1})"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  auto error = parser.iterate(json).get(doc);
  if(error) { return false; }
  ondemand::object object;
  error = doc.get_object().get(object);
  if(error) { return false; }
  for(auto field : object) {
    ondemand::raw_json_string keyv;
    error = field.key().get(keyv);
    /**
     * If you need to escape the keys, you can do
     * std::string_view keyv;
     * error = field.unescaped_key().get(keyv);
     */
    if(error) { return false; }
    if(keyv == "key") {
      uint64_t intvalue;
      error = field.value().get(intvalue);
      if(error) { return false; }
      std::cout << intvalue;
    }
  }
  return true;
}

bool using_the_parsed_json_6() {
  TEST_START();
  ASSERT_EQUAL(using_the_parsed_json_6_process(), EXIT_SUCCESS);
  TEST_SUCCEED();
}

const padded_string cars_json = R"( [
  { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
  { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
  { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
] )"_padded;

bool json_pointer_simple() {
    TEST_START();
    ondemand::parser parser;
    ondemand::document cars;
    double x;
    ASSERT_SUCCESS(parser.iterate(cars_json).get(cars));
    ASSERT_SUCCESS(cars.at_pointer("/0/tire_pressure/1").get(x));
    ASSERT_EQUAL(x,39.9);
    TEST_SUCCEED();
}

bool json_pointer_multiple() {
	TEST_START();
	ondemand::parser parser;
	ondemand::document cars;
	size_t size;
	ASSERT_SUCCESS(parser.iterate(cars_json).get(cars));
	ASSERT_SUCCESS(cars.count_elements().get(size));
	double expected[] = {39.9, 31, 30};
	for (size_t i = 0; i < size; i++) {
		std::string json_pointer = std::string("/") + std::to_string(i) + std::string("/tire_pressure/1");
		double x;
		ASSERT_SUCCESS(cars.at_pointer(json_pointer).get(x));
		ASSERT_EQUAL(x,expected[i]);
	}
	TEST_SUCCEED();
}

bool json_pointer_rewind() {
  TEST_START();
  auto json = R"( {
  "k0": 27,
  "k1": [13,26],
  "k2": true
  } )"_padded;

  ondemand::parser parser;
  ondemand::document doc;
  uint64_t i;
  bool b;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  ASSERT_SUCCESS(doc.at_pointer("/k1/1").get(i));
  ASSERT_EQUAL(i,26);
  ASSERT_SUCCESS(doc.at_pointer("/k2").get(b));
  ASSERT_EQUAL(b,true);
  doc.rewind();	// Need to manually rewind to be able to use find_field properly from start of document
  ASSERT_SUCCESS(doc.find_field("k0").get(i));
  ASSERT_EQUAL(i,27);
  TEST_SUCCEED();
}

bool iterate_many_example() {
  TEST_START();
  auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} [1,2,3]  )"_padded;
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document_stream stream;
  ASSERT_SUCCESS(parser.iterate_many(json).get(stream));
  auto i = stream.begin();
  size_t count{0};
  size_t expected_indexes[3] = {0,9,29};
  std::string_view expected_doc[3] = {"[1,2,3]", R"({"1":1,"2":3,"4":4})", "[1,2,3]"};
  for(; i != stream.end(); ++i) {
      auto doc = *i;
      ASSERT_SUCCESS(doc.type());
      ASSERT_SUCCESS(i.error());
      ASSERT_EQUAL(i.current_index(),expected_indexes[count]);
      ASSERT_EQUAL(i.source(),expected_doc[count]);
      count++;
  }
  TEST_SUCCEED();
}

std::string my_string(ondemand::document& doc) {
  std::stringstream ss;
  ss << doc;
  return ss.str();
}

bool iterate_many_truncated_example() {
  TEST_START();
  auto json = R"([1,2,3]  {"1":1,"2":3,"4":4} {"key":"intentionally unclosed string  )"_padded;
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document_stream stream;
  ASSERT_SUCCESS( parser.iterate_many(json,json.size()).get(stream) );
  std::string_view expected[2] = {"[1,2,3]", R"({"1":1,"2":3,"4":4})"};
  size_t count{0};
  for(auto i = stream.begin(); i != stream.end(); ++i) {
      ASSERT_EQUAL(i.source(),expected[count++]);
  }
  size_t truncated = stream.truncated_bytes();
  ASSERT_EQUAL(truncated,39);
  TEST_SUCCEED();
}

bool ndjson_basics_example() {
  TEST_START();
  auto json = R"({ "foo": 1 } { "foo": 2 } { "foo": 3 } )"_padded;
  ondemand::parser parser;
  ondemand::document_stream docs;
  ASSERT_SUCCESS( parser.iterate_many(json).get(docs) );
  size_t count{0};
  int64_t expected[3] = {1,2,3};
  for (auto doc : docs) {
    int64_t actual{};
    ASSERT_SUCCESS( doc["foo"].get(actual) );
    ASSERT_EQUAL( actual, expected[count++] );
  }
  TEST_SUCCEED();
}
bool stream_capacity_example() {
  auto json = R"([1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5] [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100])"_padded;
  ondemand::parser parser;
  ondemand::document_stream stream;
  size_t counter{0};
  auto error = parser.iterate_many(json, 50).get(stream);
  if( error ) { /* handle the error */ }
  for (auto doc: stream) {
    if(counter < 6) {
      int64_t val{};
      error = doc.at_pointer("/4").get(val);
      if( error ) { /* handle the error */ }
      std::cout << "5 = " << val << std::endl;
    } else {
      ondemand::value val;
      error = doc.at_pointer("/4").get(val);
      // error == simdjson::CAPACITY
      if(error) {
        std::cerr << error << std::endl;
        // We left 293 bytes unprocessed at the tail end of the input.
        std::cout << " unprocessed bytes at the end: " << stream.truncated_bytes() << std::endl;
        break;
      }
    }
    counter++;
  }
  return true;
}



bool simple_error_example() {
    ondemand::parser parser;
    auto json = R"({"bad number":3.14.1 })"_padded;
    ondemand::document doc;
    if( parser.iterate(json).get(doc) != SUCCESS ) { return false; }
    double x;
    auto error = doc["bad number"].get_double().get(x);
    // returns "simdjson::NUMBER_ERROR"
    if(error != SUCCESS) {
      std::cout << error << std::endl;
      return false;
    }
    std::cout << "Got " << x << std::endl;
    return true;
}


#if SIMDJSON_EXCEPTIONS
  bool raw_string() {
    TEST_START();
    auto json = R"( {"name": "Jack The Ripper \u0033"} )"_padded;
    // We create a buffer large enough to store all strings we need:
    std::unique_ptr<uint8_t[]> buffer(new uint8_t[json.size() + simdjson::SIMDJSON_PADDING]);
    uint8_t * ptr = buffer.get();
    ondemand::parser parser;
    ondemand::document doc = parser.iterate(json);
    // We store our strings as 'string_view' instances in a vector:
    std::vector<std::string_view> mystrings;
    for (auto key_value : doc.get_object()) {
      std::string_view keysv = parser.unescape(key_value.key(), ptr);// writes 'name'
      mystrings.push_back(keysv);
      std::string_view valuesv = parser.unescape(key_value.value().get_raw_json_string(), ptr);
      // writes 'Jack The Ripper 3', escaping the \u0033
      mystrings.push_back(valuesv);
    }
    ASSERT_EQUAL(mystrings[0],"name");
    ASSERT_EQUAL(mystrings[1],"Jack The Ripper 3");
    TEST_SUCCEED();
  }


  bool simple_error_example_except() {
    TEST_START();
    ondemand::parser parser;
    auto json = R"({"bad number":3.14.1 })"_padded;
    try {
      ondemand::document doc = parser.iterate(json);
      double x = doc["bad number"].get_double();
      std::cout << "Got " << x << std::endl;
      TEST_SUCCEED();
    } catch(simdjson_error& e) {
      // e.error() == NUMBER_ERROR
      std::cout << e.error() << std::endl;
      TEST_FAIL("I did not expect an exception");
    }
  }

  int64_t current_location_tape_error_with_except() {
    TEST_START();
    auto broken_json = R"( {"double": 13.06, false, "integer": -343} )"_padded;
    ondemand::parser parser;
    ondemand::document doc;
    try {
      doc = parser.iterate(broken_json);
      return int64_t(doc["integer"]);
    } catch(simdjson_error& err) {
      std::cout << err.error() << std::endl;
      std::cout << doc.current_location() << std::endl;
      TEST_SUCCEED();
    }
    TEST_FAIL("I expected an exception!");
  }

#endif

int load_example() {
  TEST_START();
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document tweets;
  padded_string json;
  auto error = padded_string::load("twitter.json").get(json);
  if(error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  error = parser.iterate(json).get(tweets);
  if(error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  uint64_t identifier;
  error = tweets["statuses"].at(0)["id"].get(identifier);
  if(error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  std::cout << identifier << std::endl;
  return EXIT_SUCCESS;
}


int example_1() {
  TEST_START();
  simdjson::ondemand::parser parser;
  //auto error = padded_string::load("twitter.json").get(json);
  // if(error) { std::cerr << error << std::endl; return EXIT_FAILURE; }
  padded_string json = R"( {
  "statuses": [
    {
      "id": 505874924095815700
    },
    {
      "id": 505874922023837700
    }
  ],
  "search_metadata": {
    "count": 100
  }
} )"_padded;

  simdjson::ondemand::document tweets;
  auto error = parser.iterate(json).get(tweets);
  if( error ) { return EXIT_FAILURE; }
  simdjson::ondemand::value res;
  error = tweets["search_metadata"]["count"].get(res);
  if (error != SUCCESS) {
    std::cerr << "could not access keys" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << res << " results." << std::endl;
  return true;
}
#if SIMDJSON_EXCEPTIONS
int load_example_except() {
  TEST_START();
  simdjson::ondemand::parser parser;
  padded_string json = padded_string::load("twitter.json");
  simdjson::ondemand::document tweets = parser.iterate(json);
  uint64_t identifier = tweets["statuses"].at(0)["id"];
  std::cout << identifier << std::endl;
  return EXIT_SUCCESS;
}
int load_example_except_morecomplete(void) {
  TEST_START();
  simdjson::ondemand::parser parser;
  simdjson::padded_string json_string;
  simdjson::ondemand::document doc;
  try {
    json_string = padded_string::load("twitter.json");
    doc = parser.iterate(json_string);
    uint64_t identifier = doc["statuses"].at(0)["id"];
    std::cout << identifier << std::endl;
  } catch (simdjson::simdjson_error &error) {
    std::cerr << "JSON error: " << error.what() << " near "
              << doc.current_location() << " in " << json_string << std::endl;
  }
  return EXIT_SUCCESS;
}

bool allow_comma_separated_example() {
  TEST_START();
  auto json = R"( 1, 2, 3, 4, "a", "b", "c", {"hello": "world"} , [1, 2, 3])"_padded;
  ondemand::parser parser;
  ondemand::document_stream doc_stream;
  // We pass '32' as the batch size, but it is a bogus parameter because, since
  // we pass 'true' to the allow_comma parameter, the batch size will be set to at least
  // the document size.
  auto error = parser.iterate_many(json, 32, true).get(doc_stream);
  if(error) { std::cerr << error << std::endl; return false; }
  for (auto doc : doc_stream) {
    std::cout << doc.type() << std::endl;
  }
  TEST_SUCCEED();
}
#endif
bool test_load_example() {
  TEST_START();
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document tweets;
  padded_string json = R"( {"statuses":[{"id":1234}]} )"_padded;
  auto error = parser.iterate(json).get(tweets);
  if(error) { std::cerr << error << std::endl; return false; }
  uint64_t identifier;
  error = tweets["statuses"].at(0)["id"].get(identifier);
  if(error) { std::cerr << error << std::endl; return false; }
  std::cout << identifier << std::endl;
  return identifier == 1234;
}

bool current_location_tape_error() {
  TEST_START();
  auto broken_json = R"( {"double": 13.06, false, "integer": -343} )"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(broken_json).get(doc));
  const char * ptr;
  int64_t i;
  ASSERT_ERROR(doc["integer"].get_int64().get(i), TAPE_ERROR);
  ASSERT_SUCCESS(doc.current_location().get(ptr));
  std::string expected = "false, \"integer\": -343} ";
  ASSERT_EQUAL(std::string(ptr,expected.size()), expected);
  TEST_SUCCEED();
}

bool current_location_user_error() {
  TEST_START();
  auto json = R"( [1,2,3] )"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  const char * ptr;
  int64_t i;
  ASSERT_ERROR(doc["integer"].get_int64().get(i), INCORRECT_TYPE);
  ASSERT_SUCCESS(doc.current_location().get(ptr));
  std::string expected = "[1,2,3] ";
  ASSERT_EQUAL(std::string(ptr, expected.size()), expected);
  TEST_SUCCEED();
}

bool current_location_out_of_bounds() {
  TEST_START();
  auto json = R"( [1,2,3] )"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  uint64_t expected[3] = {1, 2, 3};
  size_t count{0};
  for (auto val : doc) {
    uint64_t i;
    ASSERT_SUCCESS(val.get_uint64().get(i));
    ASSERT_EQUAL(i, expected[count++]);
  }
  ASSERT_EQUAL(count, 3);
  ASSERT_ERROR(doc.current_location(), OUT_OF_BOUNDS);
  TEST_SUCCEED();
}

bool current_location_no_error() {
  TEST_START();
  auto json = R"( [[1,2,3], -23.4, {"key": "value"}, true] )"_padded;
  ondemand::parser parser;
  ondemand::document doc;
  ASSERT_SUCCESS(parser.iterate(json).get(doc));
  const char * ptr;
  for (auto val : doc) {
    ondemand::object obj;
    auto error = val.get_object().get(obj);
    if (!error) {
      ASSERT_SUCCESS(doc.current_location().get(ptr));
      std::string expected = "\"key\": \"value\"}, true] ";
      ASSERT_EQUAL(std::string(ptr, expected.size()), expected);
    }
  }
  TEST_SUCCEED();
}



struct ZuluBBox {
  double xmin;
  double ymin;
  double width;
  double height;

  void print() {
    std::cout << xmin << ", " << ymin << ", " << width << ", " << height
              << std::endl;
  }
};
#if SIMDJSON_EXCEPTIONS

bool example1956() {

  auto json = R"+( {
  "ZuluROI": {
    "ZuluBBox": {
      "xmin": 0,
      "ymin": 0,
      "width": 1,
      "height": 1
    },
    "SubObjects": [
      {
        "ZuluDetection": {
          "label": "car",
          "class_id": 3,
          "confidence": 0.7587034106254578,
          "ZuluBBox": {
            "xmin": 0.3843536376953125,
            "ymin": 0.4532909393310547,
            "width": 0.09115534275770187,
            "height": 0.04127710685133934
          },
          "SubObjects": []
        }
      },
      {
        "ZuluDetection": {
          "label": "car",
          "class_id": 3,
          "confidence": 0.6718865633010864,
          "ZuluBBox": {
            "xmin": 0.7500002980232239,
            "ymin": 0.5212296843528748,
            "width": 0.07592231780290604,
            "height": 0.038947589695453644
          },
          "SubObjects": []
        }
      },
      {
        "ZuluDetection": {
          "label": "car",
          "class_id": 3,
          "confidence": 0.5806200504302979,
          "ZuluBBox": {
            "xmin": 0.9025363922119141,
            "ymin": 0.5925348401069641,
            "width": 0.05478987470269203,
            "height": 0.046337299048900604
          },
          "SubObjects": []
        }
      }
    ]
  },
  "timestamp (ms)": 1677085594421,
  "buffer_offset": 35673
} )+"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ondemand::object root_object = doc.get_object();
  ondemand::object roi_object = root_object["ZuluROI"];

  ondemand::object box_roi_object = roi_object["ZuluBBox"];
  ZuluBBox box = {
      double(box_roi_object["xmin"]), double(box_roi_object["ymin"]),
      double(box_roi_object["width"]), double(box_roi_object["height"])};
  box.print();

  for (ondemand::object value : roi_object["SubObjects"]) {
    ondemand::object detect = value["ZuluDetection"];
    std::cout << detect["label"].get_string() << std::endl;
    std::cout << detect["class_id"].get_uint64() << std::endl;
    std::cout << detect["confidence"].get_double() << std::endl;

    ondemand::object vbox_roi_object = detect["ZuluBBox"];
    ZuluBBox vbox = {
        double(vbox_roi_object["xmin"]), double(vbox_roi_object["ymin"]),
        double(vbox_roi_object["width"]), double(vbox_roi_object["height"])};
    vbox.print();
  }

  std::cout << root_object["timestamp (ms)"].get_uint64() << std::endl;
  std::cout << root_object["buffer_offset"].get_uint64() << std::endl;
  return true;
}


bool example1958() {
  auto json = R"+( {
    "5f08a730b280e54fd1e75a7046b93fdc": {
        "file": "/DEMOS/0-9/10_Orbyte.sid",
        "len": [
            "1:17"
        ],
        "loud": [
            "-22.8"
        ],
        "name": "10 Orbyte",
        "author": "Michael Becker (Premium)",
        "release": "2014 Tristar & Red Sector Inc.",
        "bits": 20
    },
    "2727236ead44a62f0c6e01f6dd4dc484": {
        "file": "/DEMOS/0-9/12345.sid",
        "len": [
            "0:56"
        ],
        "loud": [
            "-33.3"
        ],
        "name": "12345",
        "author": "Beal",
        "release": "1988 Beal",
        "bits": 20
    },
    "7ea765fce6c0f92570b18adc7bf52f54": {
        "file": "/DEMOS/0-9/128_Byte_Blues_BASIC.sid",
        "len": [
            "0:18"
        ],
        "loud": [
            "-27.1"
        ],
        "name": "128 Byte Blues",
        "author": "Leonard J. Paul (Freaky DNA)",
        "release": "2005 Freaky DNA",
        "bits": 62
    }
} )+"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  ondemand::object root_object = doc.get_object();
  for(auto key_value : root_object) {
    // could get std::string_view with 'unescaped_key()':
    std::cout << "key: " << key_value.key() << std::endl;
    ondemand::object obj = key_value.value();

    std::cout << "file: " << std::string_view(obj["file"]) << std::endl;

    std::cout << "len: ";
    for(std::string_view values : obj["len"]) {
      std::cout << values << std::endl;
    }
    std::cout << std::endl;

    std::cout << "loud: ";
    for(std::string_view values : obj["loud"]) {
      std::cout << values << std::endl;
    }
    std::cout << std::endl;

    std::cout << "name: " << std::string_view(obj["name"]) << std::endl;
    std::cout << "author: " << std::string_view(obj["author"]) << std::endl;
    std::cout << "release: " << std::string_view(obj["release"]) << std::endl;
    std::cout << "bits: " << uint64_t(obj["bits"]) << std::endl;
  }
  return true;
}


bool to_optional() {
  TEST_START();
  auto json = R"({ "foo1": "3.1416" } )"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
#if __cpp_lib_optional >= 201606L
  std::optional<std::string> value;
  ASSERT_SUCCESS(doc["foo1"].get_string(value));
  std::cout << value.value() << std::endl;
#else
  std::string value;
  ASSERT_SUCCESS(doc["foo1"].get_string(value));
  std::cout << value << std::endl;
#endif
  TEST_SUCCEED();
}

bool value_raw_json_array() {
  TEST_START();
  auto json = R"( [1,2,"fds", {"a":1}, [1,344]] )"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  std::string_view expected[] = {"1", "2", "\"fds\"", "{\"a\":1}", "[1,344]"};
  size_t counter = 0;
  for(auto array: doc) {
    std::string_view raw = array.raw_json();
    ASSERT_EQUAL(raw, expected[counter++]);
  }
  TEST_SUCCEED();
}


bool value_raw_json_object() {
  TEST_START();
  auto json = R"( {"key1":1,"key2":2,"key3":"fds", "key4":{"a":1}, "key5":[1,344]} )"_padded;
  ondemand::parser parser;
  ondemand::document doc = parser.iterate(json);
  std::string_view expected[] = {"1", "2", "\"fds\"", "{\"a\":1}", "[1,344]"};
  size_t counter = 0;
  for(auto key_value: doc.get_object()) {
    std::string_view raw = key_value.value().raw_json();
    ASSERT_EQUAL(raw, expected[counter++]);
  }
  TEST_SUCCEED();
}

#endif
bool run() {
  return true
#if SIMDJSON_EXCEPTIONS
    && to_optional()
    && value_raw_json_array() && value_raw_json_object()
    && gen_raw1() && gen_raw2() && gen_raw3()
    && at_end()
    && example1956() && example1958()
    && allow_comma_separated_example()
//    && basics_1() // Fails because twitter.json isn't in current directory. Compile test only.
    &&  basics_treewalk()
    &&  basics_treewalk_breakline()
    && json_value_with_array_count()
    && json_array_with_array_count()
    && json_array_count_complex()
    && json_array_count()
    && json_object_count()
    && using_the_parsed_json_rewind()
    && using_the_parsed_json_rewind_array()
    && basics_2()
    && using_the_parsed_json_1()
    && using_the_parsed_json_2()
    && big_integer()
    && big_integer_in_string()
    && using_the_parsed_json_3()
    && using_the_parsed_json_3b()
    && using_the_parsed_json_4()
    && using_the_parsed_json_5()
#endif
    && using_the_parsed_json_6()
    && json_pointer_simple()
    && json_pointer_multiple()
    && json_pointer_rewind()
    && iterate_many_example()
    && iterate_many_truncated_example()
    && ndjson_basics_example()
    && stream_capacity_example()
    && test_load_example()
    && example_1()
    && using_the_parsed_json_no_exceptions()
    && current_location_tape_error()
    && current_location_user_error()
    && current_location_out_of_bounds()
    && current_location_no_error()
    && to_string_example_no_except()
  #if SIMDJSON_EXCEPTIONS
    && to_string_example()
    && raw_string()
    && number_tests()
    && current_location_tape_error_with_except()
    && examplecrt()
    && examplecrt_realloc()
  #endif
  ;
}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, run);
}
