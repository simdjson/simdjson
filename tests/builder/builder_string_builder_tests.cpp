#include "simdjson.h"
#include "test_builder.h"
#include <string_view>

using namespace simdjson;

struct Car {
    std::string make;
    std::string model;
    int64_t year;
    std::vector<double> tire_pressure;
}; // Car

namespace builder_tests {
  using namespace std;

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

    void serialize_car(const Car& car, simdjson::builder::string_builder& builder) {
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


    bool run() {
        return
            car_test() &&
    #if SIMDJSON_EXCEPTIONS
            string_convertion_except() &&
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
