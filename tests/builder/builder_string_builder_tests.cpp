#include "simdjson.h"
#include "test_builder.h"
#include <string_view>
#include <string>
#include <map>

using namespace simdjson;

struct Car {
    std::string make;
    std::string model;
    int64_t year;
    std::vector<double> tire_pressure;
}; // Car

namespace builder_tests {
  using namespace std;

    bool allchar_test() {
        TEST_START();
        auto get_utf8_codepoints = []() -> std::string {
            std::string result;
            for (char32_t cp = 0; cp <= 0x10FFFF; ++cp) {
                if ((cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
                    continue; // Skip surrogate pairs and invalid codepoints
                }
                if (cp < 0x80) {
                    result += static_cast<char>(cp);
                } else if (cp < 0x800) {
                    result += static_cast<char>(0xC0 | (cp >> 6));
                    result += static_cast<char>(0x80 | (cp & 0x3F));
                } else if (cp < 0x10000) {
                    result += static_cast<char>(0xE0 | (cp >> 12));
                    result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    result += static_cast<char>(0x80 | (cp & 0x3F));
                } else {
                    result += static_cast<char>(0xF0 | (cp >> 18));
                    result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                    result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    result += static_cast<char>(0x80 | (cp & 0x3F));
                }
            }
            return result;
        };
        auto allutf8 = get_utf8_codepoints();
        simdjson::builder::string_builder sb;
        sb.start_object();
        sb.append_key_value("input", allutf8);
        sb.end_object();
        std::string_view p;
        ASSERT_TRUE(sb.validate_unicode());
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        simdjson::padded_string output = p;
        simdjson::ondemand::parser parser;
        simdjson::ondemand::document doc;
        ASSERT_SUCCESS(parser.iterate(output).get(doc));
        std::string_view recovered;
        ASSERT_SUCCESS(doc["input"].get(recovered));
        ASSERT_EQUAL(recovered, allutf8);
        simdjson::dom::parser domparser;
        simdjson::dom::element elem;
        ASSERT_SUCCESS(domparser.parse(output).get(elem));
        ASSERT_SUCCESS(elem["input"].get(recovered));
        ASSERT_EQUAL(recovered, allutf8);
        TEST_SUCCEED();
    }

    bool bad_utf8_test() {
        TEST_START();
        std::string bad_utf8 = "\xFF";
        simdjson::builder::string_builder sb;
        sb.start_object();
        sb.append_key_value("input", bad_utf8);
        sb.end_object();
        ASSERT_FALSE(sb.validate_unicode())
        TEST_SUCCEED();
    }
    #if SIMDJSON_EXCEPTIONS
    bool string_convertion_except() {
        TEST_START();
        simdjson::ondemand::parser p;
        simdjson::builder::string_builder sb;
        sb.append('a');
        std::string r(sb);
        ASSERT_EQUAL(r, "a");
        TEST_SUCCEED();
    }
    #endif


    bool append_char() {
        TEST_START();
        simdjson::builder::string_builder sb;
        sb.append('a');
        ASSERT_EQUAL(sb.size(), 1);
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "a");
        TEST_SUCCEED();
    }


