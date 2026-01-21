/**
 * @file compile_time_json.h
 * @brief Compile-time JSON parsing using C++26 reflection with
 * std::meta::substitute()
 */

#ifndef SIMDJSON_GENERIC_COMPILE_TIME_JSON_H
#define SIMDJSON_GENERIC_COMPILE_TIME_JSON_H

#if SIMDJSON_STATIC_REFLECTION

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <expected>
#include <meta>
#include <string>
#include <string_view>
#include <vector>

namespace simdjson {
namespace compile_time {

/**
 * @brief Compile-time JSON parser. This function parses the provided JSON
 * string at compile time and returns a custom struct type representing the JSON
 * object.
 *
 * We have a few limitations which trigger compile-time errors if violated:
 * - Only JSON objects and arrays are supported at the top level (no primitives).
 *   We will lift this limitation in the future.
 * - Strings are represented using the const char * in UTF-8, but they must not
 *   contain embedded nulls. We would prefer to represent them as std::string or
 *   std::string_view, and hope to do so in the future.
 * - Heterogeneous arrays are not supported yet. E.g., you need to have arrays of
 *   all integers, or all strings, all floats, all compatible objects, etc.
 *   For example, the following is accepted:
 *    [
 *     { "name": "Alice", "age": 30 },
 *     { "name": "Bob", "age": 25 },
 *     { "name": "Charlie", "age": 35 }
 *   ]
 *   but the following is not:
 *   [
 *     { "name": "Alice", "age": 30 },
 *     "Just a string",
 *     42,
 *     { "name": "Charlie", "age": 35 }
 *   ]
 *
 *    We may support heterogeneous arrays in the future with std::variant types.
 * - We parse the first JSON document encountered in the string. Trailing
 *   characters are ignored. Thus if your JSON begins with {"a":1}, everything
 *   after the closing } is ignored. This limitation will be lifted in the future,
 *   reporting an error.
 *
 * These limitations are safe in the sense that they result in compile-time errors.
 * Thus you will not get truncated strings or imprecise floats silently.
 *
 * This function is subject to change in the future.
 */
template <constevalutil::fixed_string json_str> consteval auto parse_json();

} // namespace compile_time
} // namespace simdjson


template <simdjson::constevalutil::fixed_string str>
consteval auto operator ""_json() {
  return simdjson::compile_time::parse_json<str>();
}

#endif // SIMDJSON_STATIC_REFLECTION
#endif // SIMDJSON_GENERIC_COMPILE_TIME_JSON_H
