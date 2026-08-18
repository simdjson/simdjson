/**
 * @file compile_time_json-inl.h
 * @brief Implementation details for compile-time JSON parsing
 *
 * This file contains inline implementations and helper utilities for
 * compile-time JSON parsing. Currently, the main implementation is
 * self-contained in the header.
 */

#ifndef SIMDJSON_GENERIC_COMPILE_TIME_JSON_INL_H

#if SIMDJSON_STATIC_REFLECTION

#include "simdjson/compile_time_json.h"
#include "simdjson/internal/fast_float.h"
#include <array>
#include <cstdint>
#include <meta>
#include <limits>
#include <string_view>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <expected>
#include <meta>
#include <string>
#include <string_view>
#include <vector>

#define simdjson_consteval_error(...)                                          \
  {                                                                            \
    std::abort();                                                              \
  }

namespace simdjson {
namespace compile_time {

/**
 * Namespace for number parsing utilities.
 * We seek to provide exact compile-time number parsing functions.
 * Correct rounding of floating-point numbers is not a trivial matter, and it is
 * harder still in a constant expression, where the runtime parser's memcpy and
 * __uint128_t tricks are unavailable. We hand that part to fast_float, which is
 * correctly rounded and usable in a constant expression from C++20 onwards.
 */
namespace number_parsing {

// Parses a JSON float starting at src. Returns the value and the number of
// characters consumed.
//
// Eisel-Lemire needs somewhere to fall back to: a mantissa of more than 19
// significant digits, or an input where the truncated product cannot decide the
// rounding, has to be finished by a slower exact method. A constant expression
// cannot call the runtime one in src/from_chars.cpp, so we use fast_float, which
// is correctly rounded in a constant expression and carries its own fallback.
consteval std::pair<double, size_t> parse_double(const char *src,
                                                 const char *end) {
  double value = 0;
  auto answer = simdjson_fast_float::from_chars_advanced(
      src, end, value,
      simdjson_fast_float::parse_options{
          simdjson_fast_float::chars_format::json});
  if (answer.ec == std::errc::invalid_argument) {
    simdjson_consteval_error("Invalid float value");
  }
  // fast_float reports result_out_of_range at either edge of the format. An
  // underflow to zero is a value like any other; an overflow to infinity is not
  // representable in JSON and was an error here before, so it stays one.
  // (Comparing against max/lowest rather than calling std::isinf, which is not
  // usable in a constant expression.)
  if (value > (std::numeric_limits<double>::max)() ||
      value < std::numeric_limits<double>::lowest()) {
    simdjson_consteval_error("Overflow while computing the float value");
  }
  return {value, size_t(answer.ptr - src)};
}

} // namespace number_parsing

consteval auto make_data_member_options(auto&& name_str) {
  std::meta::data_member_options options{};
  options.name = std::forward<decltype(name_str)>(name_str);
  return options;
}

// JSON string may contain embedded nulls, and C++26 reflection does not yet
// support std::string_view as a data member type. As a workaround, we define
// a custom type that holds a const char* and a size.
template <char... Vals> struct fixed_json_string {
  // Statically-allocated array to hold the characters and a null terminator
  static constexpr char inner_data[] = {Vals..., '\0'};

  // Constant for the length of the string view (excluding the null terminator)
  static constexpr std::size_t inner_size = sizeof...(Vals);

  // The std::string_view over the data.
  // We use data and size to avoid including the null terminator in the view.
  static constexpr std::string_view view = {inner_data, inner_size};

  constexpr operator std::string_view() { return view; }
  constexpr const char *c_str() { return view.data(); }
  constexpr const char *data() { return view.data(); }
  constexpr size_t size() { return view.size(); }
};

consteval std::meta::info to_fixed_json_string(std::string_view in) {
  std::vector<std::meta::info> Args = {};
  for (char c : in) {
    Args.push_back(std::meta::reflect_constant(c));
  }
  return std::meta::substitute(^^fixed_json_string, Args);
}

/**
 * @brief Helper struct for substitute() pattern
 */
template <std::meta::info... meta_info> struct type_builder {
  struct constructed_type;
  consteval {
    std::meta::define_aggregate(^^constructed_type, {
                                                        meta_info...});
  }
};

/**
 * @brief Type alias for the generated struct
 * Usage:
 * using struct = class_type<std::meta::data_member_spec(^^int;,  {.name =
 * "x"}),std::meta::data_member_spec(^^float, {.name = "y"})>;
 */
template <std::meta::info... meta_info>
using class_type = type_builder<meta_info...>::constructed_type;

/**
 */

/**
 * @brief Variable template for constructing instances with values
 */
template <typename T, auto... Vs> constexpr T construct_from = T{Vs...};


// in JSON, there are only a few whitespace characters that are allowed
// outside of objects, arrays, strings, and numbers.
[[nodiscard]] constexpr bool is_whitespace(char c) {
  return c == ' ' || c == '\n' || c == '\t' || c == '\r';
};

[[nodiscard]] constexpr std::string_view trim_whitespace(std::string_view str) {
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
consteval std::pair<std::meta::info, size_t>
parse_json_object_impl(std::string_view json);

///////////////////////////////////////////////////
/// NUMBER PARSING
///////////////////////////////////////////////////

// Parses a JSON number from a string view.
// Returns the number of characters consumed and the parsed value
// as a variant of int64_t, uint64_t, or double.
[[nodiscard]] consteval size_t
parse_number(std::string_view json,
             std::variant<int64_t, uint64_t, double> &out) {
  if (json.empty()) {
    simdjson_consteval_error("Empty string is not a valid JSON number");
  }
  if (json[0] == '+') {
    simdjson_consteval_error("Invalid number: leading '+' sign is not allowed");
  }
  // Compute the range and determine the type.
  auto it = json.begin();
  if (it != json.end() && (*it == '-')) {
    it++;
  }
  while (it != json.end() && (*it >= '0' && *it <= '9')) {
    it++;
  }
  bool is_float = false;
  if (it != json.end() && (*it == '.' || *it == 'e' || *it == 'E')) {
    is_float = true;
    if (*it == '.') {
      it++;
      while (it != json.end() && (*it >= '0' && *it <= '9')) {
        it++;
      }
    }
    if (it != json.end() && (*it == 'e' || *it == 'E')) {
      it++;
      if (it != json.end() && (*it == '+' || *it == '-')) {
        it++;
      }
      while (it != json.end() && (*it >= '0' && *it <= '9')) {
        it++;
      }
    }
  }
  size_t scope = it - json.begin();

  bool is_negative = json[0] == '-';
  // Note that we consider -0 to be an integer unless it has a decimal point or
  // exponent.
  if (is_float) {
    auto [value, offset] =
        number_parsing::parse_double(json.data(), json.data() + json.size());
    if (offset != scope) {
      simdjson_consteval_error(
          "Internal error: cannot agree on the character range of the float");
    }
    out = value;
    return offset;
  } else if (is_negative) {
    int64_t int_value = 0;
    std::from_chars_result res =
        std::from_chars(json.data(), json.data() + json.size(), int_value);
    if (res.ec == std::errc()) {
      out = int_value;
      if (static_cast<std::size_t>(res.ptr - json.data()) != scope) {
        simdjson_consteval_error(
            "Internal error: cannot agree on the character range of the float");
      }
      return (res.ptr - json.data());
    } else {
      simdjson_consteval_error("Invalid integer value");
    }
  } else {
    uint64_t uint_value = 0;
    std::from_chars_result res =
        std::from_chars(json.data(), json.data() + json.size(), uint_value);
    if (res.ec == std::errc()) {
      out = uint_value;
      if (static_cast<std::size_t>(res.ptr - json.data()) != scope) {
        simdjson_consteval_error(
            "Internal error: cannot agree on the character range of the float");
      }
      return (res.ptr - json.data());
    } else {
      simdjson_consteval_error("Invalid unsigned integer value");
    }
  }
}

////////////////////////////////////////////////////////
/// STRING PARSING
////////////////////////////////////////////////////////

// parse a JSON string value, handling escape sequences and validating UTF-8
// Returns the created string and the number of characters consumed.
// Note that the number of characters consumed includes the surrounding quotes.
// The number of bytes written to out differs from the number of characters
// consumed in general because of escape sequences and UTF-8 encoding.
[[nodiscard]] consteval std::pair<std::string, size_t>
parse_string(std::string_view json) {
  auto cursor = json.begin();
  auto end = json.end();
  std::string out;

  // Expect starting quote
  if (cursor == end || *(cursor++) != '"') {
    simdjson_consteval_error("Expected opening quote for string");
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
    auto hex_to_u32 = [&] [[nodiscard]] () -> uint32_t {
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
    auto codepoint_to_utf8 = [&out] [[nodiscard]] (uint32_t cp) -> bool {
      //  the high and low surrogates U+D800 through U+DFFF are invalid code
      //  points
      if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        return false;
      }
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
      } else {
        return false;
      }
      return true;
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
    uint32_t code_point = hex_to_u32();
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
          *(cursor + 1) != 'u') {
        code_point = substitution_code_point;
      } else {       // we have \u following the high surrogate
        cursor += 2; // skip \u
        uint32_t code_point_2 = hex_to_u32();
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
    } else if (code_point >= 0xdc00 && code_point < 0xe000) {
      // Isolated low surrogate, invalid
      code_point = substitution_code_point;
    }
    // Now we have the final code point, write it as UTF-8 to out.
    return codepoint_to_utf8(code_point);
  };

  while (cursor != end && *cursor != '"') {
    // If we find the end of input before closing quote, it's an error
    if (cursor == end) {
      simdjson_consteval_error("Unterminated string");
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
          simdjson_consteval_error("Truncated escape sequence in string");
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
            simdjson_consteval_error(
                "Invalid unicode escape sequence in string");
          }
          break;
        default:
          simdjson_consteval_error("Invalid escape character in string");
        }
      }
      continue; // continue to next iteration
    }
    // Handle escape sequences and UTF-8 validation. We do not process
    // the escape sequences here, just validate them.
    out += c;
    if (static_cast<unsigned char>(c) < 0x20) {
      simdjson_consteval_error("Invalid control character in string");
    }
    if (static_cast<unsigned char>(c) >= 0x80) {
      // We have a non-ASCII character inside a string
      // We must validate it.
      uint8_t first_byte = static_cast<uint8_t>(c);

      if ((first_byte & 0b11100000) == 0b11000000) {
        if (cursor == end) {
          simdjson_consteval_error("Truncated UTF-8 sequence in string");
        }

        char second_byte = *cursor;
        out += second_byte;
        ++cursor;
        if ((static_cast<uint8_t>(second_byte) & 0b11000000) != 0b10000000) {
          simdjson_consteval_error("Invalid UTF-8 continuation byte in string");
        }
        // range check
        uint32_t code_point = (first_byte & 0b00011111) << 6 |
                              (static_cast<uint8_t>(second_byte) & 0b00111111);
        if ((code_point < 0x80) || (0x7ff < code_point)) {
          simdjson_consteval_error("Invalid UTF-8 code point in string");
        }
      } else if ((first_byte & 0b11110000) == 0b11100000) {
        if (cursor == end) {
          simdjson_consteval_error("Truncated UTF-8 sequence in string");
        }
        char second_byte = *cursor;
        ++cursor;
        out += second_byte;
        if ((static_cast<uint8_t>(second_byte) & 0b11000000) != 0b10000000) {
          simdjson_consteval_error("Invalid UTF-8 continuation byte in string");
        }
        if (cursor == end) {
          simdjson_consteval_error("Truncated UTF-8 sequence in string");
        }
        char third_byte = *cursor;
        ++cursor;
        out += third_byte;
        if ((static_cast<uint8_t>(third_byte) & 0b11000000) != 0b10000000) {
          simdjson_consteval_error("Invalid UTF-8 continuation byte in string");
        }
        // range check
        uint32_t code_point = (first_byte & 0b00001111) << 12 |
                              (static_cast<uint8_t>(second_byte) & 0b00111111)
                                  << 6 |
                              (static_cast<uint8_t>(third_byte) & 0b00111111);
        if ((code_point < 0x800) || (0xffff < code_point) ||
            (0xd7ff < code_point && code_point < 0xe000)) {
          simdjson_consteval_error("Invalid UTF-8 code point in string");
        }
      } else if ((first_byte & 0b11111000) == 0b11110000) { // 0b11110000
        if (cursor == end) {
          simdjson_consteval_error("Truncated UTF-8 sequence in string");
        }
        char second_byte = *cursor;
        ++cursor;
        out += second_byte;
        if (cursor == end) {
          simdjson_consteval_error("Truncated UTF-8 sequence in string");
        }
        char third_byte = *cursor;
        ++cursor;
        out += third_byte;
        if (cursor == end) {
          simdjson_consteval_error("Truncated UTF-8 sequence in string");
        }
        char fourth_byte = *cursor;
        ++cursor;
        out += fourth_byte;
        if ((static_cast<uint8_t>(second_byte) & 0b11000000) != 0b10000000) {
          simdjson_consteval_error("Invalid UTF-8 continuation byte in string");
        }
        if ((static_cast<uint8_t>(third_byte) & 0b11000000) != 0b10000000) {
          simdjson_consteval_error("Invalid UTF-8 continuation byte in string");
        }
        if ((static_cast<uint8_t>(fourth_byte) & 0b11000000) != 0b10000000) {
          simdjson_consteval_error("Invalid UTF-8 continuation byte in string");
        }
        // range check
        uint32_t code_point =
            (first_byte & 0b00000111) << 18 |
            (static_cast<uint8_t>(second_byte) & 0b00111111) << 12 |
            (static_cast<uint8_t>(third_byte) & 0b00111111) << 6 |
            (static_cast<uint8_t>(fourth_byte) & 0b00111111);
        if (code_point <= 0xffff || 0x10ffff < code_point) {
          simdjson_consteval_error("Invalid UTF-8 code point in string");
        }
      } else {
        // we have a continuation
        simdjson_consteval_error("Invalid UTF-8 continuation byte in string");
      }
      continue;
    }
  }
  if (cursor == end) {
    simdjson_consteval_error("Unterminated string");
  }
  if (*cursor != '"') {
    simdjson_consteval_error("Internal error: expected closing quote");
  }
  cursor++; // consume the closing quote
  // We get here if and only if we have seen the closing quote.

  return {out, size_t(cursor - json.begin())};
}

