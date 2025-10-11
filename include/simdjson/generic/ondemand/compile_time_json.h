/**
 * @file compile_time_json.h
 * @brief Compile-time JSON parsing using C++26 P2996 reflection
 *
 * Parses JSON at compile-time and generates strongly-typed structs automatically.
 * Uses std::meta::substitute() pattern for recursive structures.
 *
 * @warning Requires C++26 with P2996R13 (experimental clang-p2996 compiler)
 *
 * Inspired by: https://brevzin.github.io/c++/2025/06/26/json-reflection/
 *              https://godbolt.org/z/Kn5b46T8j
 */

#ifndef SIMDJSON_GENERIC_COMPILE_TIME_JSON_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_COMPILE_TIME_JSON_H
#include "simdjson/generic/ondemand/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_STATIC_REFLECTION

#include <meta>
#include <string_view>
#include <vector>
#include <array>

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace compile_time {

/**
 * @brief Helper template for dynamic type generation via substitute()
 */
template <std::meta::info ...Ms>
struct Outer {
    struct Inner;
    consteval {
        std::meta::define_aggregate(^^Inner, {Ms...});
    }
};

template <std::meta::info ...Ms>
using Cls = Outer<Ms...>::Inner;

/**
 * @brief Helper template for aggregate initialization from reflected values
 */
template <typename T, auto ... Vs>
constexpr auto construct_from = T{Vs...};

/**
 * @brief Parsing state and helper functions
 */
struct ParseContext {
    std::string_view json;
    std::size_t pos = 0;

    constexpr ParseContext(std::string_view input) : json(input), pos(0) {}

    constexpr void skip_whitespace() {
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
               json[pos] == '\n' || json[pos] == '\r')) {
            ++pos;
        }
    }

    [[nodiscard]] constexpr char peek() const {
        return pos < json.size() ? json[pos] : '\0';
    }

    constexpr char consume() {
        return pos < json.size() ? json[pos++] : '\0';
    }

    constexpr bool expect(char ch) {
        skip_whitespace();
        if (peek() == ch) {
            consume();
            return true;
        }
        throw "expected character";
    }

    [[nodiscard]] constexpr bool match(std::string_view str) {
        if (json.substr(pos, str.size()) == str) {
            pos += str.size();
            return true;
        }
        return false;
    }

    [[nodiscard]] constexpr std::string_view parse_string() {
        if (!expect('"')) throw "expected quote";
        std::size_t start = pos;
        while (peek() != '"' && peek() != '\0') {
            if (peek() == '\\') {
                consume();
                if (pos < json.size()) consume();
            } else {
                consume();
            }
        }
        std::size_t end = pos;
        expect('"');
        return json.substr(start, end - start);
    }

    [[nodiscard]] constexpr double parse_number() {
        std::size_t start = pos;
        if (peek() == '-') consume();
        while (peek() >= '0' && peek() <= '9') consume();
        if (peek() == '.') {
            consume();
            while (peek() >= '0' && peek() <= '9') consume();
        }

        std::string_view num_str = json.substr(start, pos - start);
        double result = 0.0, sign = 1.0;
        std::size_t i = 0;

        if (i < num_str.size() && num_str[i] == '-') {
            sign = -1.0;
            ++i;
        }

        while (i < num_str.size() && num_str[i] >= '0' && num_str[i] <= '9') {
            result = result * 10.0 + (num_str[i] - '0');
            ++i;
        }

        if (i < num_str.size() && num_str[i] == '.') {
            ++i;
            double fraction = 0.0, divisor = 1.0;
            while (i < num_str.size() && num_str[i] >= '0' && num_str[i] <= '9') {
                fraction = fraction * 10.0 + (num_str[i] - '0');
                divisor *= 10.0;
                ++i;
            }
            result += fraction / divisor;
        }

        return result * sign;
    }

    [[nodiscard]] constexpr std::string_view extract_nested() {
        std::size_t start = pos;
        char open_char = peek();
        char close_char = (open_char == '{') ? '}' : ']';
        int depth = 0;
        do {
            if (peek() == open_char) depth++;
            if (peek() == close_char) depth--;
            consume();
        } while (depth > 0 && pos < json.size());
        if (depth != 0) throw "unclosed bracket";
        return json.substr(start, pos - start);
    }
};

consteval std::meta::info parse_json_impl(std::string_view json);
consteval std::meta::info parse_array_impl(std::string_view json);

