#ifndef SIMDJSON_GENERIC_FRACTURED_JSON_BUILDER_H
#define SIMDJSON_GENERIC_FRACTURED_JSON_BUILDER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/generic/builder/json_builder.h"
#include "simdjson/dom/fractured_json.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_STATIC_REFLECTION

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace builder {

/**
 * Serialize an object to a FracturedJson-formatted string.
 *
 * FracturedJson produces human-readable yet compact JSON output by intelligently
 * choosing between different layout strategies (inline, compact multiline, table,
 * expanded) based on content complexity, length, and structure similarity.
 *
 * This function combines the builder's serialization with FracturedJson formatting:
 * 1. Serializes the object to minified JSON using reflection
 * 2. Parses and reformats using FracturedJson
 *
 * Example:
 *   struct User { int id; std::string name; bool active; };
 *   User user{1, "Alice", true};
 *   auto result = to_fractured_json_string(user);
 *   // result.value() == "{ \"id\": 1, \"name\": \"Alice\", \"active\": true }"
 *
 * @param obj The object to serialize (must be a reflectable type)
 * @param opts FracturedJson formatting options
 * @param initial_capacity Initial buffer capacity for serialization
 * @return The formatted JSON string, or an error
 */
template <class T>
simdjson_warn_unused simdjson_result<std::string> to_fractured_json_string(
    const T& obj,
    const fractured_json_options& opts = {},
    size_t initial_capacity = string_builder::DEFAULT_INITIAL_CAPACITY) {
  // Step 1: Serialize to minified JSON
  std::string formatted;
  auto error = to_json_string(obj, initial_capacity).get(formatted);
  if (error) {
    return error;
  }

  // Step 2: Reformat with FracturedJson
  return fractured_json_string(formatted, opts);
}

/**
 * Extract specific fields from an object and format with FracturedJson.
 *
 * Example:
 *   struct User { int id; std::string name; std::string email; bool active; };
 *   User user{1, "Alice", "alice@example.com", true};
 *   auto result = extract_fractured_json<"id", "name">(user);
 *   // result.value() == "{ \"id\": 1, \"name\": \"Alice\" }"
 *
 * @param obj The object to serialize
 * @param opts FracturedJson formatting options
 * @param initial_capacity Initial buffer capacity for serialization
 * @return The formatted JSON string containing only the specified fields
 */
template<constevalutil::fixed_string... FieldNames, typename T>
  requires(std::is_class_v<T> && (sizeof...(FieldNames) > 0))
simdjson_warn_unused simdjson_result<std::string> extract_fractured_json(
    const T& obj,
    const fractured_json_options& opts = {},
    size_t initial_capacity = string_builder::DEFAULT_INITIAL_CAPACITY) {
  // Step 1: Extract fields to minified JSON
  std::string formatted;
  auto error = extract_from<FieldNames...>(obj, initial_capacity).get(formatted);
  if (error) {
    return error;
  }

  // Step 2: Reformat with FracturedJson
  return fractured_json_string(formatted, opts);
}

} // namespace builder
} // namespace SIMDJSON_IMPLEMENTATION

// Global namespace convenience functions

/**
 * Serialize an object to a FracturedJson-formatted string.
 * Global namespace version for convenience.
 */
template <class T>
simdjson_warn_unused simdjson_result<std::string> to_fractured_json_string(
    const T& obj,
    const fractured_json_options& opts = {},
    size_t initial_capacity = SIMDJSON_IMPLEMENTATION::builder::string_builder::DEFAULT_INITIAL_CAPACITY) {
  return SIMDJSON_IMPLEMENTATION::builder::to_fractured_json_string(obj, opts, initial_capacity);
}
/**
 * Extract specific fields from an object and format with FracturedJson.
 * Global namespace version for convenience.
 */
template<constevalutil::fixed_string... FieldNames, typename T>
  requires(std::is_class_v<T> && (sizeof...(FieldNames) > 0))
simdjson_warn_unused simdjson_result<std::string> extract_fractured_json(
    const T& obj,
    const fractured_json_options& opts = {},
    size_t initial_capacity = SIMDJSON_IMPLEMENTATION::builder::string_builder::DEFAULT_INITIAL_CAPACITY) {
  return SIMDJSON_IMPLEMENTATION::builder::extract_fractured_json<FieldNames...>(obj, opts, initial_capacity);
}

} // namespace simdjson

#endif // SIMDJSON_STATIC_REFLECTION

#endif // SIMDJSON_GENERIC_FRACTURED_JSON_BUILDER_H
