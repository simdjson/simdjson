#include "simdjson.h"
#include "test_ondemand.h"
#include <string>

using namespace simdjson;

namespace json_pointer_tests {
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

    bool run_success_test(const padded_string & json,std::string_view json_pointer,std::string expected) {
        TEST_START();
        std::cout <<":"<< json_pointer<<std::endl;
        ondemand::parser parser;
        ondemand::document doc;
        ondemand::value val;
        std::string_view actual;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        ASSERT_SUCCESS(doc.at_pointer(json_pointer).get(val));
        ASSERT_SUCCESS(simdjson::to_json_string(val).get(actual));
        ASSERT_EQUAL(actual,expected);
        // We want to see if the value is usable besides to_json_string.
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        ASSERT_SUCCESS(doc.at_pointer(json_pointer).get(val));
        ondemand::json_type type;
        ASSERT_SUCCESS(val.type().get(type));
        switch (type) {
            case ondemand::json_type::array:
               ASSERT_SUCCESS(val.get_array().error());
               break;
            case ondemand::json_type::object:
               ASSERT_SUCCESS(val.get_object().error());
               break;
            case ondemand::json_type::number:
               ASSERT_SUCCESS(val.get_double().error());
               break;
            case ondemand::json_type::string:
               ASSERT_SUCCESS(val.get_string().error());
               break;
            case ondemand::json_type::boolean:
               ASSERT_SUCCESS(val.get_bool().error());
               break;
            case ondemand::json_type::null:
               ASSERT_SUCCESS(val.is_null().error());
               break;
            default:
               TEST_FAIL("unexpected type");
        }
        TEST_SUCCEED();
    }

    bool run_failure_test(const padded_string & json,std::string_view json_pointer,error_code expected) {
        TEST_START();
        std::cout << "json_pointer: " << json_pointer << std::endl;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        ASSERT_EQUAL(doc.at_pointer(json_pointer).error(),expected);
        TEST_SUCCEED();
    }

    bool demo_test() {
        TEST_START();
        auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;

        ondemand::parser parser;
        ondemand::document cars;
        double x;
        ASSERT_SUCCESS(parser.iterate(cars_json).get(cars));
        ASSERT_SUCCESS(cars.at_pointer("/0/tire_pressure/1").get(x));
        ASSERT_EQUAL(x,39.9);
        TEST_SUCCEED();
    }

    bool demo_relative_path() {
        TEST_START();
        auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;

        ondemand::parser parser;
        ondemand::document cars;
        std::vector<double> measured;
        ASSERT_SUCCESS(parser.iterate(cars_json).get(cars));
        for (auto car_element : cars) {
            double x;
            ASSERT_SUCCESS(car_element.at_pointer("/tire_pressure/1").get(x));
            measured.push_back(x);
        }

        std::vector<double> expected = {39.9, 31, 30};
        if (measured != expected) { return false; }
        TEST_SUCCEED();
    }

    bool document_as_scalar() {
        TEST_START();
        auto number_json = R"( 1 )"_padded;
        auto null_json = R"( null )"_padded;
        auto string_json = R"( "a" )"_padded;
        auto true_json = R"( true )"_padded;
        auto false_json = R"( false )"_padded;
        auto object_json = R"( {} )"_padded;
        auto array_json = R"( {} )"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ondemand::value val;
        bool is_scalar;
        std::cout << "  checking number"<< std::endl;
        ASSERT_SUCCESS(parser.iterate(number_json).get(doc));
        ASSERT_SUCCESS(doc.is_scalar().get(is_scalar));
        ASSERT_TRUE(is_scalar);
        ASSERT_ERROR(doc.at_pointer("").get(val), simdjson::SCALAR_DOCUMENT_AS_VALUE);
        std::cout << "  checking null"<< std::endl;
        ASSERT_SUCCESS(parser.iterate(null_json).get(doc));
        ASSERT_SUCCESS(doc.is_scalar().get(is_scalar));
        ASSERT_TRUE(is_scalar);
        ASSERT_ERROR(doc.at_pointer("").get(val), simdjson::SCALAR_DOCUMENT_AS_VALUE);
        std::cout << "  checking string"<< std::endl;
        ASSERT_SUCCESS(parser.iterate(string_json).get(doc));
        ASSERT_SUCCESS(doc.is_scalar().get(is_scalar));
        ASSERT_TRUE(is_scalar);
        ASSERT_ERROR(doc.at_pointer("").get(val), simdjson::SCALAR_DOCUMENT_AS_VALUE);
        std::cout << "  checking false"<< std::endl;
        ASSERT_SUCCESS(parser.iterate(false_json).get(doc));
        ASSERT_SUCCESS(doc.is_scalar().get(is_scalar));
        ASSERT_TRUE(is_scalar);
        ASSERT_ERROR(doc.at_pointer("").get(val), simdjson::SCALAR_DOCUMENT_AS_VALUE);
        std::cout << "  checking true"<< std::endl;
        ASSERT_SUCCESS(parser.iterate(true_json).get(doc));
        ASSERT_SUCCESS(doc.is_scalar().get(is_scalar));
        ASSERT_TRUE(is_scalar);
        ASSERT_ERROR(doc.at_pointer("").get(val), simdjson::SCALAR_DOCUMENT_AS_VALUE);
        std::cout << "  checking object"<< std::endl;
        ASSERT_SUCCESS(parser.iterate(object_json).get(doc));
        ASSERT_SUCCESS(doc.is_scalar().get(is_scalar));
        ASSERT_FALSE(is_scalar);
        ASSERT_SUCCESS(doc.at_pointer("").get(val));
        std::cout << "  checking array"<< std::endl;
        ASSERT_SUCCESS(parser.iterate(array_json).get(doc));
        ASSERT_SUCCESS(doc.is_scalar().get(is_scalar));
        ASSERT_FALSE(is_scalar);
        ASSERT_SUCCESS(doc.at_pointer("").get(val));
        TEST_SUCCEED();
    }


