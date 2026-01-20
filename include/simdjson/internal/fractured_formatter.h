#ifndef SIMDJSON_INTERNAL_FRACTURED_FORMATTER_H
#define SIMDJSON_INTERNAL_FRACTURED_FORMATTER_H

#include "simdjson/dom/serialization.h"
#include "simdjson/dom/fractured_json.h"
#include "simdjson/internal/json_structure_analyzer.h"

namespace simdjson {
namespace internal {

/**
 * Fractured JSON formatter using CRTP pattern.
 *
 * This formatter intelligently chooses between different layout modes
 * (inline, compact multiline, table, expanded) based on pre-computed
 * structure metrics.
 */
class fractured_formatter : public base_formatter<fractured_formatter> {
public:
  explicit fractured_formatter(const fractured_json_options& opts = {});

  /** CRTP hook: print newline (context-aware) */
  simdjson_inline void print_newline();

  /** CRTP hook: print indentation */
  simdjson_inline void print_indents(size_t depth);

  /** CRTP hook: print space (context-aware) */
  simdjson_inline void print_space();

  /** Set the current layout mode */
  void set_layout_mode(layout_mode mode);

  /** Get the current layout mode */
  layout_mode get_layout_mode() const;

  /** Set current depth for formatting decisions */
  void set_depth(size_t depth);

  /** Get current depth */
  size_t get_depth() const;

  /** Track current line length for compact multiline decisions */
  void track_line_length(size_t chars);

  /** Reset line length (after newline) */
  void reset_line_length();

  /** Get current line length */
  size_t get_line_length() const;

  /** Check if we should break to a new line in compact mode */
  bool should_break_line(size_t upcoming_length) const;

  /** Get the options */
  const fractured_json_options& options() const;

  // Table formatting support
  /** Begin a table row */
  void begin_table_row();

  /** End a table row */
  void end_table_row();

  /** Set column widths for table alignment */
  void set_column_widths(const std::vector<size_t>& widths);

  /** Get current column index in table mode */
  size_t get_column_index() const;

  /** Advance to next column */
  void next_column();

  /** Add padding to align with column width */
  void align_to_column_width(size_t actual_width);

private:
  fractured_json_options options_;
  layout_mode current_layout_ = layout_mode::EXPANDED;
  size_t current_depth_ = 0;
  size_t current_line_length_ = 0;

  // Table state
  bool in_table_mode_ = false;
  std::vector<size_t> column_widths_;
  size_t current_column_ = 0;
};

/**
 * Specialized string builder for fractured JSON formatting.
 *
 * This builder performs two passes:
 * 1. Analyze the structure to compute metrics
 * 2. Format using the metrics to make layout decisions
 */
class fractured_string_builder {
public:
  fractured_string_builder(const fractured_json_options& opts = {});

  /** Append a DOM element with fractured formatting */
  void append(const dom::element& value);

  /** Append a DOM array with fractured formatting */
  void append(const dom::array& value);

  /** Append a DOM object with fractured formatting */
  void append(const dom::object& value);

  /** Clear the builder */
  simdjson_inline void clear();

  /** Get the formatted string */
  simdjson_inline std::string_view str() const;

private:
  fractured_formatter format_;
  structure_analyzer analyzer_;
  fractured_json_options options_;

  /** Format an element using pre-computed metrics */
  void format_element(const dom::element& elem, const element_metrics& metrics, size_t depth);

  /** Format an array with the appropriate layout */
  void format_array(const dom::array& arr, const element_metrics& metrics, size_t depth);

  /** Format an array inline: [1, 2, 3] */
  void format_array_inline(const dom::array& arr, const element_metrics& metrics);

  /** Format an array with compact multiline: multiple items per line */
  void format_array_compact_multiline(const dom::array& arr, const element_metrics& metrics, size_t depth);

  /** Format an array as a table */
  void format_array_as_table(const dom::array& arr, const element_metrics& metrics, size_t depth);

  /** Format an array expanded: one item per line */
  void format_array_expanded(const dom::array& arr, const element_metrics& metrics, size_t depth);

  /** Format an object with the appropriate layout */
  void format_object(const dom::object& obj, const element_metrics& metrics, size_t depth);

  /** Format an object inline: {"a": 1, "b": 2} */
  void format_object_inline(const dom::object& obj, const element_metrics& metrics);

  /** Format an object expanded: one key per line */
  void format_object_expanded(const dom::object& obj, const element_metrics& metrics, size_t depth);

  /** Format a scalar value */
  void format_scalar(const dom::element& elem);

  /** Calculate column widths for table formatting */
  std::vector<size_t> calculate_column_widths(const dom::array& arr,
                                               const std::vector<std::string>& columns) const;

  /** Measure the actual formatted length of a value (for alignment) */
  size_t measure_value_length(const dom::element& elem) const;
};

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_FRACTURED_FORMATTER_H
