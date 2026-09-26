#ifndef SIMDJSON_INTERNAL_JSON_STRUCTURE_ANALYZER_H
#define SIMDJSON_INTERNAL_JSON_STRUCTURE_ANALYZER_H

#include "simdjson/dom/base.h"
#include "simdjson/dom/element.h"
#include "simdjson/dom/array.h"
#include "simdjson/dom/object.h"
#include "simdjson/dom/fractured_json.h"
#include "simdjson/internal/tape_type.h"

#include <vector>
#include <string>
#include <string_view>
#include <utility>

namespace simdjson {
namespace internal {

/**
 * Layout mode for fractured JSON formatting.
 */
enum class layout_mode {
  single_line,         // Single line: [1, 2, 3] or {"a": 1}
  compact_multiline,   // Multiple items per line with breaks
  table,               // Tabular format for arrays of similar objects
  expanded             // Traditional multi-line with indentation
};

/** Kind of value found in a table column across all rows that have one. */
enum class table_column_type {
  unknown,
  simple,   // string, bool, or null
  number,
  array,
  object,
  mixed     // rows disagree on kind
};

/** Column of a table-formatted array.*/
struct table_column {
  /** Column name for object rows; empty for array rows. */
  std::string key{};
  /** Rendered length of key */
  size_t key_width = 0;
  table_column_type type = table_column_type::unknown;
  /** Widest rendered value in this column (when fully expanding all children) */
  size_t width = 0;
  /** Widest plain value in this column. */
  size_t plain_width = 0;
  /** subcolumns (only populated if every child is an array or every child is an object) */
  std::vector<table_column> children{};
};

/** Whether rows from this column list use nested vs. simple bracket padding
 * One shared decision, since picking it per-row would misalign width-aligned rows. */
inline bool table_row_is_nested(const std::vector<table_column>& columns) {
  for (const table_column& col : columns) {
    if (col.type == table_column_type::object || col.type == table_column_type::array) {
      return true;
    }
  }
  return false;
}

/**
 * Metrics computed for a JSON element during structure analysis.
 * These metrics drive layout decisions and contain child metrics for recursive formatting.
 */
struct element_metrics {
  /** Nesting depth score (0 = scalar, 1 = flat container, etc.) */
  size_t complexity = 0;

  /** Estimated character length if rendered inline (minified + spaces) */
  size_t estimated_inline_len = 0;

  /** Number of direct children (0 for scalars) */
  size_t child_count = 0;

  /** Pre-computed: can this element be rendered inline? */
  bool can_inline = false;

  /** Is this an array where all elements have similar structure? */
  bool is_uniform_array = false;

  /** for uniform arrays: this array's columns for alignment */
  std::vector<table_column> table_columns{};
  /** Widest table row after pruning recursive columns that don't fit into line budget */
  size_t table_row_width = 0;
  /** Widest table row without pruning recursive columns that don't fit into line budget */
  size_t table_row_width_full = 0;

  /** Child metrics for arrays and objects (in order of iteration) */
  std::vector<element_metrics> children{};

  /** For scalar uniform arrays (table_columns empty): the rows' common type. */
  table_column_type scalar_column_type = table_column_type::unknown;
};

/** children[idx], or a default-constructed element_metrics if idx is out of range */
inline const element_metrics& child_metrics_at(const std::vector<element_metrics>& children, size_t idx) {
  static const element_metrics empty{};
  return idx < children.size() ? children[idx] : empty;
}

/** *ptr, or a default-constructed element_metrics if ptr is null. */
inline const element_metrics& child_metrics_at(const element_metrics* ptr) {
  static const element_metrics empty{};
  return ptr ? *ptr : empty;
}

/**
 * Analyzes JSON structure to compute metrics for formatting decisions.
 *
 * The analyzer performs a single pass over the DOM to compute:
 * - Complexity (nesting depth)
 * - Estimated inline length
 * - Array uniformity for table detection
 *
 * Metrics are stored hierarchically with child metrics embedded in parent metrics,
 * enabling efficient lookup during formatting without address-based caching.
 */
class structure_analyzer {
public:
  /** Default constructor */
  structure_analyzer() : current_opts_(nullptr) {}

