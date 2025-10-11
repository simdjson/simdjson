/**
 * @file compile_time_json-inl.h
 * @brief Implementation details for compile-time JSON parsing
 *
 * This file contains inline implementations and helper utilities for compile-time
 * JSON parsing. Currently, the main implementation is self-contained in the header.
 */

#ifndef SIMDJSON_GENERIC_COMPILE_TIME_JSON_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_COMPILE_TIME_JSON_INL_H
#include "simdjson/generic/ondemand/compile_time_json.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_STATIC_REFLECTION

#include <meta>
#include <array>
#include <string_view>
#include <cstdint>

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace compile_time {

/**
 * @brief Optimized constexpr string to integer conversion
 *
 * This can be used for more efficient integer parsing in the future.
 * Currently, we parse all numbers as doubles for simplicity.
 */
constexpr int64_t parse_int_fast(std::string_view str) {
    int64_t result = 0;
    bool negative = false;
    std::size_t i = 0;

    if (i < str.size() && str[i] == '-') {
        negative = true;
        ++i;
    }

    while (i < str.size() && str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        ++i;
    }

    return negative ? -result : result;
}

/**
 * @brief Unescape JSON string at compile-time
 *
 * Currently, strings are returned as views into the original JSON.
 * This function can be used in the future for proper escape handling.
 */
template<std::size_t MaxLen = 1024>
constexpr auto unescape_json_string(std::string_view escaped) {
    std::array<char, MaxLen> result{};
    std::size_t out_pos = 0;
    std::size_t i = 0;

    while (i < escaped.size() && out_pos < MaxLen) {
        if (escaped[i] == '\\' && i + 1 < escaped.size()) {
            ++i;
            switch (escaped[i]) {
                case '"':  result[out_pos++] = '"'; break;
                case '\\': result[out_pos++] = '\\'; break;
                case '/':  result[out_pos++] = '/'; break;
                case 'b':  result[out_pos++] = '\b'; break;
                case 'f':  result[out_pos++] = '\f'; break;
                case 'n':  result[out_pos++] = '\n'; break;
                case 'r':  result[out_pos++] = '\r'; break;
                case 't':  result[out_pos++] = '\t'; break;
                case 'u':
                    // Unicode escape - would need proper implementation
                    // For now, skip the escape sequence
                    i += 4; // Skip 4 hex digits
                    break;
                default:
                    result[out_pos++] = escaped[i];
            }
            ++i;
        } else {
            result[out_pos++] = escaped[i];
            ++i;
        }
    }

    return std::pair{result, out_pos};
}

} // namespace compile_time
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_STATIC_REFLECTION
#endif // SIMDJSON_GENERIC_COMPILE_TIME_JSON_INL_H
