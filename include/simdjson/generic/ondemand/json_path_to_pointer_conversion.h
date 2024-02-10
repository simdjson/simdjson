#pragma once

#ifndef SIMDJSON_ONDEMAND_GENERIC_JSON_PATH_TO_POINTER_CONVERSION_H
#define SIMDJSON_ONDEMAND_GENERIC_JSON_PATH_TO_POINTER_CONVERSION_H

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace internal {

/**
 * Converts JSONPath to JSON Pointer.
 * @param json_path The JSONPath string to be converted.
 * @return A string containing the equivalent JSON Pointer.
 * @throws simdjson_error If the conversion fails.
 */
simdjson_inline simdjson_result<std::string> json_path_to_pointer_conversion(std::string_view json_path);

} // namespace internal
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_JSON_PATH_TO_POINTER_CONVERSION_H