    bool many_json_pointers() {
        TEST_START();
        auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;

        ondemand::parser parser;
        ondemand::document cars;
        std::vector<double> measured;
        ASSERT_SUCCESS(parser.iterate(cars_json).get(cars));
        for (int i = 0; i < 3; i++) {
            double x;
            std::string json_pointer = std::string("/") + std::to_string(i) + std::string("/tire_pressure/1");
            ASSERT_SUCCESS(cars.at_pointer(json_pointer).get(x));
            measured.push_back(x);
        }

        std::vector<double> expected = {39.9, 31, 30};
        if (measured != expected) { return false; }
        TEST_SUCCEED();
    }
    bool run_broken_tests() {
        TEST_START();
        ondemand::parser parser;
        ondemand::document doc;
        ondemand::value v;
        std::string_view val;

        auto invalid_escape_key = R"( {"hello": [0,1,2,3], "te\est": "foo", "bool": true, "num":1234, "success":"yes"} )"_padded;
        auto invalid_escape_value = R"( {"hello": [0,1,2,3], "test": "fo\eo", "bool": true, "num":1234, "success":"yes"} )"_padded;
        auto invalid_escape_value_at_jp = R"( {"hello": [0,1,2,3], "test": "foo", "bool": true, "num":1234, "success":"y\es"} )"_padded;
        auto unclosed_object = R"( {"test": "foo", "bool": true, "num":1234, "success":"yes" )"_padded;
        auto missing_bracket_before = R"( {"hello": [0,1,2,3, "test": "foo", "bool": true, "num":1234, "success":"yes"} )"_padded;
        auto missing_bracket_after = R"( {"test": "foo", "bool": true, "num":1234, "success":"yes", "hello":[0,1,2,3} )"_padded;

        std::string json_pointer = "/success";
        std::cout << "\t- invalid_escape_key" << std::endl;
        ASSERT_SUCCESS(parser.iterate(invalid_escape_key).get(doc));
        ASSERT_SUCCESS(doc.at_pointer(json_pointer).get(val));
        std::cout << "\t- invalid_escape_value" << std::endl;
        ASSERT_SUCCESS(parser.iterate(invalid_escape_value).get(doc));
        ASSERT_SUCCESS(doc.at_pointer(json_pointer).get(val));
        std::cout << "\t- invalid_escape_value_at_jp_nomat" << std::endl;
        ASSERT_SUCCESS(parser.iterate(invalid_escape_value_at_jp).get(doc));
        ASSERT_SUCCESS(doc.at_pointer(json_pointer).get(v));
        std::cout << "\t- invalid_escape_value_at_jp" << std::endl;
        ASSERT_SUCCESS(parser.iterate(invalid_escape_value_at_jp).get(doc));
        ASSERT_ERROR(doc.at_pointer(json_pointer).get(val), simdjson::STRING_ERROR);
        std::cout << "\t- unclosed_object" << std::endl;
        ASSERT_SUCCESS(parser.iterate(unclosed_object).get(doc));
        ASSERT_ERROR(doc.at_pointer(json_pointer).get(val), simdjson::INCOMPLETE_ARRAY_OR_OBJECT);
        std::cout << "\t- missing_bracket_before" << std::endl;
        ASSERT_SUCCESS(parser.iterate(missing_bracket_before).get(doc));
        ASSERT_ERROR(doc.at_pointer(json_pointer).get(val), simdjson::TAPE_ERROR);
        std::cout << "\t- missing_bracket_after" << std::endl;
        ASSERT_SUCCESS(parser.iterate(missing_bracket_after).get(doc));
        ASSERT_SUCCESS(doc.at_pointer(json_pointer).get(val));
        TEST_SUCCEED();
    }


