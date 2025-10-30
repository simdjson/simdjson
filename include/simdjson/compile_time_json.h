/**
 * @file compile_time_json.h
 * @brief Compile-time JSON parsing using C++26 reflection with
 * std::meta::substitute()
 *
 * Based on the godbolt example: https://godbolt.org/z/Kn5b46T8j
 * Uses the Outer<Ms...>::Inner + substitute() pattern for recursive type
 * generation.
 */

#ifndef SIMDJSON_GENERIC_COMPILE_TIME_JSON_H
#define SIMDJSON_GENERIC_COMPILE_TIME_JSON_H

#if SIMDJSON_STATIC_REFLECTION

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <meta>
#include <string>
#include <string_view>
#include <vector>

namespace simdjson {
namespace compile_time {

/**
 * @brief Helper struct for substitute() pattern
 * The consteval block can use define_aggregate because it's in a template
 * context
 */
template <std::meta::info... Ms> struct Outer {
  struct Inner;
  consteval {
    std::meta::define_aggregate(^^Inner, {
                                             Ms...});
  }
};

/**
 * @brief Type alias for the generated struct
 */
template <std::meta::info... Ms> using Cls = Outer<Ms...>::Inner;

/**
 * @brief Variable template for constructing instances with values
 */
template <typename T, auto... Vs> constexpr auto construct_from = T{Vs...};

// in JSON, there are only a few whitespace characters that are allowed
// outside of objects, arrays, strings, and numbers.
constexpr [[nodiscard]] bool is_whitespace(char c) {
  return c == ' ' || c == '\n' || c == '\t' || c == '\r';
};

constexpr [[nodiscard]] std::string_view trim_whitespace(std::string_view str) {
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
consteval std::meta::info parse_json_object_impl(std::string_view json);

// Error struct for reporting parsing errors
struct json_error {
  const char *message = nullptr;
  size_t position = 0;
  operator bool() const { return message != nullptr; }
  operator std::string_view() const {
    return message ? std::string_view(message) : std::string_view();
  }
  operator std::string() const {
    return message ? std::string(message) : std::string();
  }
};

constexpr [[nodiscard]] std::expected<size_t, json_error>
parse_number(std::string_view json,
             std::variant<int64_t, uint64_t, double> &out) {
  if (json.empty()) {
    return std::unexpected(json_error{"Empty input for number parsing", 0});
  }
  if (json[0] == '+') {
    return std::unexpected(
        json_error{"Invalid number: leading '+' sign is not allowed", 0});
  }
  auto is_digit_or_sign = [](char c) {
    return (c >= '0' && c <= '9') || c == '-';
  };
  auto it = std::find_if_not(suffix.begin(), suffix.end(), is_digit_or_sign);
  bool is_float = false;
  if (it != suffix.end() && (*it == '.' || *it == 'e' || *it == 'E')) {
    is_float = true;
  }
  bool is_negative = suffix[0] == '-';
  // Note that we consider -0 to be an integer unless it has a decimal point or
  // exponent.
  if (is_float) {
    // We must do some extra validation that std::from_chars won't do.
    // 1. A sign must be followed by at least one digit.
    // 2. There must be at least one digit before an optional decimal point.
    // 3. The value parsed must be finite (NaN and Inf are not valid JSON
    // numbers).
    if (is_negative &&
        (suffix.size() == 1 || !(suffix[1] >= '0' && suffix[1] <= '9'))) {
      return std::unexpected(json_error{"Invalid float value", 0});
    }
    if (!is_negative && !(suffix[0] >= '0' && suffix[0] <= '9')) {
      return std::unexpected(json_error{"Invalid float value", 0});
    }
    double float_value = 0.0;
    std::from_chars_result res = std::from_chars(
        suffix.data(), suffix.data() + suffix.size(), float_value);
    if (res.ec == std::errc() && std::isfinite(float_value)) {
      out = float_value;
      return (res.ptr - suffix.data());
    } else {
      return std::unexpected(json_error{"Invalid float value", 0});
    }
    break;
  } else if (is_negative) {
    int64_t int_value = 0;
    std::from_chars_result res = std::from_chars(
        suffix.data(), suffix.data() + suffix.size(), int_value);
    if (res.ec == std::errc()) {
      out = int_value;
      return (res.ptr - suffix.data());
    } else {
      return std::unexpected(json_error{"Invalid integer value", 0});
    }
    break;
  } else {
    uint64_t uint_value = 0;
    std::from_chars_result res = std::from_chars(
        suffix.data(), suffix.data() + suffix.size(), uint_value);
    if (res.ec == std::errc()) {
      out = uint_value;
      return (res.ptr - suffix.data());
    } else {
      return std::unexpected(json_error{"Invalid unsigned integer value", 0});
    }
    break;
  }
}

// parse a JSON string value, handling escape sequences and validating UTF-8
// Returns either the number of characters consumed or a json_error.
// Note that the number of characters consumed includes the surrounding quotes.
// The number of bytes written to out differs from the number of characters
// consumed in general because of escape sequences and UTF-8 encoding.
constexpr [[nodiscard]] std::expected<size_t, json_error>
parse_string(std::string_view json, std::string &out) {
  auto cursor = json.begin();
  auto end = json.end();
  auto position = [&]() { return static_cast<size_t>(cursor - json.begin()); };

  // Expect starting quote
  if (cursor == end || *(cursor++) != '"') {
    return std::unexpected(
        json_error{"Expected opening quote for string", position()});
  }
  // Notice that the quote is not appended.

  // Process \uXXXX escape sequences, writes out to out.
  // Returns true on success, false on error.
  auto process_escaped_unicode = [&] [[nodiscard]] () -> bool {
    // Processing \uXXXX escape sequence is a bit complicated, so we
    // define helper lambdas here.
    //
    // consume the XXXX in \uXXXX and return the corresponding code point.
    // In case of error, a value greater than 0xFFFF is returned.
    // The caller should check!
    auto hex_to_u32 = [] [[nodiscard]] (std::string_view::iterator &
                                        cursor) -> uint32_t {
      auto digit = [](uint8_t c) -> uint32_t {
        if (c >= '0' && c <= '9')
          return c - '0';
        if (c >= 'A' && c <= 'F')
          return c - 'A' + 10;
        if (c >= 'a' && c <= 'f')
          return c - 'a' + 10;
        return 0xFFFFFFFF;
      };
      if (end - cursor < 4) {
        return 0xFFFFFFFF;
      }
      uint32_t d0 = digit(*cursor++);
      uint32_t d1 = digit(*cursor++);
      uint32_t d2 = digit(*cursor++);
      uint32_t d3 = digit(*cursor++);

      return (d0 << 12) | (d1 << 8) | (d2 << 4) | d3;
    };
    // Write code point as UTF-8 into out.
    // The caller must ensure that the code point is valid,
    // i.e., not in the surrogate range or greater than 0x10FFFF.
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
    // We begin the implementation of process_escaped_unicode here.
    // The substitution_code_point is used for invalid sequences.
    // (This is U+FFFD, the Unicode replacement character.)
    constexpr uint32_t substitution_code_point = 0xfffd;
    // We need at least 4 characters for the hex digits
    if (end - cursor < 4) {
      return false;
    }
    // Consume the 4 hex digits
    uint32_t code_point = hex_to_u32(cursor, end);
    // Check for validity
    if (code_point > 0xFFFF) {
      return false;
    }
    // If we have a high surrogate, we need to process a low surrogate
    if (code_point >= 0xd800 && code_point < 0xdc00) {
      // We need at least 6 more characters for \uXXXX, if they are NOT
      // present, we have an error (isolated high surrogate), which we
      // tolerate by substituting the substitution_code_point.
      if (end - cursor < 6 || *cursor != '\\' ||
          *(cursor + 1) != 'u' > 0xFFFF) {
        code_point = substitution_code_point;
      } else {       // we have \u following the high surrogate
        cursor += 2; // skip \u
        uint32_t code_point_2 = hex_to_u32(cursor, end);
        if (code_point_2 > 0xFFFF) {
          return false;
        }
        uint32_t low_bit = code_point_2 - 0xdc00;
        if (low_bit >> 10) {
          // Not a valid low surrogate
          code_point = substitution_code_point;
        } else {
          code_point = ((code_point - 0xd800) << 10 | low_bit) + 0x10000;
        }
      }
      // Now we have the final code point, write it as UTF-8 to out.
      codepoint_to_utf8(code_point);
      // We are done, success!
      return true;
    }
  };

  while (cursor != end && *cursor != '"') {
    // If we find the end of input before closing quote, it's an error
    if (cursor == end) {
      return std::unexpected(json_error{"Unterminated string", position()});
    }
    // capture the next character and move forward
    char c = *(cursor++);
    // The one case where we don't want to append to the string directly
    // is when the string contains escape sequences.

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
      // If we have an odd number of backslashes, we have an escape sequence
      // besides the backslashes we have already added.
      if (how_many_backslashes % 2 == 1) {
        // If we have an odd number of backslashes, we must be in an escape
        // sequence check that what follows is a valid escape character
        if (cursor == end) {
          return false; // indicates error, you cannot reach the end of the
                        // stream here
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
          if (!process_escaped_unicode()) {
            return std::unexpected(json_error{
                "Invalid unicode escape sequence in string", position()});
          }
          break;
        default:
          return std::unexpected(
              json_error{"Invalid escape character in string", position()});
        }
      }
      continue; // continue to next iteration
    }
    // Handle escape sequences and UTF-8 validation. We do not process
    // the escape sequences here, just validate them.
    result += c;
    if (static_cast<unsigned char>(c) < 0x20) {
      return std::unexpected(
          json_error{"Invalid control character in string", position()});
    }
    if (static_cast<unsigned char>(c) >= 0x80) {
      // We have a non-ASCII character inside a string
      // We must validate it.
      uint8_t first_byte = static_cast<uint8_t>(c);

      if ((first_byte & 0b11100000) == 0b11000000) {
        if (cursor == end) {
          return std::unexpected(
              json_error{"Truncated UTF-8 sequence in string", position()});
        }

        char second_byte = *cursor;
        result += second_byte;
        ++cursor;
        if ((static_cast<uint8_t>(second_byte) & 0b11000000) != 0b10000000) {
          return std::unexpected(json_error{
              "Invalid UTF-8 continuation byte in string", position()});
        }
        // range check
        uint32_t code_point = (first_byte & 0b00011111) << 6 |
                              (static_cast<uint8_t>(second_byte) & 0b00111111);
        if ((code_point < 0x80) || (0x7ff < code_point)) {
          return std::unexpected(
              json_error{"Invalid UTF-8 code point in string", position()});
        }
      } else if ((first_byte & 0b11110000) == 0b11100000) {
        if (cursor == end) {
          return std::unexpected(
              json_error{"Truncated UTF-8 sequence in string", position()});
        }
        char second_byte = *cursor;
        ++cursor;
        out += second_byte;
        if ((static_cast<uint8_t>(second_byte) & 0b11000000) != 0b10000000) {
          return std::unexpected(json_error{
              "Invalid UTF-8 continuation byte in string", position()});
        }
        if (cursor == end) {
          return std::unexpected(
              json_error{"Truncated UTF-8 sequence in string", position()});
        }
        char third_byte = *cursor;
        ++cursor;
        out += third_byte;
        if ((static_cast<uint8_t>(third_byte) & 0b11000000) != 0b10000000) {
          return std::unexpected(json_error{
              "Invalid UTF-8 continuation byte in string", position()});
        }
        // range check
        uint32_t code_point = (byte & 0b00001111) << 12 |
                              (static_cast<uint8_t>(second_byte) & 0b00111111)
                                  << 6 |
                              (static_cast<uint8_t>(third_byte) & 0b00111111);
        if ((code_point < 0x800) || (0xffff < code_point) ||
            (0xd7ff < code_point && code_point < 0xe000)) {
          return std::unexpected(
              json_error{"Invalid UTF-8 code point in string", position()});
        }
      } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
        if (cursor == end) {
          return std::unexpected(
              json_error{"Truncated UTF-8 sequence in string", position()});
        }
        char second_byte = *cursor;
        ++cursor;
        out += second_byte;
        if (cursor == end) {
          return std::unexpected(
              json_error{"Truncated UTF-8 sequence in string", position()});
        }
        char third_byte = *cursor;
        ++cursor;
        out += third_byte;
        if (cursor == end) {
          return std::unexpected(
              json_error{"Truncated UTF-8 sequence in string", position()});
        }
        char fourth_byte = *cursor;
        ++cursor;
        out += fourth_byte;
        if ((static_cast<uint8_t>(second_byte) & 0b11000000) != 0b10000000) {
          return std::unexpected(json_error{
              "Invalid UTF-8 continuation byte in string", position()});
        }
        if ((static_cast<uint8_t>(third_byte) & 0b11000000) != 0b10000000) {
          return std::unexpected(json_error{
              "Invalid UTF-8 continuation byte in string", position()});
        }
        if ((static_cast<uint8_t>(fourth_byte) & 0b11000000) != 0b10000000) {
          return std::unexpected(json_error{
              "Invalid UTF-8 continuation byte in string", position()});
        }
        // range check
        uint32_t code_point =
            (byte & 0b00000111) << 18 |
            (static_cast<uint8_t>(second_byte) & 0b00111111) << 12 |
            (static_cast<uint8_t>(third_byte) & 0b00111111) << 6 |
            (static_cast<uint8_t>(fourth_byte) & 0b00111111);
        if (code_point <= 0xffff || 0x10ffff < code_point) {
          return std::unexpected(
              json_error{"Invalid UTF-8 code point in string", position()});
        }
      } else {
        // we have a continuation
        return std::unexpected(json_error{
            "Invalid UTF-8 continuation byte in string", position()});
      }
      continue;
    }
  }
  // We get here if and only if we have seen the closing quote.
  return position();
}

// Parses a JSON array and returns a std::meta::info representing the array type
// or a json_error in case of failure. The input json string_view is updated
// upon successful parsing to represent just the parsed portion.
consteval std::expected<std::meta::info, json_error>
parse_json_array_impl(std::string_view &json) {
  auto cursor = json.begin();
  auto end = json.end();

  auto is_whitespace = [](char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '\r';
  };

  auto skip_whitespace = [&]() -> void {
    while (cursor != end && is_whitespace(*cursor))
      cursor++;
  };

  auto expect_consume = [&] [[nodiscard]] (char c) -> bool {
    skip_whitespace();
    if (cursor == end || *(cursor++) != c) {
      return false;
    };
    return true;
  };

  skip_whitespace();
  if (!expect_consume('['))
    return std::unexpected(json_error{"Expected '['", position()});

  std::vector<std::meta::info> values = {^^void};
  std::meta::info element_type = ^^void;
  bool first = true;

  // using std::meta::reflect_constant, std::meta::reflect_constant_string;

  skip_whitespace();
  if (cursor != end && *cursor == ']') {
    if (!expect_consume(']'))
      return std::unexpected(json_error{"Expected ']'", position()});
    skip_whitespace();
    // update the input json
    json = std::string_view(cursor, end);
    // Empty array - use int as placeholder type since void doesn't work
    auto array_type =
        std::meta::substitute(^^std::array, {
                                                ^^int, reflect_constant(0uz)});
    values[0] = array_type;
    return std::meta::substitute(^^construct_from, values);
  }
  while (cursor != end && *cursor != ']') {
    char c = *cursor;
    switch (c) {
    case '{': {
      std::string_view value(cursor, end);
      std::meta::info parsed = parse_json_object_impl(value);
      auto dms = std::meta::data_member_spec(std::meta::type_of(parsed),
                                             {.name = field_name});
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(parsed);
      cursor += (end - cursor) - value.size();
      break;
    }
    case '[': {
      std::string_view value(cursor, end);
      std::meta::info parsed = parse_json_array_impl(value);
      auto dms = std::meta::data_member_spec(std::meta::type_of(parsed),
                                             {.name = field_name});
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(parsed);
      cursor += (end - cursor) - value.size();
      break;
    }
    case '"': {
      std::string value;
      bool res = parse_string(std::string_view(cursor, end), value);
      if (!res) {
        return std::unexpected(json_error{"Invalid value", position()});
      }
      cursor += res.value();
      auto dms =
          std::meta::data_member_spec(^^std::string, {
                                                         .name = field_name});
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(std::meta::reflect_object(value));
      break;
    }
    case 't': {
      if (end - cursor < 4 || std::string_view(cursor, 4) != "true") {
        return std::unexpected(json_error{"Invalid value", position()});
      }
      cursor += 4;
      auto dms = std::meta::data_member_spec(^^bool, {
                                                         .name = field_name});
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(std::meta::reflect_constant(true));
      break;
    }
    case 'f': {
      if (end - cursor < 5 || std::string_view(cursor, 5) != "false") {
        return std::unexpected(json_error{"Invalid value", position()});
      }
      cursor += 5;
      auto dms = std::meta::data_member_spec(^^bool, {
                                                         .name = field_name});
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(std::meta::reflect_constant(false));
      break;
    }
    case 'n': {
      if (end - cursor < 4 || std::string_view(cursor, 4) != "null") {
        return std::unexpected(json_error{"Invalid value", position()});
      }
      cursor += 4;
      auto dms = std::meta::data_member_spec(^^std::nullptr_t,
                                             {
                                                 .name = field_name});
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(std::meta::reflect_constant(nullptr));
      break;
    }
    // We deliberately do not include '+' here, as per JSON spec
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      std::string_view suffix = std::string_view(cursor, end);
      std::variant<int64_t, uint64_t, double> out;
      std::expected<size_t, json_error> r = parse_number(suffix, out);
      if (!r) {
        return std::unexpected(json_error{"Invalid number", position()});
      }
      cursor += r.value();
      if (std::holds_alternative<int64_t>(out)) {
        int64_t int_value = std::get<int64_t>(out);
        auto dms =
            std::meta::data_member_spec(^^int64_t, {
                                                       .name = field_name});
        members.push_back(std::meta::reflect_constant(dms));
        values.push_back(std::meta::reflect_constant(int_value));
      } else if (std::holds_alternative<uint64_t>(out)) {
        uint64_t uint_value = std::get<uint64_t>(out);
        auto dms =
            std::meta::data_member_spec(^^uint64_t, {
                                                        .name = field_name});
        members.push_back(std::meta::reflect_constant(dms));
        values.push_back(std::meta::reflect_constant(uint_value));
      } else {
        double float_value = std::get<double>(out);
        auto dms =
            std::meta::data_member_spec(^^double, {
                                                      .name = field_name});
        members.push_back(std::meta::reflect_constant(dms));
        values.push_back(std::meta::reflect_constant(float_value));
      }
      break;
    }
    default:
      return std::unexpected(
          json_error{"Invalid character starting value", position()});
    }
    skip_whitespace();
    if (cursor != end && *cursor == ',') {
      ++cursor;
      skip_whitespace();
    }
  }

