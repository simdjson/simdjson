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

  /** Track current line length for compact multiline decisions */
  void track_line_length(size_t chars);

  /** Check if we should break to a new line in compact mode */
  bool should_break_line(size_t upcoming_length) const;

  /** Get the options */
  const fractured_json_options& options() const;

private:
  fractured_json_options options_;
  layout_mode current_layout_ = layout_mode::expanded;
  size_t current_line_length_ = 0;
};

/** RAII helper forcing single line layout and restoring previous mode on exit */
class scoped_single_line_mode {
public:
  explicit scoped_single_line_mode(fractured_formatter& format)
      : format_(format), prev_(format.get_layout_mode()) {
    format_.set_layout_mode(layout_mode::single_line);
  }
  ~scoped_single_line_mode() { format_.set_layout_mode(prev_); }
  scoped_single_line_mode(const scoped_single_line_mode&) = delete;
  scoped_single_line_mode& operator=(const scoped_single_line_mode&) = delete;

private:
  fractured_formatter& format_;
  layout_mode prev_;
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
  void format_element(const dom::element& elem, const element_metrics& metrics, size_t depth,
                       bool has_trailing_comma = false);

  /** Format an array with the appropriate layout */
  void format_array(const dom::array& arr, const element_metrics& metrics, size_t depth,
                     bool has_trailing_comma = false);

  /** Format an array inline: [1, 2, 3] */
  void format_array_inline(const dom::array& arr, const element_metrics& metrics);

  /** Format an array with compact multiline: multiple items per line */
  void format_array_compact_multiline(const dom::array& arr, const element_metrics& metrics, size_t depth);

  /** Like format_array_compact_multiline, but rows are cross-row aligned
   * and packed using a fixed per-row slot width. */
  void format_array_compact_multiline_aligned(const dom::array& arr, const element_metrics& metrics, size_t depth);

  /** Format an array as a table */
  void format_array_as_table(const dom::array& arr, const element_metrics& metrics, size_t depth);

  /** Write one object row's columns */
  void format_table_object_row(const dom::object& obj, const element_metrics& row_metrics,
                                const std::vector<table_column>& columns, size_t depth);

  /** Write one array row's columns */
  void format_table_array_row(const dom::array& arr, const element_metrics& row_metrics,
                               const std::vector<table_column>& columns, size_t depth);

  /** Dispatches to format_table_object_row/format_table_array_row based on elem's type. */
  void format_table_row(const dom::element& elem, const element_metrics& row_metrics,
                        const std::vector<table_column>& columns, size_t depth);

  /** Row for a uniform scalar array: writes elem inline, then pads to width so every row lines up. */
  void format_table_scalar_row(const dom::element& elem, const element_metrics& row_metrics,
                                size_t width, size_t depth, table_column_type column_type);

  /** Shared per-column writer: recurses if the column has children,
   * otherwise writes a plain padded value or blank. */
  void format_table_row_columns(const std::vector<table_column>& columns,
                                 const std::vector<bool>& found,
                                 const std::vector<dom::element>& values,
                                 const std::vector<const element_metrics*>& value_metrics,
                                 size_t depth);

  /** Writes a single aligned leaf value */
  void format_table_leaf_value(const dom::element& elem, const element_metrics& vm, size_t width,
                                table_column_type column_type, bool needs_comma,
                                bool add_comma_space, size_t depth);

  /** Whether, for a column of the given type, the comma goes right after the value */
  bool comma_goes_before_padding(table_column_type column_type) const;

  /** Format an array expanded: one item per line */
  void format_array_expanded(const dom::array& arr, const element_metrics& metrics, size_t depth);

  /** Format an object with the appropriate layout */
  void format_object(const dom::object& obj, const element_metrics& metrics, size_t depth,
                      bool has_trailing_comma = false);

  /** Format an object inline: {"a": 1, "b": 2} */
  void format_object_inline(const dom::object& obj, const element_metrics& metrics);

  /** Format an object expanded: one key per line */
  void format_object_expanded(const dom::object& obj, const element_metrics& metrics, size_t depth);

  /** Format a scalar value */
  void format_scalar(const dom::element& elem);

  /** Whether to pad this container's own brackets. */
  bool bracket_padding_for(const element_metrics& metrics) const;
};

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_FRACTURED_FORMATTER_H
