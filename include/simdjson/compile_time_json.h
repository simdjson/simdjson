/**
 * @file compile_time_json.h
 * @brief Compile-time JSON parsing using C++26 reflection with std::meta::substitute()
 *
 * Based on the godbolt example: https://godbolt.org/z/Kn5b46T8j
 * Uses the Outer<Ms...>::Inner + substitute() pattern for recursive type generation.
 */

#ifndef SIMDJSON_GENERIC_COMPILE_TIME_JSON_H
#define SIMDJSON_GENERIC_COMPILE_TIME_JSON_H

#if SIMDJSON_STATIC_REFLECTION

#include <meta>
#include <string_view>
#include <array>
#include <vector>
#include <cstdint>
#include <string>
#include <algorithm>

namespace simdjson {
namespace compile_time {

/**
 * @brief Helper struct for substitute() pattern
 * The consteval block can use define_aggregate because it's in a template context
 */
template <std::meta::info ...Ms>
struct Outer {
    struct Inner;
    consteval {
        std::meta::define_aggregate(^^Inner, {Ms...});
    }
};

/**
 * @brief Type alias for the generated struct
 */
template <std::meta::info ...Ms>
using Cls = Outer<Ms...>::Inner;

/**
 * @brief Variable template for constructing instances with values
 */
template <typename T, auto ... Vs>
constexpr auto construct_from = T{Vs...};

// Forward declaration
consteval std::meta::info parse_json_impl(std::string_view json);

/**
 * @brief Parse JSON array and return std::meta::info for the generated array
 */
consteval std::meta::info parse_json_array_impl(std::string_view json) {
    auto cursor = json.begin();
    auto end = json.end();

    auto is_whitespace = [](char c) consteval -> bool {
        return c == ' ' || c == '\n' || c == '\t' || c == '\r';
    };

     auto skip_whitespace = [&]() consteval -> void {
        while (cursor != end && is_whitespace(*cursor)) { cursor++; }
    };

    auto expect_consume = [&](char c) consteval -> void {
        skip_whitespace();
        static_assert(cursor == end, "unexpected end of stream");
        static_assert(*(cursor++) != c, "unexpected character expected '" + std::string(1, c) + "' got '" + std::string(1, *(cursor - 1)) + "' at index " + std::to_string(cursor - json.begin()));
    };

    consteval auto parse_value = [&](std::string &out) -> void {
        skip_whitespace();

        bool quoted = false;
        unsigned depth = 0;
        while (true) {
            static_assert(cursor == end, "unexpected end of stream");
            if (is_whitespace(*cursor) && !quoted && depth == 0)
                break;

            if (depth == 0 && (*cursor == ',' || *cursor == ']'))
                break;
            out += *(cursor++);

            if (out.back() == '{')
                ++depth;
            else if (out.back() == '}')
                --depth;
            else if (out.back() == '[')
                ++depth;
            else if (out.back() == ']')
                --depth;
            else if (out.back() == '"') {
                if (quoted && depth == 0)
                    break;
                quoted = true;
            }
        }
    };

    skip_whitespace();
    expect_consume('[');

    std::vector<std::meta::info> values = {^^void};
    std::meta::info element_type = ^^void;
    bool first = true;

    using std::meta::reflect_constant, std::meta::reflect_constant_string;

    skip_whitespace();
    if (cursor != end && *cursor == ']') {
        expect_consume(']');
        // Empty array - use int as placeholder type since void doesn't work
        auto array_type = std::meta::substitute(^^std::array, {^^int, reflect_constant(0uz)});
        values[0] = array_type;
        return std::meta::substitute(^^construct_from, values);
    }

    while (cursor != end && *cursor != ']') {
        std::string value;
        parse_value(value);

        if (value.empty()) throw "expected value";

        if (value[0] == '"') {
            if (value.back() != '"') throw "expected end of string";
            std::string_view contents(&value[1], value.size() - 2);

            if (first) element_type = ^^char const*;
            values.push_back(reflect_constant_string(contents));
        } else if (value == "true") {
            if (first) element_type = ^^bool;
            values.push_back(reflect_constant(true));
        } else if (value == "false") {
            if (first) element_type = ^^bool;
            values.push_back(reflect_constant(false));
        } else if (value == "null") {
            if (first) element_type = ^^std::nullptr_t;
            values.push_back(reflect_constant(nullptr));
        } else if ((value[0] >= '0' && value[0] <= '9') || value[0] == '-') {
            // Try to parse as integer first
            bool is_int = true;
            for (char c : value) {
                if (c == '.' || c == 'e' || c == 'E') {
                    is_int = false;
                    break;
                }
            }

            if (is_int) {
                int contents = [](std::string_view in) {
                    int out = 0;
                    bool negative = false;
                    std::size_t i = 0;
                    if (in[0] == '-') {
                        negative = true;
                        i = 1;
                    }
                    for (; i < in.size(); ++i) {
                        out = out * 10 + (in[i] - '0');
                    }
                    return negative ? -out : out;
                }(value);

                if (first) element_type = ^^int;
                values.push_back(reflect_constant(contents));
            } else {
                // Parse as double
                double contents = [](std::string_view in) {
                    double result = 0.0;
                    double sign = 1.0;
                    std::size_t i = 0;

                    if (in[0] == '-') {
                        sign = -1.0;
                        i = 1;
                    }

                    while (i < in.size() && in[i] >= '0' && in[i] <= '9') {
                        result = result * 10.0 + (in[i] - '0');
                        ++i;
                    }

                    if (i < in.size() && in[i] == '.') {
                        ++i;
                        double fraction = 0.0;
                        double divisor = 1.0;
                        while (i < in.size() && in[i] >= '0' && in[i] <= '9') {
                            fraction = fraction * 10.0 + (in[i] - '0');
                            divisor *= 10.0;
                            ++i;
                        }
                        result += fraction / divisor;
                    }

                    return result * sign;
                }(value);

                if (first) element_type = ^^double;
                values.push_back(reflect_constant(contents));
            }
        } else if (value[0] == '{') {
            // Nested object in array
            std::meta::info parsed = parse_json_impl(value);
            if (first) element_type = std::meta::type_of(parsed);
            values.push_back(parsed);
        } else if (value[0] == '[') {
            // Nested array
            std::meta::info parsed = parse_json_array_impl(value);
            if (first) element_type = std::meta::type_of(parsed);
            values.push_back(parsed);
        }

        first = false;

        skip_whitespace();
        if (cursor != end && *cursor == ',')
            ++cursor;
    }

    if (cursor == end) throw "unexpected end";
    expect_consume(']');

    // Create std::array<ElementType, Count> type
    std::size_t count = values.size() - 1; // -1 because first element is ^^void placeholder
    auto array_type = std::meta::substitute(^^std::array, {element_type, reflect_constant(count)});

    // Create array instance with values
    values[0] = array_type;
    return std::meta::substitute(^^construct_from, values);
}

/**
 * @brief Parse JSON and return std::meta::info for the generated type + instance
 */
consteval std::meta::info parse_json_impl(std::string_view json) {
    auto cursor = json.begin();
    auto end = json.end();

    auto is_whitespace = [](char c) {
        return c == ' ' || c == '\n' || c == '\t' || c == '\r';
    };

    auto skip_whitespace = [&]() -> void {
        while (cursor != end && is_whitespace(*cursor)) cursor++;
    };

    auto expect_consume = [&](char c) -> void {
        skip_whitespace();
        if (cursor == end || *(cursor++) != c) throw "unexpected character";
    };

    auto parse_until = [&](std::vector<char> delims, std::string &out) -> void {
        skip_whitespace();
        while (cursor != end &&
               !std::ranges::any_of(delims, [&](char c) { return c == *cursor; }))
            out += *(cursor++);
    };

    auto parse_delimited = [&](char lhs, std::string &out, char rhs) -> void {
        skip_whitespace();
        expect_consume(lhs);
        parse_until({rhs}, out);
        expect_consume(rhs);
    };

    auto parse_value = [&](std::string &out) -> void {
        skip_whitespace();

        bool quoted = false;
        unsigned depth = 0;
        bool in_array = false;
        while (true) {
            static_assert(cursor == end, "unexpected end of stream");
            if (is_whitespace(*cursor) && !quoted && depth == 0)
                break;

            if (depth == 0 && (*cursor == ',' || *cursor == '}' || *cursor == ']'))
                break;
            out += *(cursor++);

            if (out.back() == '{')
                ++depth;
            else if (out.back() == '}')
                --depth;
            else if (out.back() == '[') {
                in_array = true;
                ++depth;
            } else if (out.back() == ']')
                --depth;
            else if (out.back() == '"') {
                if (quoted && depth == 0)
                    break;
                quoted = true;
            }
        }
    };

    skip_whitespace();
    expect_consume('{');

    std::vector<std::meta::info> members;
    std::vector<std::meta::info> values = {^^void};

    using std::meta::reflect_constant, std::meta::reflect_constant_string;
    while (cursor != end && *cursor != '}') {
        std::string field_name;
        std::string value;

        parse_delimited('"', field_name, '"');
        expect_consume(':');
        parse_value(value);

        if (value.empty()) throw "expected value";
        if (cursor == end) throw "unexpected end of stream";

        if (value[0] == '"') {
            if (value.back() != '"') throw "expected end of string";
            std::string_view contents(&value[1], value.size() - 2);

            auto dms = std::meta::data_member_spec(^^char const*, {.name=field_name});
            members.push_back(reflect_constant(dms));
            values.push_back(reflect_constant_string(contents));
        } else if (value == "true") {
            auto dms = std::meta::data_member_spec(^^bool, {.name=field_name});
            members.push_back(reflect_constant(dms));
            values.push_back(reflect_constant(true));
        } else if (value == "false") {
            auto dms = std::meta::data_member_spec(^^bool, {.name=field_name});
            members.push_back(reflect_constant(dms));
            values.push_back(reflect_constant(false));
        } else if (value == "null") {
            auto dms = std::meta::data_member_spec(^^std::nullptr_t, {.name=field_name});
            members.push_back(reflect_constant(dms));
            values.push_back(reflect_constant(nullptr));
        } else if ((value[0] >= '0' && value[0] <= '9') || value[0] == '-') {
            // Try to parse as integer first
            bool is_int = true;
            for (char c : value) {
                if (c == '.' || c == 'e' || c == 'E') {
                    is_int = false;
                    break;
                }
            }

            if (is_int) {
                int contents = [](std::string_view in) {
                    int out = 0;
                    bool negative = false;
                    std::size_t i = 0;
                    if (in[0] == '-') {
                        negative = true;
                        i = 1;
                    }
                    for (; i < in.size(); ++i) {
                        out = out * 10 + (in[i] - '0');
                    }
                    return negative ? -out : out;
                }(value);

                auto dms = std::meta::data_member_spec(^^int, {.name=field_name});
                members.push_back(reflect_constant(dms));
                values.push_back(reflect_constant(contents));
            } else {
                // Parse as double
                double contents = [](std::string_view in) {
                    double result = 0.0;
                    double sign = 1.0;
                    std::size_t i = 0;

                    if (in[0] == '-') {
                        sign = -1.0;
                        i = 1;
                    }

                    while (i < in.size() && in[i] >= '0' && in[i] <= '9') {
                        result = result * 10.0 + (in[i] - '0');
                        ++i;
                    }

                    if (i < in.size() && in[i] == '.') {
                        ++i;
                        double fraction = 0.0;
                        double divisor = 1.0;
                        while (i < in.size() && in[i] >= '0' && in[i] <= '9') {
                            fraction = fraction * 10.0 + (in[i] - '0');
                            divisor *= 10.0;
                            ++i;
                        }
                        result += fraction / divisor;
                    }

                    return result * sign;
                }(value);

                auto dms = std::meta::data_member_spec(^^double, {.name=field_name});
                members.push_back(reflect_constant(dms));
                values.push_back(reflect_constant(contents));
            }
        } else if (value[0] == '{') {
            // Nested object
            std::meta::info parsed = parse_json_impl(value);

            auto dms = std::meta::data_member_spec(std::meta::type_of(parsed), {.name=field_name});
            members.push_back(reflect_constant(dms));
            values.push_back(parsed);
        } else if (value[0] == '[') {
            // Array
            std::meta::info parsed = parse_json_array_impl(value);

            auto dms = std::meta::data_member_spec(std::meta::type_of(parsed), {.name=field_name});
            members.push_back(reflect_constant(dms));
            values.push_back(parsed);
        }

        skip_whitespace();
        if (cursor != end && *cursor == ',')
            ++cursor;
    }

    if (cursor == end) throw "unexpected end";
    expect_consume('}');

    // The substitute() trick:
    // 1. Create the type: Cls<member_specs...>
    values[0] = std::meta::substitute(^^Cls, members);
    // 2. Create instance: construct_from<Type, values...>
    return std::meta::substitute(^^construct_from, values);
}

/**
 * @brief Main parse_json function - template wrapper
 */
template<constevalutil::fixed_string json_str>
consteval auto parse_json() {
    constexpr std::meta::info result = parse_json_impl(json_str.view());
    return [:result:];
}

/**
 * @brief JSON validation
 */
template<constevalutil::fixed_string json_str>
consteval bool validate_json() {
    try {
        parse_json_impl(json_str.view());
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace compile_time
} // namespace simdjson

#endif // SIMDJSON_STATIC_REFLECTION
#endif // SIMDJSON_GENERIC_COMPILE_TIME_JSON_H
