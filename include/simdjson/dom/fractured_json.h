#ifndef SIMDJSON_DOM_FRACTURED_JSON_H
#define SIMDJSON_DOM_FRACTURED_JSON_H

#include "simdjson/dom/base.h"
#include "simdjson/dom/element.h"

namespace simdjson {

/**
 * Configuration options for FracturedJson formatting.
 *
 * FracturedJson intelligently chooses between different layout strategies
 * (inline, compact multiline, table, expanded) based on content complexity,
 * length, and structure similarity.
 */
struct fractured_json_options {
  /**
   * Maximum total characters per line (default: 120).
   * Content exceeding this will be expanded to multiple lines.
   */
  size_t max_total_line_length = 120;

  /**
   * Maximum length for inlined elements (default: 80).
   * Simple arrays/objects shorter than this may be rendered inline.
   */
  size_t max_inline_length = 80;

  /**
   * Maximum nesting depth for inline rendering (default: 2).
   * Elements with complexity exceeding this will be expanded.
   * Complexity 0 = scalar, 1 = flat array/object, 2 = one level of nesting.
   */
  size_t max_inline_complexity = 2;

  /**
   * Maximum complexity for compact array formatting (default: 1).
   * Arrays with elements of this complexity or less may have multiple
   * items per line.
   */
  size_t max_compact_array_complexity = 1;

  /**
   * Number of spaces per indentation level (default: 4).
   */
  size_t indent_spaces = 4;

  /**
   * Enable tabular formatting for arrays of similar objects (default: true).
   * When enabled, arrays of objects with identical keys are formatted
   * as aligned tables.
   */
  bool enable_table_format = true;

  /**
   * Minimum number of rows to trigger table mode (default: 3).
   */
  size_t min_table_rows = 3;

  /**
   * Similarity threshold for table detection (default: 0.8).
   * Objects must share at least this fraction of keys to be formatted
   * as a table.
   */
  double table_similarity_threshold = 0.8;

  /**
   * Enable compact multiline arrays (default: true).
   * When enabled, arrays of simple elements may have multiple items
   * per line.
   */
  bool enable_compact_multiline = true;

  /**
   * Maximum array items per line in compact mode (default: 10).
   */
  size_t max_items_per_line = 10;

  /**
   * Add space inside brackets for simple containers (default: true).
   * When true: { "key": "value" }
   * When false: {"key": "value"}
   */
  bool simple_bracket_padding = true;

  /**
   * Add space after colons (default: true).
   * When true: "key": "value"
   * When false: "key":"value"
   */
  bool colon_padding = true;

  /**
   * Add space after commas in inline content (default: true).
   * When true: [1, 2, 3]
   * When false: [1,2,3]
   */
  bool comma_padding = true;
};

/**
 * Format JSON using FracturedJson formatting with default options.
 *
 * FracturedJson produces human-readable yet compact output by intelligently
 * choosing between inline, compact multiline, table, and expanded layouts.
 *
 *   dom::parser parser;
 *   element doc = parser.parse(json_string);
 *   cout << fractured_json(doc) << endl;
 */
template <class T>
std::string fractured_json(T x);

/**
 * Format JSON using FracturedJson formatting with custom options.
 *
 *   dom::parser parser;
 *   element doc = parser.parse(json_string);
 *   fractured_json_options opts;
 *   opts.max_total_line_length = 80;
 *   cout << fractured_json(doc, opts) << endl;
 */
template <class T>
std::string fractured_json(T x, const fractured_json_options& options);

#if SIMDJSON_EXCEPTIONS
template <class T>
std::string fractured_json(simdjson_result<T> x);

template <class T>
std::string fractured_json(simdjson_result<T> x, const fractured_json_options& options);
#endif

/**
 * Format a JSON string using FracturedJson formatting.
 *
 * This is useful for formatting output from the builder/static reflection API
 * or any valid JSON string.
 *
 *   // With static reflection
 *   MyStruct data = {...};
 *   auto minified = simdjson::to_json_string(data);
 *   auto formatted = simdjson::fractured_json_string(minified.value());
 *
 *   // Or with any JSON string
 *   std::string json = R"({"key":"value"})";
 *   auto formatted = simdjson::fractured_json_string(json);
 */
inline std::string fractured_json_string(std::string_view json_str);

/**
 * Format a JSON string using FracturedJson formatting with custom options.
 */
inline std::string fractured_json_string(std::string_view json_str,
                                          const fractured_json_options& options);

} // namespace simdjson

#endif // SIMDJSON_DOM_FRACTURED_JSON_H
