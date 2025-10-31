/**
 * @file compile_time_json_tests.cpp
 * @brief Comprehensive tests for compile-time JSON parsing using C++26 P2996 reflection
 */
#include "test_macros.h"
#include "test_main.h"
#include "simdjson.h"

#include <print>

using namespace simdjson;
using namespace std::string_view_literals;

namespace compile_time_json_tests {
/**
 * Test 1: Basic object with primitives
 */
bool test_basic_object() {
    TEST_START();

    constexpr auto config = simdjson::compile_time::parse_json<R"({
        "port": 8080,
        "host": "localhost",
        "debug": true,
        "timeout": 30.5
    })">();

    static_assert(config.port == 8080);
    static_assert(std::string_view(config.host) == "localhost");
    static_assert(config.debug == true);
    static_assert(config.timeout == 30.5);

    ASSERT_EQUAL(config.port, 8080);
    ASSERT_EQUAL(std::string_view(config.host), "localhost"sv);
    ASSERT_TRUE(config.debug);
    ASSERT_EQUAL(config.timeout, 30.5);

    TEST_SUCCEED();
}

/**
 * Test 2: Nested objects
 */
bool test_nested_objects() {
    TEST_START();

    constexpr auto config = simdjson::compile_time::parse_json<R"({
        "server_port": 3000,
        "enable_ssl": true,
        "database": {
            "host": "db.example.com",
            "port": 5432,
            "timeout_sec": 30.0
        }
    })">();

    static_assert(config.server_port == 3000);
    static_assert(config.enable_ssl == true);
    static_assert(std::string_view(config.database.host) == "db.example.com");
    static_assert(config.database.port == 5432);
    static_assert(config.database.timeout_sec == 30.0);

    ASSERT_EQUAL(config.server_port, 3000);
    ASSERT_TRUE(config.enable_ssl);
    ASSERT_EQUAL(std::string_view(config.database.host), "db.example.com"sv);
    ASSERT_EQUAL(config.database.port, 5432);
    ASSERT_EQUAL(config.database.timeout_sec, 30.0);

    TEST_SUCCEED();
}

/**
 * Test 3: Deeply nested objects (3+ levels)
 */
bool test_deeply_nested_objects() {
    TEST_START();

    constexpr auto config = simdjson::compile_time::parse_json<R"({
        "app_name": "MyApp",
        "version": 1.5,
        "server": {
            "host": "api.example.com",
            "port": 443,
            "tls": {
                "enabled": true,
                "cert_path": "/etc/ssl/cert.pem",
                "min_version": 1.3
            }
        }
    })">();

    static_assert(std::string_view(config.app_name) == "MyApp");
    static_assert(config.version == 1.5);
    static_assert(std::string_view(config.server.host) == "api.example.com");
    static_assert(config.server.port == 443);
    static_assert(config.server.tls.enabled == true);
    static_assert(std::string_view(config.server.tls.cert_path) == "/etc/ssl/cert.pem");
    static_assert(config.server.tls.min_version == 1.3);

    ASSERT_EQUAL(std::string_view(config.app_name), "MyApp"sv);
    ASSERT_EQUAL(config.server.tls.enabled, true);
    ASSERT_EQUAL(config.server.tls.min_version, 1.3);

    TEST_SUCCEED();
}

/**
 * Test 4: Empty object
 */
bool test_empty_object() {
    TEST_START();

    constexpr auto config = simdjson::compile_time::parse_json<R"({})">();
    (void)config;  // Suppress unused warning

    TEST_SUCCEED();
}

/**
 * Test 5: Negative numbers
 */
bool test_negative_numbers() {
    TEST_START();

    constexpr auto data = simdjson::compile_time::parse_json<R"({
        "temperature": -273.15,
        "count": -42
    })">();

    static_assert(data.temperature == -273.15);
    static_assert(data.count == -42);

    ASSERT_EQUAL(data.temperature, -273.15);
    ASSERT_EQUAL(data.count, -42);

    TEST_SUCCEED();
}

/**
 * Test 6: Whitespace handling
 */
bool test_whitespace() {
    TEST_START();

    constexpr auto data = simdjson::compile_time::parse_json<R"(
        {
            "key1"  :  "value1"  ,
            "key2"  :  42
        }
    )">();

    static_assert(std::string_view(data.key1) == "value1");
    static_assert(data.key2 == 42);

    ASSERT_EQUAL(std::string_view(data.key1), "value1"sv);
    ASSERT_EQUAL(data.key2, 42);

    TEST_SUCCEED();
}

/**
 * Test 7: Real-time system configuration
 */
bool test_realtime_config() {
    TEST_START();

    constexpr auto config = simdjson::compile_time::parse_json<R"({
        "control_loop_hz": 1000,
        "max_acceleration": 9.8,
        "min_velocity": -50.0,
        "max_velocity": 50.0,
        "enable_safety_checks": true,
        "log_level": "INFO"
    })">();

    static_assert(config.control_loop_hz == 1000);
    static_assert(config.max_acceleration == 9.8);
    static_assert(config.enable_safety_checks == true);

    ASSERT_EQUAL(config.control_loop_hz, 1000);
    ASSERT_EQUAL(config.max_acceleration, 9.8);
    ASSERT_EQUAL(config.min_velocity, -50.0);
    ASSERT_EQUAL(config.max_velocity, 50.0);
    ASSERT_TRUE(config.enable_safety_checks);
    ASSERT_EQUAL(std::string_view(config.log_level), "INFO"sv);

    TEST_SUCCEED();
}

