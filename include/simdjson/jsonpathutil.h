#ifndef SIMDJSON_JSONPATHUTIL_H
#define SIMDJSON_JSONPATHUTIL_H

#include "simdjson/error.h"
#include <string>
#include "simdjson/common_defs.h"

#include <limits>
#include <utility>
#include <vector>

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

/**
 * Returns true if the error means that a JSONPath segment matched nothing
 * (missing field, index out of bounds, value of the wrong type, or a scalar
 * reached while path segments remain), as opposed to a document error.
 * A wildcard walk skips such branches, as RFC 9535 and DOM's
 * at_path_with_wildcard do.
 */
simdjson_inline bool is_matching_failure_error(error_code error) noexcept {
  return error == NO_SUCH_FIELD || error == INDEX_OUT_OF_BOUNDS ||
         error == INCORRECT_TYPE || error == INVALID_JSON_POINTER;
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
  } else if (i < json_path.size() && json_path[i] == '[') { // i.e. [0] or [123]
    size_t j = i + 1;
    while (j < json_path.size() && json_path[j] >= '0' && json_path[j] <= '9') {
      ++j;
    }
    if (j > i + 1 && j < json_path.size() && json_path[j] == ']') {
      key = json_path.substr(i + 1, j - i - 1);
      i = j + 1;
    }
  }


  return std::make_pair(key, json_path.substr(i));
}

namespace internal {
/**
 * One level of a JSONPath as the On-Demand for_each_at_path_with_wildcard()
 * consumes it.
 */
struct json_path_segment {
  // The key or array index at this level, "*" for a wildcard. Empty if the
  // path could not be parsed at this level.
  std::string_view key{};
  // Whether a '*' occurs in the rest of the path, after this segment. A
  // segment that matches nothing before a wildcard yields an empty result
  // instead of an error, as in DOM's at_path_with_wildcard. Like DOM, this
  // treats a '*' inside a quoted key as a wildcard.
  bool wildcard_follows{false};
};

/**
 * Splits json_path with get_next_key_and_json_path(), one segment per level,
 * as the On-Demand for_each_at_path_with_wildcard() historically did at each
 * node. The keys point into json_path, which must outlive the result.
 *
 * Every node visited at a given depth sees the same remaining path, so we
 * split it once up front: re-scanning the remaining path at every node costs
 * O(nodes * path length), which made a long path after a wildcard take
 * minutes on a large document.
 *
 * The last segment either has an empty key (the path is invalid there) or
 * is the end of the path.
 */
inline std::vector<json_path_segment> split_json_path(std::string_view json_path) {
  std::vector<json_path_segment> segments;
  // The remaining path (always a suffix of json_path) contains a '*' if and
  // only if it starts at or before the last one.
  const size_t last_star = json_path.rfind('*');
  std::string_view path = json_path;
  do {
    std::pair<std::string_view, std::string_view> key_and_json_path = get_next_key_and_json_path(path);
    path = key_and_json_path.second;
    const size_t offset = json_path.size() - path.size();
    json_path_segment segment;
    segment.key = key_and_json_path.first;
    segment.wildcard_follows = last_star != std::string_view::npos && last_star >= offset;
    segments.push_back(segment);
    if (segment.key.empty()) { break; }
  } while (!path.empty());
  return segments;
}

/**
 * A JSONPath as the DOM at_path_with_wildcard() consumes it, parsed once.
 *
 * Every node visited at a given depth sees the same remaining path, so we
 * parse it up front rather than at each node: re-scanning (and converting)
 * the remaining path at every node costs O(nodes * path length), which
 * made a long path after a couple of wildcards take minutes on a large
 * document.
 */
struct wildcard_json_path {
  enum class tail_type { invalid, all_values, pointer };
  // One JSON pointer ("/key") per leading segment, "/*" being a wildcard.
  std::vector<std::string> steps{};
  // What to do once the steps are exhausted.
  tail_type tail{tail_type::invalid};
  // The rest of the path, as a JSON pointer, when tail == pointer.
  std::string tail_pointer{};
};

/**
 * Splits json_path the same way the recursive DOM at_path_with_wildcard()
 * historically did, one level at a time, but in a single pass.
 */
inline wildcard_json_path parse_wildcard_json_path(std::string_view json_path) {
  wildcard_json_path result;
  // The remaining path (always a suffix of json_path) contains a '*' if and
  // only if it starts at or before the last one.
  const size_t last_star = json_path.rfind('*');
  std::string_view path = json_path;
  while (true) {
    size_t i = 0;
    if (!path.empty() && path.front() == '$') {
      i = 1;
    }
    if (i >= path.size() || (path[i] != '.' && path[i] != '[')) {
      result.tail = wildcard_json_path::tail_type::invalid;
      return result;
    }
    const size_t offset = json_path.size() - path.size();
    if (last_star == std::string_view::npos || last_star < offset) {
      result.tail_pointer = json_path_to_pointer_conversion(path);
      result.tail = (result.tail_pointer == "-1")
                        ? wildcard_json_path::tail_type::invalid
                        : wildcard_json_path::tail_type::pointer;
      return result;
    }
    if ((path.compare(i, 3, "[*]") == 0 && path.size() == i + 3) ||
        (path.compare(i, 2, ".*") == 0 && path.size() == i + 2)) {
      result.tail = wildcard_json_path::tail_type::all_values;
      return result;
    }
    std::pair<std::string_view, std::string_view> key_and_json_path = get_next_key_and_json_path(path);
    if (key_and_json_path.first.empty()) {
      result.tail = wildcard_json_path::tail_type::invalid;
      return result;
    }
    std::string step;
    step.reserve(key_and_json_path.first.size() + 1);
    step += '/';
    // The step is consumed by at_pointer, which unescapes ~0/~1, so '~' and
    // '/' in the key must be escaped as json_path_to_pointer_conversion does:
    // a raw '/' splits the step into two pointer tokens and a raw '~'
    // invalidates it, either way addressing the wrong element.
    for (const char c : key_and_json_path.first) {
      if (c == '~') {
        step += "~0";
      } else if (c == '/') {
        step += "~1";
      } else {
        step += c;
      }
    }
    result.steps.push_back(std::move(step));
    path = key_and_json_path.second;
    // The segment above consumed the path. An empty remainder is the node
    // that segment selected (at_pointer("")), not a parse error. The check
    // at the top of the loop is for a non-empty remainder that does not
    // continue with '.' or '['. A '*' inside the final segment ($.a.b*,
    // $['b*']) belongs to that segment.
    if (path.empty()) {
      result.tail = wildcard_json_path::tail_type::pointer;
      return result;
    }
  }
}
} // namespace internal

} // namespace simdjson
#endif // SIMDJSON_JSONPATHUTIL_H
