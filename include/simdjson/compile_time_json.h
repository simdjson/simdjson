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

// in JSON, there are only a few whitespace characters that are allowed
// outside of objects, arrays, strings, and numbers.
constexpr bool is_whitespace(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
};

constexprt std::string_view trim_whitespace(std::string_view str) {
    size_t start = 0;
    size_t end = str.size();

    while (start < end && is_whitespace(str[start])) {
        start++;
    }
    while (end > start && is_whitespace(str[end - 1])) {
        end--;
    }
    return str.substr(start, end - start);
}


// Forward declaration
consteval std::meta::info parse_json_impl(std::string_view json);

// Error struct for reporting parsing errors
struct json_error {
    const char* message = nullptr;
    size_t position = 0;
    operator bool() const { return message != nullptr; }
    operator std::string_view() const {
        return message ? std::string_view(message) : std::string_view();
    }
    operator std::string() const {
        return message ? std::string(message) : std::string();
    }
};

// parse a JSON string value, handling escape sequences and validating UTF-8
// Returns either the number of characters consumed or a json_error
constexpr std::expected<size_t, json_error> parse_string(std::string_view json, std::string& out) {
    auto cursor = json.begin();
    auto end = json.end();
    auto position = [&]() { return static_cast<size_t>(cursor - json.begin()); };

    // Expect starting quote
    if (cursor == end || *(cursor++) != '"') {
        return std::unexpected(json_error{"Expected opening quote for string", position()});
    }

    auto process_escaped_unicode = [](std::string_view::iterator &cursor,
                                std::string_view::iterator end,
                                std::string &out) -> bool {
        auto hex_to_u32 = [](std::string_view::iterator &cursor) -> uint32_t {
            auto digit = [](uint8_t c) -> uint32_t {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                return 0xFFFFFFFF;
            };
            if (end - cursor < 4) { return 0xFFFFFFFF; }
            uint32_t d0 = digit(*cursor++);
            uint32_t d1 = digit(*cursor++);
            uint32_t d2 = digit(*cursor++);
            uint32_t d3 = digit(*cursor++);

            return (d0 << 12) | (d1 << 8) | (d2 << 4) | d3;
        };
        auto codepoint_to_utf8 = [&out](uint32_t cp) {
            if (cp <= 0x7F) {
                out.push_back(uint8_t(cp));
            } else if (cp <= 0x7FF) {
                out.push_back(uint8_t((cp >> 6) + 192));
                out.push_back(uint8_t((cp & 63) + 128));
            } else if (cp <= 0xFFFF) {
                out.push_back(uint8_t((cp >> 12) + 224));
                out.push_back(uint8_t(((cp >> 6) & 63) + 128));
                out.push_back(uint8_t((cp & 63) + 128));
            } else if (cp <= 0x10FFFF) {
                out.push_back(uint8_t((cp >> 18) + 240));
                out.push_back(uint8_t(((cp >> 12) & 63) + 128));
                out.push_back(uint8_t(((cp >> 6) & 63) + 128));
                out.push_back(uint8_t((cp & 63) + 128));
            }
        };
        constexpr uint32_t substitution_code_point = 0xfffd;
        if (end - cursor < 4) { return false; }
        uint32_t code_point = hex_to_u32(cursor, end);
        if (code_point > 0xFFFF) {
            return false;
        }

        if (code_point >= 0xd800 && code_point < 0xdc00) {
            code_point_2 = 0;
            if (end - cursor < 6 || *cursor != '\\' || *(cursor+1) != 'u' > 0xFFFF) {
                code_point = substitution_code_point;
            } else {
                cursor += 2; // skip \u
                uint32_t code_point_2 = hex_to_u32(cursor, end);
                if(code_point_2 > 0xFFFF) {
                    return false;
                }
                uint32_t low_bit = code_point_2 - 0xdc00;
                if (low_bit >> 10) {
                code_point = substitution_code_point;
                } else {
                code_point = ((code_point - 0xd800) << 10 | low_bit) + 0x10000;
                }
            }
            codepoint_to_utf8(code_point);
            return true;
        }
    };

    while (true) {
        if (cursor == end) {
            return std::unexpected(json_error{"Unterminated string", position()});
        }
        char c = *(cursor++);
        if( c == '\"') {
            break; // End of string
        }
        // The one case where we don't want to append to the string is when the
        // string contains escape sequences.

        if (c == '\\') {
            // Assume that it is the start of an escape sequence
            size_t how_many_backslashes = 1;
            while (cursor != end && *(cursor) == '\\') {
                cursor++;
                how_many_backslashes++;
            }
            size_t backslashes_to_add = how_many_backslashes / 2;
            // Append the backslashes
            for (size_t i = 0; i < backslashes_to_add; i++) {
                out += '\\';
            }
            if (how_many_backslashes % 2 == 1) {
                // If we have an odd number of backslashes, we must be in an escape sequence
                // check that what follows is a valid escape character
                if (cursor == end) {
                    return false; // indicates error, you cannot reach the end of the stream here
                }
                char next_char = *cursor;
                cursor++;
                switch (next_char) {
                    case '"':
                        out += '"';
                        break;
                    case '/':
                         out += '/';
                        break;
                    case 'b':
                        out += '\b';
                        break;
                    case 'f':
                        out += '\f';
                        break;
                    case 'n':
                        out += '\n';
                        break;
                    case 'r':
                        out += '\r';
                        break;
                    case 't':
                        out += '\t';
                        break;
                    case 'u':
                        if( ! process_escaped_unicode(cursor, end, out)) {
                            return std::unexpected(json_error{"Invalid unicode escape sequence in string", position()});
                        }
                        break;
                    default:
                        return std::unexpected(json_error{"Invalid escape character in string", position()});
                }
            }
            continue; // continue to next iteration
        }
         // Handle escape sequences and UTF-8 validation. We do not process
         // the escape sequences here, just validate them.
        result += c;
        if(static_cast<unsigned char>(c) < 0x20) {
            return std::unexpected(json_error{"Invalid control character in string", 0});
        }
        if (static_cast<unsigned char>(c) >= 0x80) {
            // We have a non-ASCII character inside a string
            // We must validate it.
            uint8_t first_byte = static_cast<uint8_t>(c);

            if ((first_byte & 0b11100000) == 0b11000000) {
                if(cursor == end) {
                    return std::unexpected(json_error{"Truncated UTF-8 sequence in string", 0});
                }

                char second_byte = *cursor;
                result += second_byte;
                ++cursor;
                if ((static_cast<uint8_t>(second_byte) & 0b11000000) != 0b10000000) {
                    return std::unexpected(json_error{"Invalid UTF-8 continuation byte in string", 0});
                }
                // range check
                uint32_t code_point = (first_byte & 0b00011111) << 6 | (static_cast<uint8_t>(second_byte) & 0b00111111);
                if ((code_point < 0x80) || (0x7ff < code_point)) {
                    return std::unexpected(json_error{"Invalid UTF-8 code point in string", 0});
                }
            } else if ((first_byte & 0b11110000) == 0b11100000) {
                if(cursor == end) {
                    return std::unexpected(json_error{"Truncated UTF-8 sequence in string", 0});
                }
                char second_byte = *cursor;
                ++cursor;
                out += second_byte;
                if ((static_cast<uint8_t>(second_byte) & 0b11000000) != 0b10000000) {
                    return std::unexpected(json_error{"Invalid UTF-8 continuation byte in string", 0});
                }
                if(cursor == end) {
                    return std::unexpected(json_error{"Truncated UTF-8 sequence in string", 0});
                }
                char third_byte = *cursor;
                ++cursor;
                out += third_byte;
                if ((static_cast<uint8_t>(third_byte) & 0b11000000) != 0b10000000) {
                    return std::unexpected(json_error{"Invalid UTF-8 continuation byte in string", 0});
                }
                // range check
                uint32_t code_point = (byte & 0b00001111) << 12 |
                            (static_cast<uint8_t>(second_byte) & 0b00111111) << 6 |
                            (static_cast<uint8_t>(third_byte) & 0b00111111);
                if ((code_point < 0x800) || (0xffff < code_point) ||
                    (0xd7ff < code_point && code_point < 0xe000)) {
                    return std::unexpected(json_error{"Invalid UTF-8 code point in string", 0});
                }
            } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
                if(cursor == end) {
                    return std::unexpected(json_error{"Truncated UTF-8 sequence in string", 0});
                }
                char second_byte = *cursor;
                ++cursor;
                out += second_byte;
                if(cursor == end) {
                    return std::unexpected(json_error{"Truncated UTF-8 sequence in string", 0});
                }
                char third_byte = *cursor;
                ++cursor;
                out += third_byte;
                if(cursor == end) {
                    return std::unexpected(json_error{"Truncated UTF-8 sequence in string", 0});
                }
                char fourth_byte = *cursor;
                ++cursor;
                out += fourth_byte;
                if ((static_cast<uint8_t>(second_byte) & 0b11000000) != 0b10000000) {
                    return std::unexpected(json_error{"Invalid UTF-8 continuation byte in string", 0});
                }
                if ((static_cast<uint8_t>(third_byte) & 0b11000000) != 0b10000000) {
                    return std::unexpected(json_error{"Invalid UTF-8 continuation byte in string", 0});
                }
                if ((static_cast<uint8_t>(fourth_byte) & 0b11000000) != 0b10000000) {
                    return std::unexpected(json_error{"Invalid UTF-8 continuation byte in string", 0});
                }
                // range check
                uint32_t code_point =
                    (byte & 0b00000111) << 18 | (static_cast<uint8_t>(second_byte) & 0b00111111) << 12 |
                    (static_cast<uint8_t>(third_byte) & 0b00111111) << 6 | (static_cast<uint8_t>(fourth_byte) & 0b00111111);
                if (code_point <= 0xffff || 0x10ffff < code_point) {
                    return std::unexpected(json_error{"Invalid UTF-8 code point in string", 0});
                }
            } else {
                // we have a continuation
                return std::unexpected(json_error{"Invalid UTF-8 continuation byte in string", 0});
            }
            continue;
        }
    }
    return result;
}