/**
 * Test 8: External JSON file (future #embed support)
 */
bool test_external_json_embed() {
    TEST_START();

    // Future C++26 with #embed:
    // constexpr auto config = simdjson::compile_time::parse_json<#embed "test_config.json">();

    // Current workaround - inline the JSON from test_config.json
    constexpr auto config = simdjson::compile_time::parse_json<R"({
  "system_name": "RealTimeController",
  "version": "2.1.0",
  "control_loop_hz": 1000,
  "max_latency_us": 500,
  "enable_diagnostics": true,
  "log_level": "INFO"
})">();

    static_assert(std::string_view(config.system_name) == "RealTimeController");
    static_assert(std::string_view(config.version) == "2.1.0");
    static_assert(config.control_loop_hz == 1000);
    static_assert(config.max_latency_us == 500);
    static_assert(config.enable_diagnostics == true);
    static_assert(std::string_view(config.log_level) == "INFO");

    ASSERT_EQUAL(std::string_view(config.system_name), "RealTimeController"sv);
    ASSERT_EQUAL(std::string_view(config.version), "2.1.0"sv);
    ASSERT_EQUAL(config.control_loop_hz, 1000);
    ASSERT_EQUAL(config.max_latency_us, 500);
    ASSERT_TRUE(config.enable_diagnostics);
    ASSERT_EQUAL(std::string_view(config.log_level), "INFO"sv);

    TEST_SUCCEED();
}



/**
 * Test 10: Null values
 */
bool test_null_values() {
    TEST_START();

    constexpr auto data = simdjson::compile_time::parse_json<R"({
        "nullable_field": null,
        "number": 42
    })">();

    static_assert(data.nullable_field == nullptr);
    static_assert(data.number == 42);

    ASSERT_EQUAL(data.nullable_field, nullptr);
    ASSERT_EQUAL(data.number, 42);

    TEST_SUCCEED();
}

/**
 * Test 11: Arrays of primitives
 */
bool test_arrays_primitives() {
    TEST_START();

    constexpr auto data = simdjson::compile_time::parse_json<R"({
        "values": [1, 2, 3, 4, 5],
        "flags": [true, false, true]
    })">();

    static_assert(data.values.size() == 5);
    static_assert(data.values[0] == 1);
    static_assert(data.values[4] == 5);
    static_assert(data.flags.size() == 3);
    static_assert(data.flags[0] == true);
    static_assert(data.flags[1] == false);

    ASSERT_EQUAL(data.values.size(), 5);
    ASSERT_EQUAL(data.values[0], 1);
    ASSERT_EQUAL(data.values[4], 5);
    ASSERT_EQUAL(data.flags.size(), 3);
    ASSERT_TRUE(data.flags[0]);
    ASSERT_FALSE(data.flags[1]);

    TEST_SUCCEED();
}

/**
 * Test 12: Arrays of objects
 */
bool test_arrays_of_objects() {
    TEST_START();

    constexpr auto data = simdjson::compile_time::parse_json<R"({
        "users": [
            {"name": "Alice", "age": 30},
            {"name": "Bob", "age": 25}
        ]
    })">();

    static_assert(data.users.size() == 2);
    static_assert(std::string_view(data.users[0].name) == "Alice");
    static_assert(data.users[0].age == 30);
    static_assert(std::string_view(data.users[1].name) == "Bob");
    static_assert(data.users[1].age == 25);

    ASSERT_EQUAL(data.users.size(), 2);
    ASSERT_EQUAL(std::string_view(data.users[0].name), "Alice"sv);
    ASSERT_EQUAL(data.users[0].age, 30);
    ASSERT_EQUAL(std::string_view(data.users[1].name), "Bob"sv);
    ASSERT_EQUAL(data.users[1].age, 25);

    TEST_SUCCEED();
}

/**
 * Test 13: Nested arrays in objects
 */
bool test_nested_arrays() {
    TEST_START();

    constexpr auto data = simdjson::compile_time::parse_json<R"({
        "config": {
            "ports": [8080, 8081, 8082]
        }
    })">();

    static_assert(data.config.ports.size() == 3);
    static_assert(data.config.ports[0] == 8080);
    static_assert(data.config.ports[2] == 8082);

    ASSERT_EQUAL(data.config.ports.size(), 3);
    ASSERT_EQUAL(data.config.ports[0], 8080);
    ASSERT_EQUAL(data.config.ports[2], 8082);

    TEST_SUCCEED();
}

/**
 * Test 14: Complex mixed structure with arrays and nested objects
 */
