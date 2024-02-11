#pragma once

#ifndef SIMDJSON_ONDEMAND_GENERIC_JSON_PATH_TO_POINTER_CONVERSION_INL_H
#define SIMDJSON_ONDEMAND_GENERIC_JSON_PATH_TO_POINTER_CONVERSION_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_JSON_PATH_TO_POINTER_CONVERSION_INL_H
#include "simdjson/generic/ondemand/json_path_to_pointer_conversion.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {}
namespace ondemand {

simdjson_inline std::string json_path_to_pointer_conversion(std::string_view json_path) {
  if (json_path.empty() || (json_path.front() != '.' && json_path.front() != '[') {
    return "-1"; // Sentinel value to be handled as an error by the caller.
  }

  std::string result;
  // Reserve space to reduce allocations, adjusting for potential increases due
  // to escaping.
  result.reserve(json_path.size() * 2);

  // Skip the initial '.' as it's assumed every path starts with it.
  size_t i = 0;

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
        return "-1"; // Returning sentinel value that will be handled as an error by the caller
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

  return simdjson_result<std::string>(result);
}


} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_ONDEMAND_GENERIC_JSON_PATH_TO_POINTER_CONVERSION_INL_H