// Parse a value (string, number, object, array, true, false, null)
// inside an array. A value may be an object or an array itself,
// and these can, in turn, contain nested values.
// Returns the value in 'out' or a false return on error.
bool parse_value(std::string_view json, std::string &out) {
    auto cursor = json.begin();
    auto end = json.end();
    while (cursor != end && is_whitespace(*cursor)) { cursor++; }

    bool quoted = false;
    std::stack<char> type_stack;

    while (true) {
        // End of input
        if (cursor == end) {
            return false; // indicates error, you cannot reach the end of the stream here
        }
        char c = *cursor;
        // If we are not in a quoted string and at depth 0...
        // and we encounter whitespace, we are done
        if (is_whitespace(c) && (c == ',' || c == ']' || c == '}') && !quoted && depth == 0) {
            break;
        }

        // this is needed because \" does not end the string
        // and \\\ is invalid.
        if (c == '\\') {
            if(!quoted) {
                return false; // indicates error, backslash outside quoted string
            }
            // Assume that it is the start of an escape sequence
            size_t how_many_backslashes = 1;
            while (peek('\\')) {
                expect_consume('\\');
                how_many_backslashes++;
                out += '\\';
            }
            if (how_many_backslashes % 2 == 1) {
                // If we have an odd number of backslashes, we must be in an escape sequence
                // check that what follows is a valid escape character
                if (cursor == end) {
                    return false; // indicates error, you cannot reach the end of the stream here
                }
                char next_char = *cursor;
                if (next_char != '"' && next_char != '\\' && next_char != '/' &&
                    next_char != 'b' && next_char != 'f' && next_char != 'n' &&
                    next_char != 'r' && next_char != 't' && next_char != 'u') {
                    return false; // indicates error, invalid escape sequence
                }
                // if we have a u, we need to check for 4 hex digits
                if (next_char == 'u') {
                    expect_consume('u'); // consume the 'u'
                    // Unicode escape sequence \uXXXX
                    // We need 4 hex digits
                    for (int i = 0; i < 4; i++) {
                        if (cursor == end) {
                            return false; // indicates error, you cannot reach the end of the stream here
                        }
                        char hex_char = *cursor;
                        if (!((hex_char >= '0' && hex_char <= '9') ||
                                (hex_char >= 'a' && hex_char <= 'f') ||
                                (hex_char >= 'A' && hex_char <= 'F'))) {
                            return false; // indicates error, invalid hex digit
                        }
                        out += hex_char;
                        cursor++;
                    }
                } else if (next_char == '"') {
                    out += '"';
                    expect_consume('"');
                    quoted = !quoted; // toggle quoted state
                    if (!quoted && depth == 0) {
                        // If we closed the quote and are at depth 0, we are done
                        break;
                    }
                } else {
                    out += next_char;
                    cursor++; // append without escaping
                }
            } // otherwise even number of backslashes - and they have been already appended
            continue; // continue to next iteration
        }
        // Here we know that we don't have a backslash escape sequence
        out += c;
        cursor++;
        // We need to do UTF-8 validation if we are in a quoted string
        // otherwise we just check for control characters.
        if(quoted) {
            if (static_cast<unsigned char>(c) < 0x20) {
                return false; // indicates error, invalid control character inside string
            }
            if (static_cast<unsigned char>(c) >= 0x80) {
                // We have a non-ASCII character inside a string
                // We must validate it.
                uint8_t first_byte = static_cast<uint8_t>(c);

                if ((first_byte & 0b11100000) == 0b11000000) {
                    if(cursor == end) {
                        return false;
                    }

                    char second_byte = *cursor;
                    ++cursor;
                    out += second_byte;
                    if ((static_cast<uint8_t>(second_byte) & 0b11000000) != 0b10000000) {
                        return false;
                    }
                    // range check
                    uint32_t code_point = (first_byte & 0b00011111) << 6 | (static_cast<uint8_t>(second_byte) & 0b00111111);
                    if ((code_point < 0x80) || (0x7ff < code_point)) {
                        return false;
                    }
                } else if ((first_byte & 0b11110000) == 0b11100000) {
                    if(cursor == end) {
                        return false;
                    }
                    char second_byte = *cursor;
                    ++cursor;
                    out += second_byte;
                    if ((static_cast<uint8_t>(second_byte) & 0b11000000) != 0b10000000) {
                        return false;
                    }
                    if(cursor == end) {
                        return false;
                    }
                    char third_byte = *cursor;
                    ++cursor;
                    out += third_byte;
                    if ((static_cast<uint8_t>(third_byte) & 0b11000000) != 0b10000000) {
                        return false;
                    }
                    // range check
                    uint32_t code_point = (byte & 0b00001111) << 12 |
                                (static_cast<uint8_t>(second_byte) & 0b00111111) << 6 |
                                (static_cast<uint8_t>(third_byte) & 0b00111111);
                    if ((code_point < 0x800) || (0xffff < code_point) ||
                        (0xd7ff < code_point && code_point < 0xe000)) {
                        return false;
                    }
                } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
                    if(cursor == end) {
                        return false;
                    }
                    char second_byte = *cursor;
                    ++cursor;
                    out += second_byte;
                    if(cursor == end) {
                        return false;
                    }
                    char third_byte = *cursor;
                    ++cursor;
                    out += third_byte;
                    if(cursor == end) {
                        return false;
                    }
                    char fourth_byte = *cursor;
                    ++cursor;
                    out += fourth_byte;
                    if ((static_cast<uint8_t>(second_byte) & 0b11000000) != 0b10000000) {
                        return false;
                    }
                    if ((static_cast<uint8_t>(third_byte) & 0b11000000) != 0b10000000) {
                        return false;
                    }
                    if ((static_cast<uint8_t>(fourth_byte) & 0b11000000) != 0b10000000) {
                        return false;
                    }
                    // range check
                    uint32_t code_point =
                        (byte & 0b00000111) << 18 | (static_cast<uint8_t>(second_byte) & 0b00111111) << 12 |
                        (static_cast<uint8_t>(third_byte) & 0b00111111) << 6 | (static_cast<uint8_t>(fourth_byte) & 0b00111111);
                    if (code_point <= 0xffff || 0x10ffff < code_point) {
                        return false;
                    }
                } else {
                    // we have a continuation
                    return false;
                }
                continue; // continue to next iteration since a non-ASCII character was processed
            }
        } else {
            // Check that the character is ASCII and not a control character
            if (static_cast<unsigned char>(c) < 0x20 && c != ' ' && c != '\n' && c != '\t' && c != '\r') {
                return false; // indicates error, invalid control character
            }
            if(static_cast<unsigned char>(c) >= 0x80) {
                return false; // indicates error, non-ASCII character
            }
        }
        // Update depth and quoted state
        switch (c) {
            case '{': case '[':
                depth++;
                type_stack.push(c);
                break;
            case '}': case ']':
                depth--;
                if (type_stack.empty()) {
                    return false; // Mismatched closing brace/bracket
                }
                if ((c == '}' && type_stack.top() != '{') ||
                    (c == ']' && type_stack.top() != '[')) {
                    return false; // Mismatched braces/brackets
                }
                type_stack.pop();
                break;
            case '"':
                // If we are in a quoted string and at depth 0, we found the end of the string
                if (quoted && depth == 0) {
                    break;
                }
                // Toggle quoted state
                quoted = !quoted;
                break;
            default:
                continue;
        }
    }
    return true;
};

