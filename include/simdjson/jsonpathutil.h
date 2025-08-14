#ifndef SIMDJSON_JSONPATHUTIL_H
#define SIMDJSON_JSONPATHUTIL_H

#include <string>
#include "simdjson/common_defs.h"

#include <utility>

namespace simdjson {
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
  } else if ((i+1 < json_path.size()) && json_path[i] == '[' && (json_path[i+1] == '\'' || json_path[i+1] == '"')) {
    i += 2;
    size_t key_start = i;
    while (i < json_path.length() && json_path[i] != '\'' && json_path[i] != '"') {
      ++i;
    }

    key = json_path.substr(key_start, i - key_start);

    i += 2;
  } else if ((i+2 < json_path.size()) && json_path[i] == '[' && json_path[i+1] == '*' && json_path[i+2] == ']') { // i.e [*].additional_keys or [*]["additional_keys"]
    key = "*";
    i += 3;
  }


  return std::make_pair(key, json_path.substr(i));
}

} // namespace simdjson
#endif // SIMDJSON_JSONPATHUTIL_H