    bool many_json_pointers_object_array() {
        TEST_START();
        auto dogcatpotato = R"( { "dog" : [1,2,3], "cat" : [5, 6, 7], "potato" : [1234]})"_padded;

        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(dogcatpotato).get(doc));
        ondemand::object obj;
        ASSERT_SUCCESS(doc.get_object().get(obj));
        int64_t x;
        ASSERT_SUCCESS(obj.at_pointer("/dog/1").get(x));
        ASSERT_EQUAL(x, 2);
        ASSERT_SUCCESS(obj.at_pointer("/potato/0").get(x));
        ASSERT_EQUAL(x, 1234);
        TEST_SUCCEED();
    }
    bool many_json_pointers_object() {
        TEST_START();
        auto cfoofoo2 = R"( { "c" :{ "foo": { "a": [ 10, 20, 30 ] }}, "d": { "foo2": { "a": [ 10, 20, 30 ] }} , "e": 120 })"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(cfoofoo2).get(doc));
        ondemand::object obj;
        ASSERT_SUCCESS(doc.get_object().get(obj));
        int64_t x;
        ASSERT_SUCCESS(obj.at_pointer("/c/foo/a/1").get(x));
        ASSERT_EQUAL(x, 20);
        ASSERT_SUCCESS(obj.at_pointer("/d/foo2/a/2").get(x));
        ASSERT_EQUAL(x, 30);
        ASSERT_SUCCESS(obj.at_pointer("/e").get(x));
        ASSERT_EQUAL(x, 120);
        TEST_SUCCEED();
    }
    bool many_json_pointers_array() {
        TEST_START();
        auto cfoofoo2 = R"( [ 111, 2, 3, { "foo": { "a": [ 10, 20, 33 ] }}, { "foo2": { "a": [ 10, 20, 30 ] }}, 1001 ])"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(cfoofoo2).get(doc));
        ondemand::array arr;
        ASSERT_SUCCESS(doc.get_array().get(arr));
        int64_t x;
        ASSERT_SUCCESS(arr.at_pointer("/3/foo/a/1").get(x));
        ASSERT_EQUAL(x, 20);
        TEST_SUCCEED();
    }
    struct car_type {
        std::string make;
        std::string model;
        uint64_t year;
        std::vector<double> tire_pressure;
        car_type(std::string_view _make, std::string_view _model, uint64_t _year,
          std::vector<double>&& _tire_pressure) :
          make{_make}, model{_model}, year(_year), tire_pressure(_tire_pressure) {}
    };

    bool json_pointer_invalidation() {
        TEST_START();
        auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;

        ondemand::parser parser;
        ondemand::document cars;
        std::vector<double> measured;
        ASSERT_SUCCESS(parser.iterate(cars_json).get(cars));
        std::vector<car_type> content;
        for (int i = 0; i < 3; i++) {
            ondemand::object obj;
            std::string json_pointer = std::string("/") + std::to_string(i);
            // Each successive at_pointer call invalidates
            // previously parsed values, strings, objects and array.
            ASSERT_SUCCESS(cars.at_pointer(json_pointer).get(obj));
            // We materialize the object.
            std::string_view make;
            ASSERT_SUCCESS(obj["make"].get(make));
            std::string_view model;
            ASSERT_SUCCESS(obj["model"].get(model));
            uint64_t year;
            ASSERT_SUCCESS(obj["year"].get(year));
            // We materialize the array.
            ondemand::array arr;
            ASSERT_SUCCESS(obj["tire_pressure"].get(arr));
            std::vector<double> values;
            for(auto x : arr) {
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

#if SIMDJSON_EXCEPTIONS
    bool json_pointer_invalidation_exceptions() {
        TEST_START();
        auto cars_json = R"( [
        { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
        { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
        { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
        ] )"_padded;

        ondemand::parser parser;
        std::vector<double> measured;
        ondemand::document cars = parser.iterate(cars_json);
        std::vector<car_type> content;
        for (int i = 0; i < 3; i++) {
            std::string json_pointer = std::string("/") + std::to_string(i);
            // Each successive at_pointer call invalidates
            // previously parsed values, strings, objects and array.
            ondemand::object obj(cars.at_pointer(json_pointer).get_object());
            // We materialize the object.
            std::string_view make = obj["make"];
            std::string_view model = obj["model"];
            uint64_t year(obj["year"]);
            // We materialize the array.
            ondemand::array arr(obj["tire_pressure"].get_array());
            std::vector<double> values;
            for(auto x : arr) {
                double value_double(x.get_double());
                values.push_back(value_double);
            }
            content.emplace_back(make, model, year, std::move(values));
        }
        std::string expected[] = {"Toyota", "Kia", "Toyota"};
        for (car_type c : content) {
            std::cout << c.make << " " << c.model << " " << c.year << "\n";
        }
        TEST_SUCCEED();
    }
#endif
    bool run() {
        return
#if SIMDJSON_EXCEPTIONS
                json_pointer_invalidation_exceptions() &&
#endif
                many_json_pointers_array() &&
                many_json_pointers_object() &&
                many_json_pointers_object_array() &&
                run_broken_tests() &&
                json_pointer_invalidation() &&
                demo_test() &&
                demo_relative_path() &&
                run_success_test(TEST_RFC_JSON,"/foo",R"(["bar", "baz"])") &&
                run_success_test(TEST_RFC_JSON,"/foo/0",R"("bar")") &&
                run_success_test(TEST_RFC_JSON,"/",R"(0)") &&
                run_success_test(TEST_RFC_JSON,"/a~1b",R"(1)") &&
                run_success_test(TEST_RFC_JSON,"/c%d",R"(2)") &&
                run_success_test(TEST_RFC_JSON,"/e^f",R"(3)") &&
                run_success_test(TEST_RFC_JSON,"/g|h",R"(4)") &&
                run_success_test(TEST_RFC_JSON,R"(/i\\j)",R"(5)") &&
                run_success_test(TEST_RFC_JSON,R"(/k\"l)",R"(6)") &&
                run_success_test(TEST_RFC_JSON,"/ ",R"(7)") &&
                run_success_test(TEST_RFC_JSON,"/m~0n",R"(8)") &&
                run_success_test(TEST_RFC_JSON,"",R"({
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
    })") &&
                run_success_test(TEST_JSON, "", R"({
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
    })") &&
                run_success_test(TEST_JSON, R"(/~1~001abc)", R"([
        0,
        {
            "\\\" 0": [
            "value0",
            "value1"
            ]
        }
        ])") &&
                run_success_test(TEST_JSON, R"(/~1~001abc/1)", R"({
            "\\\" 0": [
            "value0",
            "value1"
            ]
        })") &&
                run_success_test(TEST_JSON, R"(/~1~001abc/1/\\\" 0)", R"([
            "value0",
            "value1"
            ])") &&
                run_success_test(TEST_JSON, R"(/~1~001abc/1/\\\" 0/0)", "\"value0\"") &&
                run_success_test(TEST_JSON, R"(/~1~001abc/1/\\\" 0/1)", "\"value1\"") &&
                run_success_test(TEST_JSON, "/arr", R"([])") &&
                run_success_test(TEST_JSON, "/0", "\"0 ok\"") &&
                run_success_test(TEST_JSON, "/01", "\"01 ok\"") &&
                run_success_test(TEST_JSON, "", R"({
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
    })") &&
                run_failure_test(TEST_JSON, R"(/~1~001abc/1/\\\" 0/2)", INDEX_OUT_OF_BOUNDS) &&
                run_failure_test(TEST_JSON, "/arr/0", INDEX_OUT_OF_BOUNDS) &&
                run_failure_test(TEST_JSON, "~1~001abc", INVALID_JSON_POINTER) &&
                run_failure_test(TEST_JSON, "/~01abc", NO_SUCH_FIELD) &&
                run_failure_test(TEST_JSON, "/~1~001abc/01", INVALID_JSON_POINTER) &&
                run_failure_test(TEST_JSON, "/~1~001abc/", INVALID_JSON_POINTER) &&
                run_failure_test(TEST_JSON, "/~1~001abc/-", INDEX_OUT_OF_BOUNDS) &&
                many_json_pointers() &&
                document_as_scalar() &&
                true;
    }
}   // json_pointer_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, json_pointer_tests::run);
}