  if (!expect_consume(']'))
    return std::unexpected(json_error{"Expected ']'", position()});
  skip_whitespace();
  // update the input json
  json = std::string_view(cursor, end);
  // Create std::array<ElementType, Count> type
  std::size_t count =
      values.size() - 1; // -1 because first element is ^^void placeholder
  auto array_type = std::meta::substitute(
      ^^std::array, {
                        element_type, reflect_constant(count)});

  // Create array instance with values
  values[0] = array_type;
  return std::meta::substitute(^^construct_from, values);
}

// Parses a JSON object and returns a std::meta::info representing the object
// type or a json_error in case of failure. The input json string_view is
// updated upon successful parsing to represent just the parsed portion.
consteval std::expected<std::meta::info, json_error>
parse_json_object_impl(std::string_view &json) {
  auto cursor = json.begin();
  auto end = json.end();

  auto skip_whitespace = [&]() -> void {
    while (cursor != end && is_whitespace(*cursor))
      cursor++;
  };

  auto expect_consume = [&] [[nodiscard]] (char c) -> bool {
    skip_whitespace();
    if (cursor == end || *(cursor++) != c) {
      return false;
    };
    return true;
  };

  if (!expect_consume('{'))
    return std::unexpected(json_error{"Expected '{'", position()});

  std::vector<std::meta::info> members;
  std::vector<std::meta::info> values = {^^void};

  while (cursor != end && *cursor != '}') {
    // Not all strings can be identifiers, but in JSON field names can be any
    // string. Thus we may have a problem if the field name contains characters
    // not allowed in identifiers. Let the standard library handle that case.
    std::string field_name;
    std::expected<size_t, json_error> field =
        parse_string(std::string_view(cursor, end));
    if (!field) {
      return std::unexpected(field.error());
    }
    cursor += field.value();
    if (!expect_consume(':')) {
      return std::unexpected(json_error{"Expected ':'", position()});
    }
    skip_whitespace();
    if (cursor == end) {
      return std::unexpected(
          json_error{"Expected value after colon", position()});
    }
    char c = *cursor;
    switch (c) {
    case '{': {
      std::meta::info parsed = parse_json_object_impl(value);
      auto dms = std::meta::data_member_spec(std::meta::type_of(parsed),
                                             {.name = field_name});
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(parsed);
      break;
    }
    case '[': {
      std::meta::info parsed = parse_json_array_impl(value);
      auto dms = std::meta::data_member_spec(std::meta::type_of(parsed),
                                             {.name = field_name});
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(parsed);
      break;
    }
    case '"': {
      std::string value;
      bool res = parse_string(std::string_view(cursor, end), value);
      if (!res) {
        return std::unexpected(json_error{"Invalid value", position()});
      }
      cursor += res.value();

      auto dms =
          std::meta::data_member_spec(^^std::string, {
                                                         .name = field_name});
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(std::meta::reflect_object(value));
      break;
    }
    case 't': {
      if (end - cursor < 4 || std::string_view(cursor, 4) != "true") {
        return std::unexpected(json_error{"Invalid value", position()});
      }
      cursor += 4;

      auto dms = std::meta::data_member_spec(^^bool, {
                                                         .name = field_name});
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(std::meta::reflect_constant(true));
      break;
    }
    case 'f': {
      if (end - cursor < 5 || std::string_view(cursor, 5) != "false") {
        return std::unexpected(json_error{"Invalid value", position()});
      }
      cursor += 5;

      auto dms = std::meta::data_member_spec(^^bool, {
                                                         .name = field_name});
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(std::meta::reflect_constant(false));
      break;
    }
    case 'n': {
      if (end - cursor < 4 || std::string_view(cursor, 4) != "null") {
        return std::unexpected(json_error{"Invalid value", position()});
      }
      cursor += 4;

      auto dms = std::meta::data_member_spec(^^std::nullptr_t,
                                             {
                                                 .name = field_name});
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(std::meta::reflect_constant(nullptr));
      break;
    }
    // We deliberately do not include '+' here, as per JSON spec
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      std::string_view suffix = std::string_view(cursor, end);
      std::variant<int64_t, uint64_t, double> out;
      std::expected<size_t, json_error> r = parse_number(suffix, out);
      if (!r) {
        return std::unexpected(json_error{"Invalid number", position()});
      }
      cursor += r.value();
      if (std::holds_alternative<int64_t>(out)) {
        int64_t int_value = std::get<int64_t>(out);
        auto dms =
            std::meta::data_member_spec(^^int64_t, {
                                                       .name = field_name});
        members.push_back(std::meta::reflect_constant(dms));
        values.push_back(std::meta::reflect_constant(int_value));
      } else if (std::holds_alternative<uint64_t>(out)) {
        uint64_t uint_value = std::get<uint64_t>(out);
        auto dms =
            std::meta::data_member_spec(^^uint64_t, {
                                                        .name = field_name});
        members.push_back(std::meta::reflect_constant(dms));
        values.push_back(std::meta::reflect_constant(uint_value));
      } else {
        double float_value = std::get<double>(out);
        auto dms =
            std::meta::data_member_spec(^^double, {
                                                      .name = field_name});
        members.push_back(std::meta::reflect_constant(dms));
        values.push_back(std::meta::reflect_constant(float_value));
      }
      break;
    }
    default:
      return std::unexpected(
          json_error{"Invalid character starting value", position()});
    }
    skip_whitespace();
    if (cursor != end && *cursor == ',') {
      ++cursor;
      skip_whitespace();
    }
  }

  if (!expect_consume('}'))
    return std::unexpected(json_error{"Expected '}'", position()});
  skip_whitespace();
  // update the input json
  json = std::string_view(cursor, end);
  // The substitute() trick:
  // 1. Create the type: Cls<member_specs...>
  values[0] = std::meta::substitute(^^Cls, members);
  // 2. Create instance: construct_from<Type, values...>
  return std::meta::substitute(^^construct_from, values);
}