bool test_complex_mixed() {
    TEST_START();

    constexpr auto config = simdjson::compile_time::parse_json<R"({
        "app": "myapp",
        "version": 1.0,
        "config": {
            "ports": [8080, 8081, 8082],
            "enabled": true
        },
        "servers": [
            {"host": "server1", "port": 3000},
            {"host": "server2", "port": 3001}
        ]
    })">();

    static_assert(std::string_view(config.app) == "myapp");
    static_assert(config.version == 1.0);
    static_assert(config.config.ports[0] == 8080);
    static_assert(config.config.enabled == true);
    static_assert(std::string_view(config.servers[0].host) == "server1");
    static_assert(config.servers[1].port == 3001);

    ASSERT_EQUAL(std::string_view(config.app), "myapp"sv);
    ASSERT_EQUAL(config.config.ports[0], 8080);
    ASSERT_EQUAL(std::string_view(config.servers[0].host), "server1"sv);
    ASSERT_EQUAL(config.servers[1].port, 3001);

    TEST_SUCCEED();
}

/**
 * Test 15: Empty arrays
 */
bool test_empty_arrays() {
    TEST_START();

    constexpr auto data = simdjson::compile_time::parse_json<R"({
        "empty": []
    })">();

    static_assert(data.empty.size() == 0);

    ASSERT_EQUAL(data.empty.size(), 0);

    TEST_SUCCEED();
}

/**
 * Test 16: Simple object with string values
 */
bool test_simple_object_int() {
    TEST_START();

    constexpr auto config = simdjson::compile_time::parse_json<R"({
        "app": 1,
        "version": 2
    })">();
    std::print("App: {}, Version: {}\n",
        config.app,
        config.version
    );
    static_assert(config.app == 1);
    static_assert(config.version == 2);

    TEST_SUCCEED();
}

/**
 * Test 17: Simple object with string values
 */
bool test_simple_object_str() {
    TEST_START();

    constexpr auto config = simdjson::compile_time::parse_json<R"({
        "app": "ohai", "version": "1.0.0"
    })">();
    std::print("App: {}, Version: {}\n",
        config.app,
    config.version
    );
    static_assert(std::string_view(config.app) == "ohai");
    static_assert(std::string_view(config.version) == "1.0.0");

    TEST_SUCCEED();
}

/**
 * Test 18: Simple object with string values and nested object
 */
bool test_simple_object_str_with_obj() {
    TEST_START();
    constexpr auto config = simdjson::compile_time::parse_json<R"({"master": {
        "app": "ohai", "version": "1.0.0", "unicode": "\u00E9\u0074\u00E9\t"
    }})">();
    std::print("App: {}, Version: {}, Unicode: {}\n",
        config.master.app,
       config.master.version,
         config.master.unicode
    );
    static_assert(std::string_view(config.master.app) == "ohai");
    static_assert(std::string_view(config.master.version) == "1.0.0");
    static_assert(std::string_view(config.master.unicode) == "\xc3\xa9\x74\xc3\xa9\t");

    TEST_SUCCEED();
}

/**
 * Test 19: Simple array
 */
bool simple_array() {
    TEST_START();
    constexpr auto config = simdjson::compile_time::parse_json<R"(

    [ 1, 2, 3, 4, 5, 6 ]

    )">();
    std::print("Array size: {}\n",
        config.size()
    );
    static_assert(config.size() == 6);
    static_assert(config[0] == 1);
    static_assert(config[5] == 6);
    TEST_SUCCEED();
}


/**
 * Test 20: Array of objects
 */
bool array_of_objects() {
    TEST_START();
    constexpr auto config = simdjson::compile_time::parse_json<R"(

    [
      { "name": "Alice", "age": 30 },
      { "name": "Bob", "age": 25 },
      { "name": "Charlie", "age": 35 }
    ]

    )">();
    std::print("Array size: {}\n",
        config.size()
    );
    static_assert(config.size() == 3);
    static_assert(std::string_view(config[0].name) == "Alice");
    static_assert(config[1].age == 25);
    static_assert(std::string_view(config[2].name) == "Charlie");
    static_assert(config[2].age == 35);
    TEST_SUCCEED();
}

bool run() {
    return test_basic_object() &&
           test_nested_objects() &&
           test_deeply_nested_objects() &&
           test_empty_object() &&
           test_negative_numbers() &&
           test_whitespace() &&
           test_realtime_config() &&
           test_external_json_embed() &&
           test_null_values() &&
           test_arrays_primitives() &&
           test_arrays_of_objects() &&
           test_nested_arrays() &&
           test_complex_mixed() &&
           test_simple_object_str() &&
           test_simple_object_int() &&
           test_simple_object_str_with_obj() &&
           test_empty_arrays() &&
           simple_array() &&
           array_of_objects();
}

} // namespace compile_time_json_tests

int main(int argc, char *argv[]) {
#if SIMDJSON_STATIC_REFLECTION
    return test_main(argc, argv, compile_time_json_tests::run);
#else
    std::cout << "Compile-time JSON tests require SIMDJSON_STATIC_REFLECTION=ON" << std::endl;
    return EXIT_SUCCESS;
#endif
}