consteval std::meta::info parse_array_impl(std::string_view json) {
    ParseContext ctx{json};
    ctx.skip_whitespace();

    if (!ctx.expect('[')) {
        throw "expected '['";
    }

    ctx.skip_whitespace();

    if (ctx.peek() == ']') {
        ctx.consume();
        return std::meta::substitute(^^construct_from, {^^std::array<void, 0>, ^^void});
    }

    std::vector<std::meta::info> element_values;

    while (ctx.peek() != ']') {
        ctx.skip_whitespace();
        char ch = ctx.peek();

        if (ch == '"') {
            auto str_value = ctx.parse_string();
            element_values.push_back(std::meta::reflect_constant_string(str_value));
        } else if (ch == 't' || ch == 'f') {
            bool bool_value = ctx.peek() == 't';
            if (bool_value) {
                if (!ctx.match("true")) throw "expected 'true'";
            } else {
                if (!ctx.match("false")) throw "expected 'false'";
            }
            element_values.push_back(std::meta::reflect_constant(bool_value));
        } else if (ch == 'n') {
            if (!ctx.match("null")) throw "expected 'null'";
            element_values.push_back(std::meta::reflect_constant(nullptr));
        } else if (ch == '{') {
            auto nested_json = ctx.extract_nested();
            std::meta::info parsed = parse_json_impl(nested_json);
            element_values.push_back(parsed);
        } else if (ch == '[') {
            auto nested_json = ctx.extract_nested();
            std::meta::info parsed = parse_array_impl(nested_json);
            element_values.push_back(parsed);
        } else if ((ch >= '0' && ch <= '9') || ch == '-') {
            double num_value = ctx.parse_number();
            element_values.push_back(std::meta::reflect_constant(num_value));
        } else {
            throw "unexpected array element type";
        }

        ctx.skip_whitespace();
        if (ctx.peek() == ',') {
            ctx.consume();
            ctx.skip_whitespace();
        } else {
            break;
        }
    }

    ctx.expect(']');

    if (element_values.empty()) {
        return std::meta::substitute(^^construct_from, {^^std::array<void, 0>, ^^void});
    }

    std::meta::info element_type = std::meta::type_of(element_values[0]);
    std::size_t count = element_values.size();
    std::meta::info array_type = std::meta::substitute(^^std::array, {element_type, std::meta::reflect_constant(count)});

    std::vector<std::meta::info> values;
    values.push_back(array_type);
    for (auto& elem : element_values) {
        values.push_back(elem);
    }

    return std::meta::substitute(^^construct_from, values);
}

/**
 * @brief Main compile-time JSON parser using substitute() pattern
 *
 * Returns std::meta::info containing type and value information.
 * This can be recursively called for nested objects without compiler crashes.
 */
consteval std::meta::info parse_json_impl(std::string_view json) {
    ParseContext ctx{json};
    ctx.skip_whitespace();

    if (!ctx.expect('{')) {
        throw "expected '{'";
    }

    std::vector<std::meta::info> members;
    std::vector<std::meta::info> values = {^^void};

    using std::meta::reflect_constant;

    while (ctx.peek() != '}') {
        ctx.skip_whitespace();
        if (ctx.peek() == '}') break;

        auto field_name = ctx.parse_string();
        ctx.skip_whitespace();
        ctx.expect(':');
        ctx.skip_whitespace();

        char ch = ctx.peek();

        if (ch == '"') {
            auto str_value = ctx.parse_string();
            auto dms = std::meta::data_member_spec(^^char const*, {.name=field_name});
            members.push_back(reflect_constant(dms));
            values.push_back(std::meta::reflect_constant_string(str_value));
        } else if (ch == 't' || ch == 'f') {
            bool bool_value = ctx.peek() == 't';
            if (bool_value) {
                if (!ctx.match("true")) throw "expected 'true'";
            } else {
                if (!ctx.match("false")) throw "expected 'false'";
            }
            auto dms = std::meta::data_member_spec(^^bool, {.name=field_name});
            members.push_back(reflect_constant(dms));
            values.push_back(reflect_constant(bool_value));
        } else if (ch == 'n') {
            if (!ctx.match("null")) throw "expected 'null'";
            auto dms = std::meta::data_member_spec(^^std::nullptr_t, {.name=field_name});
            members.push_back(reflect_constant(dms));
            values.push_back(reflect_constant(nullptr));
        } else if (ch == '[') {
            auto array_json = ctx.extract_nested();
            std::meta::info parsed = parse_array_impl(array_json);
            auto dms = std::meta::data_member_spec(std::meta::type_of(parsed), {.name=field_name});
            members.push_back(reflect_constant(dms));
            values.push_back(parsed);
        } else if (ch == '{') {
            auto nested_json = ctx.extract_nested();
            std::meta::info parsed = parse_json_impl(nested_json);
            auto dms = std::meta::data_member_spec(std::meta::type_of(parsed), {.name=field_name});
            members.push_back(reflect_constant(dms));
            values.push_back(parsed);
        } else if ((ch >= '0' && ch <= '9') || ch == '-') {
            double num_value = ctx.parse_number();
            auto dms = std::meta::data_member_spec(^^double, {.name=field_name});
            members.push_back(reflect_constant(dms));
            values.push_back(reflect_constant(num_value));
        } else {
            throw "unexpected value type";
        }

        ctx.skip_whitespace();
        if (ctx.peek() == ',') ctx.consume();
    }

    ctx.expect('}');

    values[0] = std::meta::substitute(^^Cls, members);
    return std::meta::substitute(^^construct_from, values);
}

/**
 * @brief JSON string wrapper for template parameters
 */
struct JSONString {
    std::meta::info Rep;
    consteval JSONString(const char *Json) : Rep{parse_json_impl(Json)} {}
};

/**
 * @brief Main parse_json function - template variable pattern
 *
 * Usage:
 *   constexpr auto config = json_to_object<R"({"port":8080,"host":"localhost"})">;
 *   static_assert(config.port == 8080);
 *   static_assert(std::string_view(config.host) == "localhost");
 */
template <JSONString json>
inline constexpr auto json_to_object = [:json.Rep:];

/**
 * @brief Alternative: User-defined literal for JSON parsing
 *
 * Usage:
 *   constexpr auto config = R"({"port":8080})"_json;
 */
template <JSONString json>
consteval auto operator""_json() {
    return [:json.Rep:];
}

/**
 * @brief Validate JSON syntax at compile-time
 */
template <JSONString json>
consteval bool validate_json() {
    return true;
}

} // namespace compile_time
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_STATIC_REFLECTION
#endif // SIMDJSON_GENERIC_COMPILE_TIME_JSON_H