    bool append_integer() {
        TEST_START();
        simdjson::builder::string_builder sb;
        sb.append(42);
        ASSERT_EQUAL(sb.size(), 2);
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "42");
        TEST_SUCCEED();
    }

    bool append_float() {
        TEST_START();
        simdjson::builder::string_builder sb;
        sb.append(1.1);
        ASSERT_EQUAL(sb.size(), 3);
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "1.1");
        TEST_SUCCEED();
    }

    bool append_null() {
        TEST_START();
        simdjson::builder::string_builder sb;
        sb.append_null();
        ASSERT_EQUAL(sb.size(), 4);
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "null");
        TEST_SUCCEED();
    }

    bool clear() {
        TEST_START();
        simdjson::builder::string_builder sb;
        sb.append('a');
        sb.clear();
        ASSERT_EQUAL(sb.size(), 0);
        TEST_SUCCEED();
    }

    bool escape_and_append() {
        TEST_START();
        simdjson::builder::string_builder sb;
        sb.escape_and_append("Hello, \"world\"!");
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "Hello, \\\"world\\\"!");
        TEST_SUCCEED();
    }

    bool escape_and_append_with_quotes() {
        TEST_START();
        simdjson::builder::string_builder sb;
        sb.escape_and_append_with_quotes("Hello, \"world\"!");
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "\"Hello, \\\"world\\\"!\"");
        TEST_SUCCEED();
    }

    bool various_integers() {
        TEST_START();
        std::vector<std::pair<int64_t, std::string_view>> test_cases = {
            {0, "0"},
            {1, "1"},
            {-1, "-1"},
            {42, "42"},
            {-42, "-42"},
            {100, "100"},
            {-100, "-100"},
            {999, "999"},
            {-999, "-999"},
            {2147483647, "2147483647"},  // max 32-bit integer
            {-2147483648, "-2147483648"}, // min 32-bit integer
            {4294967296ULL, "4294967296"}, // 2^32
            {-4294967296LL, "-4294967296"},
            {10000000000LL, "10000000000"}, // 10 billion
            {-10000000000LL, "-10000000000"},
            {9223372036854775807LL, "9223372036854775807"}, // max 64-bit integer
            {-9223372036854775807LL-1, "-9223372036854775808"}, // min 64-bit integer
            {1234567890123LL, "1234567890123"},
            {-1234567890123LL, "-1234567890123"},
        };
        for (const auto& [value, expected] : test_cases) {
            simdjson::builder::string_builder sb;
            sb.append(value);
            std::string_view p;
            auto result = sb.view().get(p);
            ASSERT_SUCCESS(result);
            ASSERT_EQUAL(p, expected);
        }
        TEST_SUCCEED();
    }

    bool various_unsigned_integers() {
        TEST_START();
        std::vector<std::pair<uint64_t, std::string_view>> test_cases = {
            {0, "0"},
            {1, "1"},
            {42, "42"},
            {100, "100"},
            {999, "999"},
            {2147483647, "2147483647"},  // max 32-bit integer
            {4294967296ULL, "4294967296"}, // 2^32
            {10000000000LL, "10000000000"}, // 10 billion
            {9223372036854775807LL, "9223372036854775807"}, // max 64-bit integer
            {1234567890123LL, "1234567890123"},
        };
        for (const auto& [value, expected] : test_cases) {
            simdjson::builder::string_builder sb;
            sb.append(value);
            std::string_view p;
            auto result = sb.view().get(p);
            ASSERT_SUCCESS(result);
            ASSERT_EQUAL(p, expected);
        }
        TEST_SUCCEED();
    }

    bool append_raw() {
        TEST_START();
        simdjson::builder::string_builder sb;
        sb.append_raw("Test");
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "Test");
        TEST_SUCCEED();
    }

    bool raw_with_length() {
        TEST_START();
        simdjson::builder::string_builder sb;
        sb.append_raw("Test String", 4);
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "Test");
        TEST_SUCCEED();
    }

    bool string_convertion() {
        TEST_START();
        simdjson::builder::string_builder sb;
        sb.append('a');
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "a");
        TEST_SUCCEED();
    }

    bool unicode_validation() {
        TEST_START();
        simdjson::builder::string_builder sb;
        sb.append('a');
        ASSERT_TRUE(sb.validate_unicode());
        TEST_SUCCEED();
    }

    bool buffer_growth() {
        TEST_START();
        simdjson::builder::string_builder sb;
        for(int i = 0; i < 3; ++i) {
            sb.append('a');
        }
        ASSERT_EQUAL(sb.size(), 3);
        TEST_SUCCEED();
    }

    void serialize_car_long(const Car& car, simdjson::builder::string_builder& builder) {
        // start of JSON
        builder.append_raw("{");

        // "make"
        builder.escape_and_append_with_quotes("make");
        builder.append_raw(":");
        builder.escape_and_append_with_quotes(car.make);

        // "model"
        builder.append_raw(",");
        builder.escape_and_append_with_quotes("model");
        builder.append_raw(":");
        builder.escape_and_append_with_quotes(car.model);

        // "year"
        builder.append_raw(",");
        builder.escape_and_append_with_quotes("year");
        builder.append_raw(":");
        builder.append(car.year);

        // "tire_pressure"
        builder.append_raw(",");
        builder.escape_and_append_with_quotes("tire_pressure");
        builder.append_raw(":[");

        // vector tire_pressure
        for (size_t i = 0; i < car.tire_pressure.size(); ++i) {
            builder.append(car.tire_pressure[i]);
            if (i < car.tire_pressure.size() - 1) {
                builder.append_raw(",");
            }
        }
        // end of array
        builder.append_raw("]");

        // end of object
        builder.append_raw("}");
    }

    bool car_test_long() {
        TEST_START();
        simdjson::builder::string_builder sb;
        Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
        serialize_car_long(c, sb);
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
        TEST_SUCCEED();
    }



    void serialize_car(const Car& car, simdjson::builder::string_builder& builder) {
        // start of JSON
        builder.start_object();

        // "make"
        builder.append_key_value("make", car.make);
        builder.append_comma();

        // "model"
        builder.append_key_value("model", car.model);

        builder.append_comma();

        // "year"
        builder.append_key_value("year", car.year);

        builder.append_comma();

        // "tire_pressure"
        builder.escape_and_append_with_quotes("tire_pressure");
        builder.append_colon();
        builder.start_array();
        // vector tire_pressure
        for (size_t i = 0; i < car.tire_pressure.size(); ++i) {
            builder.append(car.tire_pressure[i]);
            if (i < car.tire_pressure.size() - 1) {
                builder.append_comma();
            }
        }
        // end of array
        builder.end_array();

        // end of object
        builder.end_object();
    }


    bool car_test() {
        TEST_START();
        simdjson::builder::string_builder sb;
        Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
        serialize_car(c, sb);
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
        TEST_SUCCEED();
    }
    #if SIMDJSON_SUPPORTS_CONCEPTS
    bool serialize_optional() {
        TEST_START();
        simdjson::builder::string_builder sb;
        std::optional<std::string> optional_string = "Hello, World!";
        sb.append(optional_string);
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "\"Hello, World!\"");
        sb.clear();
        std::optional<std::string> optional_string_null = std::nullopt;
        sb.append(optional_string_null);
        result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "null");
        TEST_SUCCEED();
    }
    #endif

    #if SIMDJSON_SUPPORTS_RANGES && SIMDJSON_SUPPORTS_CONCEPTS
    void serialize_car_simple(const Car& car, simdjson::builder::string_builder& builder) {
        builder.start_object();
        builder.append_key_value("make", car.make);
        builder.append_comma();
        builder.append_key_value("model", car.model);
        builder.append_comma();
        builder.append_key_value("year", car.year);
        builder.append_comma();
        builder.append_key_value("tire_pressure", car.tire_pressure);
        builder.end_object();
    }
    bool car_test_simple() {
        TEST_START();
        simdjson::builder::string_builder sb;
        Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
        serialize_car(c, sb);
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
        TEST_SUCCEED();
    }

    bool car_test_simple_complete() {
        TEST_START();
        Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
        simdjson::builder::string_builder sb;
        sb.start_object();
        sb.append_key_value("make", c.make);
        sb.append_comma();
        sb.append_key_value("model", c.model);
        sb.append_comma();
        sb.append_key_value("year", c.year);
        sb.append_comma();
        sb.append_key_value("tire_pressure", c.tire_pressure);
        sb.end_object();
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
        TEST_SUCCEED();
    }

    bool map_test() {
        TEST_START();
        std::map<std::string,double> c = {{"key1", 1}, {"key2", 1}};
        simdjson::builder::string_builder sb;
        sb.append(c);
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "{\"key1\":1.0,\"key2\":1.0}");
                std::string s;

        ASSERT_SUCCESS(simdjson::to_json(c).get(s));
        ASSERT_EQUAL(s, "{\"key1\":1.0,\"key2\":1.0}");
        TEST_SUCCEED();
    }
    bool double_double_test() {
        TEST_START();
        std::vector<std::vector<double>> c = {{1.0, 2.0}, {3.0, 4.0}};
        simdjson::builder::string_builder sb;
        sb.append(c);
        std::string_view p;
        auto result = sb.view().get(p);
        ASSERT_SUCCESS(result);
        ASSERT_EQUAL(p, "[[1.0,2.0],[3.0,4.0]]");
        std::string s;
        ASSERT_SUCCESS(simdjson::to_json(c).get(s));
        ASSERT_EQUAL(s, "[[1.0,2.0],[3.0,4.0]]");
        TEST_SUCCEED();
    }
    #if SIMDJSON_EXCEPTIONS
    bool car_test_simple_complete_exceptions() {
        TEST_START();
        Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
        simdjson::builder::string_builder sb;
        sb.start_object();
        sb.append_key_value("make", c.make);
        sb.append_comma();
        sb.append_key_value("model", c.model);
        sb.append_comma();
        sb.append_key_value("year", c.year);
        sb.append_comma();
        sb.append_key_value("tire_pressure", c.tire_pressure);
        sb.end_object();
        std::string_view p = sb.view();
        ASSERT_EQUAL(p, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
        TEST_SUCCEED();
    }
    #endif // SIMDJSON_EXCEPTIONS
    #endif // SIMDJSONS_SUPPORT_RANGES


    #if SIMDJSON_EXCEPTIONS
    bool car_test_exception() {
        TEST_START();
        simdjson::builder::string_builder sb;
        Car c = {"Toyota", "Corolla", 2017, {30.0,30.2,30.513,30.79}};
        serialize_car_long(c, sb);
        std::string_view p{sb};
        ASSERT_EQUAL(p, "{\"make\":\"Toyota\",\"model\":\"Corolla\",\"year\":2017,\"tire_pressure\":[30.0,30.2,30.513,30.79]}");
        TEST_SUCCEED();
    }
    #endif
    bool run() {
        return
            allchar_test() &&
            bad_utf8_test() &&
            various_integers() &&
            various_unsigned_integers() &&
            car_test_long() &&
            car_test() &&
    #if SIMDJSON_EXCEPTIONS
            car_test_exception() &&
            string_convertion_except() &&
    #endif
    #if SIMDJSON_SUPPORTS_RANGES && SIMDJSON_SUPPORTS_CONCEPTS
            map_test() &&
            double_double_test() &&
            car_test_simple() &&
            car_test_simple_complete() &&
        #if SIMDJSON_EXCEPTIONS
            car_test_simple_complete_exceptions() &&
        #endif
    #endif
    #if SIMDJSON_SUPPORTS_CONCEPTS
           serialize_optional() &&
    #endif
            append_char() &&
            append_integer() &&
            append_float() &&
            append_null() &&
            clear() &&
            escape_and_append() &&
            escape_and_append_with_quotes() &&
            append_raw() &&
            raw_with_length() &&
            string_convertion() &&
            buffer_growth() &&
            unicode_validation() &&
            true;
    }

} // namespace builder_tests

int main(int argc, char *argv[]) {
  return test_main(argc, argv, builder_tests::run);
}