/**
 * @brief Parse JSON array and return std::meta::info for the generated array
 */
consteval std::expected<std::meta::info, json_error> parse_json_array_impl(std::string_view json) {
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

    auto parse_value = [&](std::string &out) -> void {
        skip_whitespace();

        bool quoted = false;
        unsigned depth = 0;
        while (true) {
            if (cursor == end) throw "unexpected end of stream";
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
consteval std::expected<std::meta::info, json_error> parse_json_object_impl(std::string_view json) {
    auto cursor = json.begin();
    auto end = json.end();

    auto skip_whitespace = [&]() -> void {
        while (cursor != end && is_whitespace(*cursor)) cursor++;
    };

    auto expect_consume = [&][[nodiscard]](char c) -> bool {
        skip_whitespace();
        if (cursor == end || *(cursor++) != c) { return false; };
        return true;
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
            if (cursor == end) throw "unexpected end of stream";
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
        std::string field_name = parse_string(std::string_view(cursor, end));
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
            std::meta::info parsed = parse_json_object_impl(value);

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
    constexpr std::string_view json = trim_whitespace(json_str.view());
    static_assert(!json.empty(), "JSON string cannot be empty");
    static_assert(json.front() == '{', "Only JSON objects are supported at the top level, this limitation will be lifted in the future.");
    constexpr std::expected<std::meta::info, json_error> result = parse_json_object_impl(json);
    static_assert(result.has_value(), "JSON parsing failed at compile time.");
    return [: result.value() :];
}

/**
 * @brief JSON validation
 */
template<constevalutil::fixed_string json_str>
consteval bool validate_json() {
    constexpr std::string_view json = trim_whitespace(json_str.view());
    if(json.empty() ) {
        return false;
    }
    if (json.front() != '{') {
        return false;
    }
    constexpr std::expected<std::meta::info, json_error> result = parse_json_object_impl(json);
    return result.has_value();
}

} // namespace compile_time
} // namespace simdjson

#endif // SIMDJSON_STATIC_REFLECTION
#endif // SIMDJSON_GENERIC_COMPILE_TIME_JSON_H