////////////////////////////////////////////////////
/// ARRAY PARSING
////////////////////////////////////////////////////

// Parses a JSON array and returns a std::meta::info representing the array as
// well as the number of characters consumed.
consteval std::pair<std::meta::info, size_t>
parse_json_array_impl(const std::string_view json) {
  size_t consumed = 0;
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

  if (!expect_consume('[')) {
    simdjson_consteval_error("Expected '['");
  }

  std::vector<std::meta::info> values = {^^void};
  skip_whitespace();
  if (cursor != end && *cursor == ']') {
    if (!expect_consume(']')) {
      simdjson_consteval_error("Expected ']'");
    }
    // Empty array - use int as placeholder type since void doesn't work
    auto array_type = std::meta::substitute(
        ^^std::array, {
                          ^^int, std::meta::reflect_constant(0uz)});
    values[0] = array_type;
    consumed = size_t(cursor - json.begin());
    if (json[consumed - 1] != ']') {
      simdjson_consteval_error("Expected ']'");
    }
    return {std::meta::substitute(^^construct_from, values), consumed};
  }
  while (cursor != end && *cursor != ']') {
    char c = *cursor;
    switch (c) {
    case '{': {
      std::string_view value(cursor, end);
      auto [parsed, object_size] = parse_json_object_impl(value);
      if (*(cursor + object_size - 1) != '}') {
        simdjson_consteval_error("Expected '}'");
      }
      values.push_back(parsed);
      cursor += object_size;
      break;
    }
    case '[': {
      std::string_view value(cursor, end);
      auto [parsed, array_size] = parse_json_array_impl(value);
      if (*(cursor + array_size - 1) != ']') {
        simdjson_consteval_error("Expected ']'");
      }
      values.push_back(parsed);
      cursor += array_size;
      break;
    }
    case '"': {
      auto res = parse_string(std::string_view(cursor, end));
      cursor += res.second;
      for (char ch : res.first) {
        if (ch == '\0') {
          simdjson_consteval_error(
              "Field string values cannot contain embedded nulls");
        }
      }
      values.push_back(std::meta::reflect_constant_string(res.first));
      break;
    }
    case 't': {
      if (end - cursor < 4 || std::string_view(cursor, 4) != "true") {
        simdjson_consteval_error("Invalid value");
      }
      cursor += 4;
      values.push_back(std::meta::reflect_constant(true));
      break;
    }
    case 'f': {
      if (end - cursor < 5 || std::string_view(cursor, 5) != "false") {
        simdjson_consteval_error("Invalid value");
      }
      cursor += 5;
      values.push_back(std::meta::reflect_constant(false));
      break;
    }
    case 'n': {
      if (end - cursor < 4 || std::string_view(cursor, 4) != "null") {
        simdjson_consteval_error("Invalid value");
      }
      cursor += 4;
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
      size_t r = parse_number(suffix, out);
      cursor += r;
      if (std::holds_alternative<int64_t>(out)) {
        int64_t int_value = std::get<int64_t>(out);
        values.push_back(std::meta::reflect_constant(int_value));
      } else if (std::holds_alternative<uint64_t>(out)) {
        uint64_t uint_value = std::get<uint64_t>(out);
        values.push_back(std::meta::reflect_constant(uint_value));
      } else {
        double float_value = std::get<double>(out);
        values.push_back(std::meta::reflect_constant(float_value));
      }
      break;
    }
    default:
      simdjson_consteval_error("Invalid character starting value");
    }
    skip_whitespace();
    if (cursor != end && *cursor == ',') {
      ++cursor;
      skip_whitespace();
    }
  }

  if (!expect_consume(']')) {
    simdjson_consteval_error("Expected ']'");
  }
  std::size_t count = values.size() - 1;
  // We assume all elements have the same type as the first element.
  // However, if the array is heterogeneous, we should use std::variant.
  auto elem_type = std::meta::type_of(values[1]);
  // String literals reflected via reflect_constant_string have type const
  // char[N], but when passed as template auto parameters they decay to
  // const char*.  Use const char* as the element type so that
  // construct_from can aggregate-initialize the array.
  if (std::meta::is_array_type(elem_type) &&
      std::meta::remove_all_extents(elem_type) == ^^const char) {
    elem_type = ^^const char *;
  }
  auto array_type = std::meta::substitute(
      ^^std::array,
      {
          elem_type, std::meta::reflect_constant(count)});

  // Create array instance with values
  values[0] = array_type;
  consumed = size_t(cursor - json.begin());
  if (json[consumed - 1] != ']') {
    simdjson_consteval_error("Expected ']'");
  }
  return {std::meta::substitute(^^construct_from, values), consumed};
}

