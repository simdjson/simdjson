#ifndef SIMDJSON_DOM_FRACTURED_JSON_H
#define SIMDJSON_DOM_FRACTURED_JSON_H

#include "simdjson/dom/base.h"
#include "simdjson/dom/element.h"

namespace simdjson {

/** Specifies where commas should be in table-formatted elements. */
enum class table_comma_placement {
  /** Commas come right after the value */
  before_padding,
  /** Commas come after the column padding, so they line up in their own column. */
  after_padding,
  /** Commas come right after the value, except for columns of numbers */
  before_padding_except_numbers,
};

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
   * Forces elements close to the root to always fully expand, regardless of other settings.
   * (default: -1). -1 = none; 0 = root node only; 1 = root node and its children; etc.
   */
  int always_expand_depth = -1;

  /**
   * Enable tabular formatting for arrays of similar objects (default: true).
   * When enabled, arrays of objects with identical keys are formatted
   * as aligned tables.
   */
  bool enable_table_format = true;

  /**
   * Maximum complexity for table formatting. (default: 2).
   */
  size_t max_table_row_complexity = 2;

  /**
   * Enable compact multiline arrays (default: true).
   * When enabled, arrays of simple elements may have multiple items
   * per line.
   */
  bool enable_compact_multiline = true;

  /**
   * Minimum number of rows a compact multiline array must be able to pack
   * per line to be used. (default: 3)
   */
  size_t min_compact_array_row_items = 3;

  /**
   * Add space inside brackets for containers that hold only scalar values
   * (default: true). When true: { "key": "value" }. When false:
   * {"key": "value"}.
   * @see nested_bracket_padding
   */
  bool simple_bracket_padding = true;

  /**
   * Add space inside brackets for containers that hold at least one
   * nested array/object (default: true). When true: { "a": [ 1, 2 ] }.
   * When false: { "a": [1, 2]}.
   * @see simple_bracket_padding
   */
  bool nested_bracket_padding = true;

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

  /**
   * Placement of commas relative to column padding in table-formatted rows
   * (default: before_padding_except_numbers).
   */
  table_comma_placement comma_placement = table_comma_placement::before_padding_except_numbers;
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
