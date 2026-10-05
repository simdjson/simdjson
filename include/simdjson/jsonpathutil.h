#ifndef SIMDJSON_JSONPATHUTIL_H
#define SIMDJSON_JSONPATHUTIL_H

#include "simdjson/error.h"
#include <string>
#include "simdjson/common_defs.h"

#include <limits>
#include <utility>

namespace simdjson {
namespace internal {
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
  } else if ((i + 1 < json_path.size()) && json_path[i] == '[' &&
             json_path[i + 1] >= '0' && json_path[i + 1] <= '9') {
    // Array index: [0], [12]. Without a closing ']', return an empty key and
    // the original path, as for a malformed bracket-quoted key.
    const size_t key_start = i + 1;
    size_t key_end = key_start;
    while (key_end < json_path.length() && json_path[key_end] >= '0' && json_path[key_end] <= '9') {
      ++key_end;
    }
    if (key_end >= json_path.length() || json_path[key_end] != ']') {
      return {key, json_path};
    }
    key = json_path.substr(key_start, key_end - key_start);
    i = key_end + 1; // past ]
  }


  return std::make_pair(key, json_path.substr(i));
}

namespace internal {
/**
 * Returns true if get_next_key_and_json_path can read every segment of the
 * JSONPath.
 */
inline bool json_path_is_well_formed(std::string_view json_path) noexcept {
  while (!json_path.empty()) {
    auto result_pair = get_next_key_and_json_path(json_path);
    if (result_pair.first.empty()) { return false; }
    json_path = result_pair.second;
  }
  return true;
}

/**
 * Returns true if the error means that an element reached through a wildcard
 * does not have the rest of the path: a missing key, an index out of range, or
 * a scalar where the path goes on. Such an element is skipped, as in the DOM
 * at_path_with_wildcard. A malformed path is not skipped, since it would fail
 * the same way for every element.
 */
inline bool is_wildcard_mismatch(error_code error, std::string_view remaining_path) noexcept {
  return error == NO_SUCH_FIELD || error == INDEX_OUT_OF_BOUNDS ||
         (error == INVALID_JSON_POINTER && json_path_is_well_formed(remaining_path));
}
} // namespace internal

} // namespace simdjson
#endif // SIMDJSON_JSONPATHUTIL_H