////////////////////////////////////////////////////
/// OBJECT PARSING
////////////////////////////////////////////////////

// Parses a JSON object and returns a std::meta::info representing the object
// type as well as the number of characters consumed.
consteval std::pair<std::meta::info, size_t>
parse_json_object_impl(std::string_view json) {
  size_t consumed = 0;
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

  if (!expect_consume('{')) {
    simdjson_consteval_error("Expected '{'");
  }

  std::vector<std::meta::info> members;
  std::vector<std::meta::info> values = {^^void};

  while (cursor != end && *cursor != '}') {
    skip_whitespace();
    // Not all strings can be identifiers, but in JSON field names can be any
    // string. Thus we may have a problem if the field name contains characters
    // not allowed in identifiers. Let the standard library handle that case.
    auto field = parse_string(std::string_view(cursor, end));
    std::string field_name = field.first;
    cursor += field.second;
    if (!expect_consume(':')) {
      simdjson_consteval_error("Expected ':'");
    }
    skip_whitespace();
    if (cursor == end) {
      simdjson_consteval_error("Expected value after colon");
    }
    char c = *cursor;
    switch (c) {
    case '{': {
      std::string_view value(cursor, end);
      auto [parsed, object_size] = parse_json_object_impl(value);
      if (*(cursor + object_size - 1) != '}') {
        simdjson_consteval_error("Expected '}'");
      }
      cursor += object_size;
      auto dms = std::meta::data_member_spec(std::meta::type_of(parsed), make_data_member_options(field_name));
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(parsed);

      break;
    }
    case '[': {
      std::string_view value(cursor, end);
      auto [parsed, array_size] = parse_json_array_impl(value);
      auto dms = std::meta::data_member_spec(std::meta::type_of(parsed), make_data_member_options(field_name));
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(parsed);
      if (*(cursor + array_size - 1) != ']') {
        simdjson_consteval_error("Expected ']'");
      }
      cursor += array_size;

      break;
    }
    case '"': {
      auto res = parse_string(std::string_view(cursor, end));
      std::string_view value = res.first;
      cursor += res.second;
      for (char ch : value) {
        if (ch == '\0') {
          simdjson_consteval_error(
              "Field string values cannot contain embedded nulls");
        }
      }
      auto dms =
          std::meta::data_member_spec(^^const char *, make_data_member_options(field_name));
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(std::meta::reflect_constant_string(value));
      break;
    }
    case 't': {
      if (end - cursor < 4 || std::string_view(cursor, 4) != "true") {
        simdjson_consteval_error("Invalid value");
      }
      cursor += 4;

      auto dms = std::meta::data_member_spec(^^bool, make_data_member_options(field_name));
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(std::meta::reflect_constant(true));
      break;
    }
    case 'f': {
      if (end - cursor < 5 || std::string_view(cursor, 5) != "false") {
        simdjson_consteval_error("Invalid value");
      }
      cursor += 5;

      auto dms = std::meta::data_member_spec(^^bool, make_data_member_options(field_name));
      members.push_back(std::meta::reflect_constant(dms));
      values.push_back(std::meta::reflect_constant(false));
      break;
    }
    case 'n': {
      if (end - cursor < 4 || std::string_view(cursor, 4) != "null") {
        simdjson_consteval_error("Invalid value");
      }
      cursor += 4;

      auto dms = std::meta::data_member_spec(^^std::nullptr_t, make_data_member_options(field_name));
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
      size_t r = parse_number(suffix, out);
      cursor += r;
      if (std::holds_alternative<int64_t>(out)) {
        int64_t int_value = std::get<int64_t>(out);
        auto dms =
            std::meta::data_member_spec(^^int64_t, make_data_member_options(field_name));
        members.push_back(std::meta::reflect_constant(dms));
        values.push_back(std::meta::reflect_constant(int_value));
      } else if (std::holds_alternative<uint64_t>(out)) {
        uint64_t uint_value = std::get<uint64_t>(out);
        auto dms =
            std::meta::data_member_spec(^^uint64_t, make_data_member_options(field_name));
        members.push_back(std::meta::reflect_constant(dms));
        values.push_back(std::meta::reflect_constant(uint_value));
      } else {
        double float_value = std::get<double>(out);
        auto dms =
            std::meta::data_member_spec(^^double, make_data_member_options(field_name));
        members.push_back(std::meta::reflect_constant(dms));
        values.push_back(std::meta::reflect_constant(float_value));
      }
      break;
    }
    default:
      simdjson_consteval_error("Invalid character starting value");
    }
    skip_whitespace();
    if (cursor == end) {
      simdjson_consteval_error("Expected '}' or ','");
    }
    if (*cursor == ',') {
      ++cursor;
      skip_whitespace();
    } else if (*cursor != '}') {
      simdjson_consteval_error("Expected '}'");
    }
  }

  if (!expect_consume('}')) {
    simdjson_consteval_error("Expected '}'");
  }
  values[0] = std::meta::substitute(^^class_type, members);
  consumed = size_t(cursor - json.begin());
  if (json[consumed - 1] != '}') {
    simdjson_consteval_error("Expected '}'");
  }
  return {std::meta::substitute(^^construct_from, values), consumed};
}

/**
 * Our public function to parse JSON at compile time.
 * @brief Compile-time JSON parser. This function parses the provided JSON
 * string at compile time and returns a custom struct type representing the JSON
 * object.
 */
template <constevalutil::fixed_string json_str> consteval auto parse_json() {
  constexpr std::string_view json = trim_whitespace(json_str.view());
  static_assert(!json.empty(), "JSON string cannot be empty");
  /*static_assert(json.front() != '{' && json.front() != '[',
                "Only JSON objects and arrays are supported at the top level, this "
                "limitation will be lifted in the future.");*/

  if constexpr (json.front() == '[') {
    return [: parse_json_array_impl(json).first :];
  } else {
    return [: parse_json_object_impl(json).first :];
  }
}

} // namespace compile_time
} // namespace simdjson

#endif // SIMDJSON_STATIC_REFLECTION
#endif // SIMDJSON_GENERIC_COMPILE_TIME_JSON_INL_H