  /** Copy constructor - deleted since class has pointer member */
  structure_analyzer(const structure_analyzer&) = delete;

  /** Copy assignment - deleted since class has pointer member */
  structure_analyzer& operator=(const structure_analyzer&) = delete;

  /** Move constructor */
  structure_analyzer(structure_analyzer&&) = default;

  /** Move assignment */
  structure_analyzer& operator=(structure_analyzer&&) = default;

  /**
   * Analyze a DOM element and compute metrics.
   * @param elem The element to analyze
   * @param opts Formatting options that affect metric computation
   * @return Metrics for the root element (with child metrics embedded)
   */
  element_metrics analyze(const dom::element& elem,
                          const fractured_json_options& opts);

  /**
   * Clear state.
   */
  void clear();

  /**
   * Analyze an array element directly (for standalone array formatting).
   * @param arr The array to analyze
   * @param opts Formatting options
   * @return Metrics for the array
   */
  element_metrics analyze_array(const dom::array& arr,
                                const fractured_json_options& opts);

  /**
   * Analyze an object element directly (for standalone object formatting).
   * @param obj The object to analyze
   * @param opts Formatting options
   * @return Metrics for the object
   */
  element_metrics analyze_object(const dom::object& obj,
                                 const fractured_json_options& opts);

  /** Decide layout at the given render depth. Kept out of analysis since
   * the same metrics can render inline or expanded at different depths. */
  static layout_mode decide_layout(const element_metrics& metrics,
                                    size_t depth,
                                    const fractured_json_options& opts,
                                    bool has_trailing_comma = false);

private:
  const fractured_json_options* current_opts_ = nullptr;

  /** Recursive analysis implementation */
  element_metrics analyze_element(const dom::element& elem, size_t depth) const;

  /** Analyze scalar values (strings, numbers, booleans, null) */
  element_metrics analyze_scalar(const dom::element& elem) const;

  /** Analyze an array element */
  element_metrics analyze_array(const dom::array& arr, size_t depth) const;

  /** Analyze an object element */
  element_metrics analyze_object(const dom::object& obj, size_t depth) const;

  /** Estimate inline length for a string (including quotes and escaping) */
  size_t estimate_string_length(std::string_view s) const;

  /** Estimate inline length for a number */
  size_t estimate_number_length(double d) const;
  size_t estimate_number_length(int64_t i) const;
  size_t estimate_number_length(uint64_t u) const;

  /**
   * Check if an array contains uniformly-shaped rows suitable for table
   * formatting, filling in corresponding metrics
   * @param arr The array to check
   * @param metrics The array's metrics, with children already filled in
   * @param depth The array's depth
   */
  bool check_array_uniformity(const dom::array& arr, element_metrics& metrics, size_t depth) const;

  static table_column_type classify_table_value(dom::element_type type);

  /** Find common type across a set of sibling values and the widest of their rendered lengths */
  static void classify_and_measure(const std::vector<std::pair<dom::element, const element_metrics*>>& values,
                                    table_column_type& common, size_t& max_width);

  /** Recursively build table columns for an array */
  void build_table_columns(const std::vector<std::pair<dom::element, const element_metrics*>>& values,
                            std::vector<table_column>& out_columns) const;

  /** Rendered width of a row, assuming each column's current */
  size_t compute_columns_width(const std::vector<table_column>& columns) const;

  /** Height of a column's recursion (0 = leaf). */
  static size_t column_height(const table_column& column);

  /** Flatten deepest columns of the table */
  static bool flatten_deepest_columns(std::vector<table_column>& columns);

  /** Bottom-up refresh of column widths after flatten_deepest_columns. */
  void recompute_column_widths(std::vector<table_column>& columns) const;
};

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_JSON_STRUCTURE_ANALYZER_H