/**
 * @brief Compile-time JSON parser. This function parses the provided JSON
 * string at compile time and returns a custom struct type representing the JSON
 * object. If you want to validate JSON without generating a type, use
 * validate_json().
 */
template <constevalutil::fixed_string json_str> consteval auto parse_json() {
  constexpr std::string_view json = trim_whitespace(json_str.view());
  static_assert(!json.empty(), "JSON string cannot be empty");
  static_assert(json.front() == '{',
                "Only JSON objects are supported at the top level, this "
                "limitation will be lifted in the future.");
  constexpr std::string_view expected_json = json;
  constexpr std::expected<std::meta::info, json_error> result =
      parse_json_object_impl(json);
  static_assert(result.has_value(), "JSON parsing failed at compile time.");
  static_assert(expected_json == json,
                "Extra characters found after JSON object.");
  return [:result.value():];
}

/**
 * @brief JSON validation. This function checks if the provided JSON string is
 * valid. It returns true if the JSON is valid, false otherwise.
 */
template <constevalutil::fixed_string json_str> consteval bool validate_json() {
  constexpr std::string_view json = trim_whitespace(json_str.view());
  if (json.empty()) {
    return false;
  }
  if (json.front() != '{') {
    return false;
  }
  constexpr std::string_view expected_json = json;
  constexpr std::expected<std::meta::info, json_error> result =
      parse_json_object_impl(json);
  return result.has_value() && (expected_json == json);
}

} // namespace compile_time
} // namespace simdjson

#endif // SIMDJSON_STATIC_REFLECTION
#endif // SIMDJSON_GENERIC_COMPILE_TIME_JSON_H
