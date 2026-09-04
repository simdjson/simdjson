#ifndef SIMDJSON_JSONPATHUTIL_H
#define SIMDJSON_JSONPATHUTIL_H

#include "simdjson/base.h"
#include "simdjson/error.h"
#include <string>
#include "simdjson/common_defs.h"

#include <limits>
#include <utility>

namespace simdjson {
namespace internal {

/** Maximum number of recursive path segments in the single-pass On-Demand API. */
constexpr size_t MAX_RECURSIVE_PATH_DEPTH = 64;

/** Validate one RFC 6901 reference token (without its leading slash). */
simdjson_inline bool json_pointer_token_is_well_formed(std::string_view token) noexcept {
  for (size_t i = 0; i < token.size(); i++) {
    if (token[i] == '~') {
      if (++i >= token.size() || (token[i] != '0' && token[i] != '1')) {
        return false;
      }
    }
  }
  return true;
}

/** Validate a non-empty RFC 6901 JSON Pointer. */
simdjson_inline bool json_pointer_is_well_formed(std::string_view pointer) noexcept {
  if (pointer.empty() || pointer.front() != '/') { return false; }
  size_t token_start = 1;
  while (token_start <= pointer.size()) {
    const size_t slash = pointer.find('/', token_start);
    const std::string_view token = pointer.substr(
      token_start, slash == std::string_view::npos ? slash : slash - token_start);
    if (!json_pointer_token_is_well_formed(token)) { return false; }
    if (slash == std::string_view::npos) { break; }
    token_start = slash + 1;
  }
  return true;
}

/** Compare an encoded RFC 6901 reference token with an unescaped object key. */
simdjson_inline bool json_pointer_token_equals(std::string_view token,
                                               std::string_view key) noexcept {
  size_t key_index = 0;
  for (size_t i = 0; i < token.size(); i++) {
    char c = token[i];
    if (c == '~') {
      // Callers validate the token before comparing it.
      c = token[++i] == '0' ? '~' : '/';
    }
    if (key_index >= key.size() || key[key_index++] != c) {
      return false;
    }
  }
  return key_index == key.size();
}

/**
 * Parses the next JSON Pointer array index token.
 *
 * The caller passes a pointer fragment with no leading '/', such as "123/foo".
 * On success, array_index receives the parsed index and token_length receives
 * the number of bytes consumed before the next '/' or the end of the fragment.
 */
simdjson_inline error_code parse_json_pointer_array_index(std::string_view json_pointer,
                                                          size_t &array_index,
                                                          size_t &token_length) noexcept {
  array_index = 0;
  token_length = 0;

  for (; token_length < json_pointer.length() && json_pointer[token_length] != '/';
       token_length++) {
    uint8_t digit = uint8_t(json_pointer[token_length] - '0');
    // Check for non-digit in array index. If it's there, we're trying to get a field in an object.
    if (digit > 9) {
      return INCORRECT_TYPE;
    }
    // 0 followed by other digits is invalid.
    if (token_length > 0 && json_pointer[0] == '0') {
      return INVALID_JSON_POINTER;
    }
    if (array_index >
        (((std::numeric_limits<size_t>::max)() - digit) / 10)) {
      return INDEX_OUT_OF_BOUNDS;
    }
    array_index = array_index * 10 + digit;
  }

  // Empty string is invalid; so is a "/" with no digits before it.
  if (token_length == 0) {
    return INVALID_JSON_POINTER;
  }

  return SUCCESS;
}
} // namespace internal

/**
 * Converts JSONPath to JSON Pointer.
 * @param json_path The JSONPath string to be converted.
 * @return A string containing the equivalent JSON Pointer.
 */
inline std::string json_path_to_pointer_conversion(std::string_view json_path) {
  size_t i = 0;
  // if JSONPath starts with $, skip it
   // json_path.starts_with('$') requires C++20.
  if (!json_path.empty() && json_path.front() == '$') {
    i = 1;
  }
  if (i >= json_path.size() || (json_path[i] != '.' &&
      json_path[i] != '[')) {
    return "-1"; // This is just a sentinel value, the caller should check for this and return an error.
  }

  std::string result;
  // Reserve space to reduce allocations, adjusting for potential increases due
  // to escaping.
  result.reserve(json_path.size() * 2);

  while (i < json_path.length()) {
    if (json_path[i] == '.') {
      result += '/';
    } else if (json_path[i] == '[') {
      result += '/';
      ++i; // Move past the '['
      while (i < json_path.length() && json_path[i] != ']') {
          if (json_path[i] == '~') {
            result += "~0";
          } else if (json_path[i] == '/') {
            result += "~1";
          } else {
            result += json_path[i];
          }
          ++i;
      }
      if (i == json_path.length() || json_path[i] != ']') {
          return "-1"; // Using sentinel value that will be handled as an error by the caller.
      }
    } else {
      if (json_path[i] == '~') {
          result += "~0";
      } else if (json_path[i] == '/') {
          result += "~1";
      } else {
          result += json_path[i];
      }
    }
    ++i;
  }

  return result;
}

inline std::pair<std::string_view, std::string_view> get_next_key_and_json_path(std::string_view& json_path) {
  std::string_view key;

  if (json_path.empty()) {
    return {key, json_path};
  }
  size_t i = 0;

  // if JSONPath starts with $, skip it
  if (json_path.front() == '$') {
    i = 1;
  }


  if (i < json_path.length() && json_path[i] == '.') {
    i += 1;
    size_t key_start = i;

    while (i < json_path.length() && json_path[i] != '[' && json_path[i] != '.') {
      ++i;
    }

    key = json_path.substr(key_start, i - key_start);
  } else if ((i + 1 < json_path.size()) && json_path[i] == '[' &&
             (json_path[i + 1] == '\'' || json_path[i + 1] == '"')) {
    // Bracket-quoted key: ['key'] or ["key"].
    // Require a matching closing quote and a following ']'. If either is
    // missing, return an empty key and the original path so callers can treat
    // this as a parse failure (e.g. INVALID_JSON_POINTER) without advancing.
    // Without this check, i += 2 can make i > size() and substr throws
    // std::out_of_range, which aborts noexcept callers such as
    // at_path_with_wildcard / for_each_at_path_with_wildcard.
    const char quote = json_path[i + 1];
    i += 2;
    const size_t key_start = i;
    while (i < json_path.length() && json_path[i] != quote) {
      ++i;
    }
    if (i >= json_path.length() ||               // missing closing quote
        i + 1 >= json_path.length() ||           // missing ]
        json_path[i + 1] != ']') {
      return {key, json_path};
    }
    key = json_path.substr(key_start, i - key_start);
    i += 2; // past quote and ]
  } else if ((i+2 < json_path.size()) && json_path[i] == '[' && json_path[i+1] == '*' && json_path[i+2] == ']') { // i.e [*].additional_keys or [*]["additional_keys"]
    key = "*";
    i += 3;
  }


  return std::make_pair(key, json_path.substr(i));
}

namespace internal {

// On-Demand JSON Pointer/Path lookup is recursive because it must preserve its
// single-pass traversal semantics. Parsing may deliberately be configured much
// deeper, but a query must not turn that into unbounded call-stack consumption.
simdjson_inline bool json_pointer_depth_exceeded(std::string_view pointer) noexcept {
  size_t depth = 0;
  for (char c : pointer) {
    if (c == '/' && ++depth > MAX_RECURSIVE_PATH_DEPTH) { return true; }
  }
  return false;
}

simdjson_inline bool json_path_depth_exceeded(std::string_view path) noexcept {
  size_t depth = 0;
  while (!path.empty()) {
    const size_t previous_size = path.size();
    auto segment = get_next_key_and_json_path(path);
    if (segment.first.empty() || segment.second.size() >= previous_size) {
      return false; // malformed paths are rejected by the caller
    }
    if (++depth > MAX_RECURSIVE_PATH_DEPTH) { return true; }
    path = segment.second;
  }
  return false;
}

} // namespace internal

} // namespace simdjson
#endif // SIMDJSON_JSONPATHUTIL_H
