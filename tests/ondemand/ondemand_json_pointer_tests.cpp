#include "simdjson.h"
#include "test_ondemand.h"

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
        ondemand::parser parser;
        ondemand::document doc;
        ondemand::value val;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        ASSERT_SUCCESS(doc.at_pointer(json_pointer).get(val));
        std::string actual = simdjson::to_string(val);
        ASSERT_EQUAL(actual,expected);
        TEST_SUCCEED();
    }

    bool run_failure_test(const padded_string & json,std::string_view json_pointer,error_code expected) {
        TEST_START();
        ondemand::parser parser;
        ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        ASSERT_EQUAL(doc.at_pointer(json_pointer).error(),expected);
        TEST_SUCCEED();
    }

    bool rfc_tests() {
        TEST_START();
        ASSERT_TRUE(run_success_test(TEST_RFC_JSON,"/foo",R"(["bar","baz"])"));
        ASSERT_TRUE(run_success_test(TEST_RFC_JSON,"/foo/0",R"("bar")"));
        ASSERT_TRUE(run_success_test(TEST_RFC_JSON,"/",R"(0)"));
        ASSERT_TRUE(run_success_test(TEST_RFC_JSON,"/a~1b",R"(1)"));
        ASSERT_TRUE(run_success_test(TEST_RFC_JSON,"/c%d",R"(2)"));
        ASSERT_TRUE(run_success_test(TEST_RFC_JSON,"/e^f",R"(3)"));
        ASSERT_TRUE(run_success_test(TEST_RFC_JSON,"/g|h",R"(4)"));
        ASSERT_TRUE(run_success_test(TEST_RFC_JSON,R"(/i\\j)",R"(5)"));
        ASSERT_TRUE(run_success_test(TEST_RFC_JSON,R"(/k\"l)",R"(6)"));
        ASSERT_TRUE(run_success_test(TEST_RFC_JSON,"/ ",R"(7)"));
        ASSERT_TRUE(run_success_test(TEST_RFC_JSON,"/m~0n",R"(8)"));
        TEST_SUCCEED();
    }

    bool test_json_success_tests() {
        TEST_START();
        ASSERT_TRUE(run_success_test(TEST_JSON, "", R"({"/~01abc":[0,{"\\\" 0":["value0","value1"]}],"0":"0 ok","01":"01 ok","":"empty ok","arr":[]})"));
        ASSERT_TRUE(run_success_test(TEST_JSON, R"(/~1~001abc)", R"([0,{"\\\" 0":["value0","value1"]}])"));
        ASSERT_TRUE(run_success_test(TEST_JSON, R"(/~1~001abc/1)", R"({"\\\" 0":["value0","value1"]})"));
        ASSERT_TRUE(run_success_test(TEST_JSON, R"(/~1~001abc/1/\\\" 0)", R"(["value0","value1"])"));
        ASSERT_TRUE(run_success_test(TEST_JSON, R"(/~1~001abc/1/\\\" 0/0)", "\"value0\""));
        ASSERT_TRUE(run_success_test(TEST_JSON, R"(/~1~001abc/1/\\\" 0/1)", "\"value1\""));
        ASSERT_TRUE(run_success_test(TEST_JSON, "/arr", R"([])"));
        ASSERT_TRUE(run_success_test(TEST_JSON, "/0", "\"0 ok\""));
        ASSERT_TRUE(run_success_test(TEST_JSON, "/01", "\"01 ok\""));
        ASSERT_TRUE(run_success_test(TEST_JSON, "", R"({"/~01abc":[0,{"\\\" 0":["value0","value1"]}],"0":"0 ok","01":"01 ok","":"empty ok","arr":[]})"));
        TEST_SUCCEED();
    }

    bool test_json_failure_tests() {
        TEST_START();
        ASSERT_TRUE(run_failure_test(TEST_JSON, R"(/~1~001abc/1/\\\" 0/2)", INDEX_OUT_OF_BOUNDS));
        ASSERT_TRUE(run_failure_test(TEST_JSON, "/arr/0", INDEX_OUT_OF_BOUNDS));
        ASSERT_TRUE(run_failure_test(TEST_JSON, "~1~001abc", INVALID_JSON_POINTER));
        ASSERT_TRUE(run_failure_test(TEST_JSON, "/~01abc", NO_SUCH_FIELD));
        ASSERT_TRUE(run_failure_test(TEST_JSON, "/~1~001abc/01", INVALID_JSON_POINTER));
        ASSERT_TRUE(run_failure_test(TEST_JSON, "/~1~001abc/", INVALID_JSON_POINTER));
        ASSERT_TRUE(run_failure_test(TEST_JSON, "/~1~001abc/-", INDEX_OUT_OF_BOUNDS));
        TEST_SUCCEED();
    }

    bool empty_json_pointer() {
        TEST_START();
        auto json = R"( { "make": "Toyota", "model": "Camry" } )"_padded;
        ondemand::parser parser;
        ondemand::document doc;
        ondemand::value val;
        ASSERT_SUCCESS(parser.iterate(json).get(doc));
        ASSERT_SUCCESS(doc.at_pointer("").get(val));
        std::string expected = R"({"make":"Toyota","model":"Camry"})";
        std::string actual = simdjson::to_string(val);
        ASSERT_EQUAL(actual,expected);

        auto json2 = R"( [1,2,3] )"_padded;
        ASSERT_SUCCESS(parser.iterate(json2).get(doc));
        ASSERT_SUCCESS(doc.at_pointer("").get(val));
        std::string expected2 = R"([1,2,3])";
        std::string actual2 = simdjson::to_string(val);
        ASSERT_EQUAL(actual2,expected2);

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


    bool run() {
        return
                empty_json_pointer() &&
                demo_test() &&
                demo_relative_path() &&
                rfc_tests() &&
                test_json_success_tests() &&
                test_json_failure_tests() &&
                true;
    }
}

int main(int argc, char *argv[]) {
  return test_main(argc, argv, json_pointer_tests::run);
}