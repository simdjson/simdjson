#ifndef SIMDJSON_DOM_FRACTURED_JSON_INL_H
#define SIMDJSON_DOM_FRACTURED_JSON_INL_H

#include "simdjson/dom/fractured_json.h"
#include "simdjson/dom/serialization.h"
#include "simdjson/dom/element-inl.h"
#include "simdjson/dom/array-inl.h"
#include "simdjson/dom/object-inl.h"
#include "simdjson/dom/parser-inl.h"
#include "simdjson/padded_string.h"
#include "simdjson/internal/json_structure_analyzer.h"
#include "simdjson/internal/fractured_formatter.h"

#include <cmath>
#include <algorithm>
#include <cstring>
#include <iterator>
#include <unordered_map>

namespace simdjson {
namespace internal {

//
// Structure Analyzer Implementation
//

inline element_metrics structure_analyzer::analyze(const dom::element& elem,
                                                    const fractured_json_options& opts) {
  current_opts_ = &opts;
  return analyze_element(elem, 0);
}

inline void structure_analyzer::clear() {
  current_opts_ = nullptr;
}

inline element_metrics structure_analyzer::analyze_array(const dom::array& arr,
                                                          const fractured_json_options& opts) {
  current_opts_ = &opts;
  return analyze_array(arr, 0);
}

inline element_metrics structure_analyzer::analyze_object(const dom::object& obj,
                                                           const fractured_json_options& opts) {
  current_opts_ = &opts;
  return analyze_object(obj, 0);
}

inline element_metrics structure_analyzer::analyze_element(const dom::element& elem, size_t depth) const {
  switch (elem.type()) {
    case dom::element_type::ARRAY: {
      dom::array arr;
      if (elem.get_array().get(arr) == SUCCESS) {
        return analyze_array(arr, depth);
      }
      break;
    }
    case dom::element_type::OBJECT: {
      dom::object obj;
      if (elem.get_object().get(obj) == SUCCESS) {
        return analyze_object(obj, depth);
      }
      break;
    }
    default:
      // Handle all scalar types with a helper
      return analyze_scalar(elem);
  }
  return element_metrics{};
}

inline element_metrics structure_analyzer::analyze_scalar(const dom::element& elem) const {
  element_metrics metrics;
  metrics.complexity = 0;
  metrics.child_count = 0;
  metrics.can_inline = true;

  switch (elem.type()) {
    case dom::element_type::STRING: {
      std::string_view str;
      if (elem.get_string().get(str) == SUCCESS) {
        metrics.estimated_inline_len = estimate_string_length(str);
      }
      break;
    }
    case dom::element_type::INT64: {
      int64_t val;
      if (elem.get_int64().get(val) == SUCCESS) {
        metrics.estimated_inline_len = estimate_number_length(val);
      }
      break;
    }
    case dom::element_type::UINT64: {
      uint64_t val;
      if (elem.get_uint64().get(val) == SUCCESS) {
        metrics.estimated_inline_len = estimate_number_length(val);
      }
      break;
    }
    case dom::element_type::DOUBLE: {
      double val;
      if (elem.get_double().get(val) == SUCCESS) {
        metrics.estimated_inline_len = estimate_number_length(val);
      }
      break;
    }
    case dom::element_type::BOOL: {
      bool val;
      if (elem.get_bool().get(val) == SUCCESS) {
        metrics.estimated_inline_len = val ? 4 : 5; // "true" or "false"
      }
      break;
    }
    case dom::element_type::NULL_VALUE:
      metrics.estimated_inline_len = 4; // "null"
      break;
    default:
      break;
  }

  return metrics;
}

inline element_metrics structure_analyzer::analyze_array(const dom::array& arr,
                                                          size_t depth) const {
  element_metrics metrics;
  metrics.complexity = 1; // At least 1 for being an array
  metrics.estimated_inline_len = 2; // "[]"
  metrics.child_count = 0;

  size_t max_child_complexity = 0;
  bool first = true;

  for (dom::element child : arr) {
    if (!first) {
      metrics.estimated_inline_len += 2; // ", "
    }
    first = false;

    element_metrics child_metrics = analyze_element(child, depth + 1);
    metrics.estimated_inline_len += child_metrics.estimated_inline_len;
    max_child_complexity = (std::max)(max_child_complexity, child_metrics.complexity);
    metrics.child_count++;
    metrics.children.push_back(std::move(child_metrics));
  }

  // Complexity is 1 + max child complexity
  metrics.complexity = 1 + max_child_complexity;

  // Bracket padding "[ 1, 2 ]" vs "[1, 2]"
  bool use_bracket_padding = (max_child_complexity >= 1)
      ? current_opts_->nested_bracket_padding : current_opts_->simple_bracket_padding;
  if (use_bracket_padding && metrics.child_count > 0) {
    metrics.estimated_inline_len += 2;
  }

  // Check if can inline
  metrics.can_inline = metrics.complexity <= current_opts_->max_inline_complexity;

  // Check for uniform array (table formatting, or aligned compact multiline).
  bool wants_table_columns =
      (current_opts_->enable_table_format &&
       max_child_complexity <= current_opts_->max_table_row_complexity) ||
      (current_opts_->enable_compact_multiline &&
       max_child_complexity <= current_opts_->max_compact_array_complexity);
  if (wants_table_columns) {
    metrics.is_uniform_array = check_array_uniformity(arr, metrics, depth);
  }

  return metrics;
}

inline element_metrics structure_analyzer::analyze_object(const dom::object& obj,
                                                           size_t depth) const {
  element_metrics metrics;
  metrics.complexity = 1;
  metrics.estimated_inline_len = 2; // "{}"
  metrics.child_count = 0;

  size_t max_child_complexity = 0;
  bool first = true;

  for (dom::key_value_pair field : obj) {
    if (!first) {
      metrics.estimated_inline_len += 2; // ", "
    }
    first = false;

    // Key length: quotes + key + colon + space
    metrics.estimated_inline_len += estimate_string_length(field.key) + 2;

    element_metrics child_metrics = analyze_element(field.value, depth + 1);
    metrics.estimated_inline_len += child_metrics.estimated_inline_len;
    max_child_complexity = (std::max)(max_child_complexity, child_metrics.complexity);
    metrics.child_count++;
    metrics.children.push_back(std::move(child_metrics));
  }

  metrics.complexity = 1 + max_child_complexity;

  // Bracket padding '{ "a": 1 }' vs '{"a": 1}'
  bool use_bracket_padding = (max_child_complexity >= 1)
      ? current_opts_->nested_bracket_padding : current_opts_->simple_bracket_padding;
  if (use_bracket_padding && metrics.child_count > 0) {
    metrics.estimated_inline_len += 2;
  }

  metrics.can_inline = metrics.complexity <= current_opts_->max_inline_complexity;

  return metrics;
}

inline size_t structure_analyzer::estimate_string_length(std::string_view s) const {
  size_t len = 2; // quotes
  for (char c : s) {
    if (c == '"' || c == '\\' || static_cast<unsigned char>(c) < 32) {
      len += 2; // escape sequence (at least)
    } else {
      len += 1;
    }
  }
  return len;
}

inline size_t structure_analyzer::estimate_number_length(double d) const {
  if (!std::isfinite(d)) {
#if SIMDJSON_ENABLE_NAN_INF
    if (std::isnan(d)) {
      return 3; // "NaN"
    } else if (d < 0) {
      return 9; // "-Infinity"
    } else {
      return 8; // "Infinity"
    }
#else
    return 4; // "null" for invalid numbers
#endif
  }
  // Rough estimate: up to 17 significant digits + sign + decimal point + exponent
  char buf[32];
  int len = snprintf(buf, sizeof(buf), "%.17g", d);
  return len > 0 ? static_cast<size_t>(len) : 20;
}

inline size_t structure_analyzer::estimate_number_length(int64_t i) const {
  if (i == 0) return 1;
  // Handle INT64_MIN specially to avoid overflow when negating
  if (i == INT64_MIN) return 20; // "-9223372036854775808" is 20 characters
  size_t len = (i < 0) ? 1 : 0; // negative sign
  int64_t abs_val = (i < 0) ? -i : i;
  while (abs_val > 0) {
    len++;
    abs_val /= 10;
  }
  return len;
}

inline size_t structure_analyzer::estimate_number_length(uint64_t u) const {
  if (u == 0) return 1;
  size_t len = 0;
  while (u > 0) {
    len++;
    u /= 10;
  }
  return len;
}

inline table_column_type structure_analyzer::classify_table_value(dom::element_type type) {
  switch (type) {
    case dom::element_type::OBJECT: return table_column_type::object;
    case dom::element_type::ARRAY: return table_column_type::array;
    case dom::element_type::INT64:
    case dom::element_type::UINT64:
    case dom::element_type::DOUBLE: return table_column_type::number;
    case dom::element_type::NULL_VALUE: return table_column_type::unknown;
    default: return table_column_type::simple; // string, bool
  }
}

inline void structure_analyzer::classify_and_measure(
    const std::vector<std::pair<dom::element, const element_metrics*>>& values,
    table_column_type& common, size_t& max_width) {
  common = table_column_type::unknown;
  max_width = 0;
  for (const auto& v : values) {
    table_column_type t = classify_table_value(v.first.type());
    if (t != table_column_type::unknown) {
      if (common == table_column_type::unknown) common = t;
      else if (t != common) common = table_column_type::mixed;
    }
    if (v.second) {
      max_width = (std::max)(max_width, v.second->estimated_inline_len);
    }
  }
}

inline void structure_analyzer::build_table_columns(
    const std::vector<std::pair<dom::element, const element_metrics*>>& values,
    std::vector<table_column>& out_columns) const {
  out_columns.clear();
  if (values.empty()) {
    return;
  }

  table_column_type common = table_column_type::unknown;
  for (const auto& v : values) {
    table_column_type t = classify_table_value(v.first.type());
    if (t == table_column_type::unknown) continue;
    if (common == table_column_type::unknown) common = t;
    else if (t != common) { common = table_column_type::mixed; break; }
  }
  if (common != table_column_type::object && common != table_column_type::array) {
    return;
  }

  std::vector<std::vector<std::pair<dom::element, const element_metrics*>>> per_column_values;

  if (common == table_column_type::object) {
    std::unordered_map<std::string_view, size_t> column_index;
    for (const auto& v : values) {
      if (v.first.type() != dom::element_type::OBJECT) continue;
      dom::object obj;
      if (v.first.get_object().get(obj) != SUCCESS) continue;

      size_t field_idx = 0;
      for (dom::key_value_pair field : obj) {
        auto it = column_index.find(field.key);
        size_t col_idx;
        if (it == column_index.end()) {
          col_idx = out_columns.size();
          column_index.emplace(field.key, col_idx);
          out_columns.emplace_back();
          out_columns.back().key.assign(field.key.data(), field.key.size());
          out_columns.back().key_width = estimate_string_length(field.key);
          per_column_values.emplace_back();
        } else {
          col_idx = it->second;
        }
        const element_metrics* field_metrics = (v.second && field_idx < v.second->children.size())
            ? &v.second->children[field_idx] : nullptr;
        per_column_values[col_idx].emplace_back(field.value, field_metrics);
        field_idx++;
      }
    }
  } else { // array: columns by position
    for (const auto& v : values) {
      if (v.first.type() != dom::element_type::ARRAY) continue;
      dom::array sub_arr;
      if (v.first.get_array().get(sub_arr) != SUCCESS) continue;

      size_t idx = 0;
      for (dom::element item : sub_arr) {
        if (out_columns.size() <= idx) {
          out_columns.emplace_back();
          per_column_values.emplace_back();
        }
        const element_metrics* item_metrics = (v.second && idx < v.second->children.size())
            ? &v.second->children[idx] : nullptr;
        per_column_values[idx].emplace_back(item, item_metrics);
        idx++;
      }
    }
  }

  for (size_t i = 0; i < out_columns.size(); i++) {
    table_column_type col_type;
    size_t max_width;
    classify_and_measure(per_column_values[i], col_type, max_width);
    out_columns[i].type = col_type;
    out_columns[i].plain_width = max_width;

    if (col_type == table_column_type::object || col_type == table_column_type::array) {
      build_table_columns(per_column_values[i], out_columns[i].children);
    }

    out_columns[i].width = out_columns[i].children.empty() ? max_width : compute_columns_width(out_columns[i].children);
  }
}

inline bool structure_analyzer::check_array_uniformity(const dom::array& arr,
                                                        element_metrics& metrics,
                                                        size_t depth) const {
  std::vector<std::pair<dom::element, const element_metrics*>> values;
  values.reserve(metrics.child_count);

  size_t row_idx = 0;
  for (dom::element elem : arr) {
    const element_metrics* row_metrics = (row_idx < metrics.children.size()) ? &metrics.children[row_idx] : nullptr;
    values.emplace_back(elem, row_metrics);
    row_idx++;
  }

  build_table_columns(values, metrics.table_columns);
  if (metrics.table_columns.empty()) {
    // Not uniformly object or array. Check for uniform scalar
    table_column_type common;
    size_t max_width;
    classify_and_measure(values, common, max_width);
    if (common != table_column_type::number && common != table_column_type::simple) {
      return false;
    }
    metrics.scalar_column_type = common;
    metrics.table_row_width = max_width;
    metrics.table_row_width_full = max_width;
    return true;
  }

  metrics.table_row_width_full = compute_columns_width(metrics.table_columns);

  size_t row_indent_width = (depth + 1) * current_opts_->indent_spaces;
  size_t budget = (row_indent_width + 1 >= current_opts_->max_total_line_length)
      ? 0 : current_opts_->max_total_line_length - row_indent_width - 1;

  size_t width = metrics.table_row_width_full;
  while (width > budget && flatten_deepest_columns(metrics.table_columns)) {
    recompute_column_widths(metrics.table_columns);
    width = compute_columns_width(metrics.table_columns);
  }

  metrics.table_row_width = width;
  return true;
}

inline size_t structure_analyzer::compute_columns_width(const std::vector<table_column>& columns) const {
  size_t width = 2; // "{}" or "[]"
  if (table_row_is_nested(columns) ? current_opts_->nested_bracket_padding : current_opts_->simple_bracket_padding) {
    width += 2;
  }

  for (const table_column& col : columns) {
    if (!col.key.empty()) {
      width += col.key_width;
      width += current_opts_->colon_padding ? 2 : 1;
    }
    width += col.width;
  }
  if (columns.size() > 1) {
    width += (columns.size() - 1) * (current_opts_->comma_padding ? 2 : 1);
  }
  return width;
}

inline size_t structure_analyzer::column_height(const table_column& column) {
  size_t height = 0;
  for (const table_column& child : column.children) {
    height = (std::max)(height, column_height(child));
  }
  return column.children.empty() ? 0 : height + 1;
}

inline bool structure_analyzer::flatten_deepest_columns(std::vector<table_column>& columns) {
  size_t max_height = 0;
  for (const table_column& col : columns) {
    max_height = (std::max)(max_height, column_height(col));
  }

  bool changed = false;
  for (table_column& col : columns) {
    if (column_height(col) != max_height || max_height == 0) continue;
    if (max_height == 1) {
      col.children.clear();
      col.width = col.plain_width;
      changed = true;
    } else {
      changed |= flatten_deepest_columns(col.children);
    }
  }
  return changed;
}

inline void structure_analyzer::recompute_column_widths(std::vector<table_column>& columns) const {
  for (table_column& col : columns) {
    if (!col.children.empty()) {
      recompute_column_widths(col.children);
      col.width = compute_columns_width(col.children);
    }
  }
}

inline layout_mode structure_analyzer::decide_layout(const element_metrics& metrics,
                                                      size_t depth,
                                                      const fractured_json_options& opts,
                                                      bool has_trailing_comma) {
  if (metrics.child_count == 0) {
    return layout_mode::single_line;
  }

  long long signed_depth = static_cast<long long>(depth);
  bool depth_allows_inline_or_compact = signed_depth > opts.always_expand_depth;
  bool depth_allows_table = signed_depth >= opts.always_expand_depth;

  // Check inline feasibility
  size_t reserved_width = depth * opts.indent_spaces + (has_trailing_comma ? 1 : 0);
  if (depth_allows_inline_or_compact && metrics.can_inline &&
      metrics.estimated_inline_len + reserved_width <= opts.max_total_line_length) {
    return layout_mode::single_line;
  }

  // Rows (table's or compact multiline's) render one level deeper than the
  // array itself.
  size_t row_indent_width = (depth + 1) * opts.indent_spaces;

  // Check compact multiline
  // for uniform arrays fall back to table if we would have to flatten any formatting
  bool compact_multiline_enabled = opts.enable_compact_multiline &&
      metrics.complexity <= opts.max_compact_array_complexity + 1 &&
      metrics.child_count >= opts.min_compact_array_row_items;
  if (depth_allows_inline_or_compact && compact_multiline_enabled) {
    bool aligned = metrics.is_uniform_array;
    size_t comma_width = opts.comma_padding ? 2 : 1;
    size_t avg_item_width;
    if (aligned) {
      avg_item_width = metrics.table_row_width_full + comma_width;
    } else {
      size_t sum = 0;
      for (const element_metrics& child : metrics.children) {
        sum += child.estimated_inline_len;
      }
      avg_item_width = comma_width + sum / metrics.child_count;
    }

    size_t row_pack_space = (row_indent_width >= opts.max_total_line_length)
        ? 0 : opts.max_total_line_length - row_indent_width;
    if (avg_item_width * opts.min_compact_array_row_items <= row_pack_space) {
      return layout_mode::compact_multiline;
    }
  }

  // Check Table mode
  if (depth_allows_table && opts.enable_table_format &&
      metrics.is_uniform_array &&
      metrics.table_row_width + 1 + row_indent_width <= opts.max_total_line_length) {
    return layout_mode::table;
  }

  return layout_mode::expanded;
}

//
// Fractured Formatter Implementation
//

inline fractured_formatter::fractured_formatter(const fractured_json_options& opts)
    : options_(opts) {}

simdjson_inline void fractured_formatter::print_newline() {
  if (current_layout_ == layout_mode::single_line) {
    return; // No newlines in inline mode
  }
  one_char('\n');
  current_line_length_ = 0;
}

simdjson_inline void fractured_formatter::print_indents(size_t depth) {
  if (current_layout_ == layout_mode::single_line) {
    return; // No indentation in inline mode
  }
  for (size_t i = 0; i < depth * options_.indent_spaces; i++) {
    one_char(' ');
    current_line_length_++;
  }
}

simdjson_inline void fractured_formatter::print_space() {
  one_char(' ');
  current_line_length_++;
}

inline void fractured_formatter::set_layout_mode(layout_mode mode) {
  current_layout_ = mode;
}

inline layout_mode fractured_formatter::get_layout_mode() const {
  return current_layout_;
}

inline void fractured_formatter::track_line_length(size_t chars) {
  current_line_length_ += chars;
}

inline bool fractured_formatter::should_break_line(size_t upcoming_length) const {
  return (current_line_length_ + upcoming_length) > options_.max_total_line_length;
}

inline const fractured_json_options& fractured_formatter::options() const {
  return options_;
}

//
// Fractured String Builder Implementation
//

inline fractured_string_builder::fractured_string_builder(const fractured_json_options& opts)
    : format_(opts), analyzer_{}, options_(opts) {}

inline void fractured_string_builder::append(const dom::element& value) {
  // Phase 1: Analyze structure (metrics tree is built recursively)
  element_metrics root_metrics = analyzer_.analyze(value, options_);

  // Phase 2: Format using metrics tree (passed through recursion)
  format_element(value, root_metrics, 0);
}

inline void fractured_string_builder::append(const dom::array& value) {
  // Analyze the array to get proper metrics with children
  element_metrics metrics = analyzer_.analyze_array(value, options_);
  format_array(value, metrics, 0);
}

inline void fractured_string_builder::append(const dom::object& value) {
  // Analyze the object to get proper metrics with children
  element_metrics metrics = analyzer_.analyze_object(value, options_);
  format_object(value, metrics, 0);
}

simdjson_inline void fractured_string_builder::clear() {
  format_.clear();
  analyzer_.clear();
}

simdjson_inline std::string_view fractured_string_builder::str() const {
  return format_.str();
}

inline void fractured_string_builder::format_element(const dom::element& elem,
                                                       const element_metrics& metrics,
                                                       size_t depth,
                                                       bool has_trailing_comma) {
  switch (elem.type()) {
    case dom::element_type::ARRAY: {
      dom::array arr;
      if (elem.get_array().get(arr) == SUCCESS) {
        format_array(arr, metrics, depth, has_trailing_comma);
      }
      break;
    }
    case dom::element_type::OBJECT: {
      dom::object obj;
      if (elem.get_object().get(obj) == SUCCESS) {
        format_object(obj, metrics, depth, has_trailing_comma);
      }
      break;
    }
    default:
      format_scalar(elem);
      break;
  }
}

inline void fractured_string_builder::format_array(const dom::array& arr,
                                                    const element_metrics& metrics,
                                                    size_t depth,
                                                    bool has_trailing_comma) {
  layout_mode layout = structure_analyzer::decide_layout(metrics, depth, options_, has_trailing_comma);
  switch (layout) {
    case layout_mode::single_line:
      format_array_inline(arr, metrics);
      break;
    case layout_mode::compact_multiline:
      format_array_compact_multiline(arr, metrics, depth);
      break;
    case layout_mode::table:
      format_array_as_table(arr, metrics, depth);
      break;
    case layout_mode::expanded:
    default:
      format_array_expanded(arr, metrics, depth);
      break;
  }
}

inline void fractured_string_builder::format_array_inline(const dom::array& arr,
                                                            const element_metrics& metrics) {
  scoped_single_line_mode single_line(format_);

  format_.start_array();

  bool first = true;
  bool empty = true;
  size_t child_idx = 0;
  for (dom::element elem : arr) {
    empty = false;
    if (!first) {
      format_.comma();
      if (options_.comma_padding) {
        format_.print_space();
      }
    } else if (bracket_padding_for(metrics)) {
      format_.print_space();
    }
    first = false;
    const element_metrics& child_metrics = child_metrics_at(metrics.children, child_idx);
    format_element(elem, child_metrics, 0);
    child_idx++;
  }

  if (bracket_padding_for(metrics) && !empty) {
    format_.print_space();
  }
  format_.end_array();
}

inline void fractured_string_builder::format_array_compact_multiline(const dom::array& arr,
                                                                       const element_metrics& metrics,
                                                                       size_t depth) {
  if (metrics.is_uniform_array) {
    format_array_compact_multiline_aligned(arr, metrics, depth);
    return;
  }

  format_.start_array();
  format_.print_newline();
  format_.print_indents(depth + 1);

  bool first = true;
  bool prev_item_was_expanded = false;
  size_t child_idx = 0;

  for (dom::element elem : arr) {
    const element_metrics& child_metrics = child_metrics_at(metrics.children, child_idx);

    if (!first) {
      format_.comma();
      format_.track_line_length(1);

      // Check if we should break to new line
      if (prev_item_was_expanded ||
          format_.should_break_line(child_metrics.estimated_inline_len)) {
        format_.print_newline();
        format_.print_indents(depth + 1);
      } else if (options_.comma_padding) {
        format_.print_space();
      }
    }
    first = false;

    bool is_last = (child_idx + 1 == metrics.child_count);
    layout_mode item_layout = structure_analyzer::decide_layout(child_metrics, depth + 1, options_, !is_last);
    bool item_fits = item_layout == layout_mode::single_line;
    if (item_fits) {
      {
        scoped_single_line_mode single_line(format_);
        format_element(elem, child_metrics, depth + 1, !is_last);
      }
      format_.track_line_length(child_metrics.estimated_inline_len);
    } else {
      format_element(elem, child_metrics, depth + 1, !is_last);
    }
    prev_item_was_expanded = !item_fits;

    child_idx++;
  }

  format_.print_newline();
  format_.print_indents(depth);
  format_.end_array();
}

inline void fractured_string_builder::format_array_compact_multiline_aligned(
    const dom::array& arr, const element_metrics& metrics, size_t depth) {
  const std::vector<table_column>& columns = metrics.table_columns;

  format_.start_array();
  format_.print_newline();
  format_.print_indents(depth + 1);

  size_t indent_width = (depth + 1) * options_.indent_spaces;
  size_t available_line_space = (indent_width >= options_.max_total_line_length)
      ? 0 : options_.max_total_line_length - indent_width;
  size_t comma_width = options_.comma_padding ? 2 : 1;
  size_t remaining_line_space = available_line_space;

  bool first = true;
  size_t child_idx = 0;

  for (dom::element elem : arr) {
    bool needs_comma = (child_idx + 1 < metrics.child_count);
    size_t space_needed = metrics.table_row_width_full + (needs_comma ? comma_width : 0);

    if (!first) {
      if (remaining_line_space < space_needed) {
        format_.print_newline();
        format_.print_indents(depth + 1);
        remaining_line_space = available_line_space;
      } else if (options_.comma_padding) {
        format_.print_space();
      }
    }
    first = false;

    const element_metrics& row_metrics = child_metrics_at(metrics.children, child_idx);
    if (columns.empty()) {
      format_table_scalar_row(elem, row_metrics, metrics.table_row_width_full, depth + 1,
                              metrics.scalar_column_type);
    } else {
      format_table_row(elem, row_metrics, columns, depth + 1);
    }
    if (needs_comma) {
      format_.comma();
    }
    remaining_line_space -= (std::min)(remaining_line_space, space_needed);
    child_idx++;
  }

  format_.print_newline();
  format_.print_indents(depth);
  format_.end_array();
}

inline void fractured_string_builder::format_table_row_columns(
    const std::vector<table_column>& columns,
    const std::vector<bool>& found,
    const std::vector<dom::element>& values,
    const std::vector<const element_metrics*>& value_metrics,
    size_t depth) {
  const size_t num_columns = columns.size();
  size_t last_present_idx = num_columns;
  for (size_t i = 0; i < num_columns; i++) {
    if (found[i]) last_present_idx = i;
  }

  size_t comma_width = options_.comma_padding ? 2 : 1;

  for (size_t col_idx = 0; col_idx < num_columns; col_idx++) {
    const table_column& column = columns[col_idx];
    const bool is_last_col = (col_idx == num_columns - 1);

    if (found[col_idx]) {
      if (!column.key.empty()) {
        format_.key(column.key);
        if (options_.colon_padding) {
          format_.print_space();
        }
      }

      bool needs_comma = !is_last_col && (col_idx < last_present_idx);

      if (!column.children.empty()) {
        // Recurses into this cell's own columns instead of a plain value;
        // every row aligns those the same way (blank-padding missing
        // ones), so the result is always exactly column.width wide
        // no padding needed afterward, unlike the leaf case below.
        if (column.type == table_column_type::object) {
          dom::object sub_obj;
          if (values[col_idx].get_object().get(sub_obj) == SUCCESS) {
            const element_metrics& sub_metrics = child_metrics_at(value_metrics[col_idx]);
            format_table_object_row(sub_obj, sub_metrics, column.children, depth);
          }
        } else {
          dom::array sub_arr;
          if (values[col_idx].get_array().get(sub_arr) == SUCCESS) {
            const element_metrics& sub_metrics = child_metrics_at(value_metrics[col_idx]);
            format_table_array_row(sub_arr, sub_metrics, column.children, depth);
          }
        }
        // value is already padded
        if (needs_comma) {
          format_.comma();
          if (options_.comma_padding) {
            format_.print_space();
          }
        }
      } else {
        const element_metrics& vm = child_metrics_at(value_metrics[col_idx]);
        format_table_leaf_value(values[col_idx], vm, column.width, column.type, needs_comma,
                                 /*add_comma_space=*/true, depth);
      }

      if (!is_last_col && !needs_comma) {
        // Found, but no more real values follow: blank space where a comma would go.
        for (size_t i = 0; i < comma_width; i++) {
          format_.one_char(' ');
        }
      }
    } else {
      size_t slot_width = column.width;
      if (!column.key.empty()) {
        slot_width += column.key_width + (options_.colon_padding ? 2 : 1);
      }
      for (size_t i = 0; i < slot_width; i++) {
        format_.one_char(' ');
      }

      if (!is_last_col) {
        for (size_t i = 0; i < comma_width; i++) {
          format_.one_char(' ');
        }
      }
    }
  }
}

inline void fractured_string_builder::format_table_object_row(
    const dom::object& obj, const element_metrics& row_metrics,
    const std::vector<table_column>& columns, size_t depth) {
  const size_t num_columns = columns.size();
  std::vector<bool> found(num_columns, false);
  std::vector<dom::element> values(num_columns);
  std::vector<const element_metrics*> value_metrics(num_columns, nullptr);

  for (size_t col_idx = 0; col_idx < num_columns; col_idx++) {
    size_t field_idx = 0;
    for (dom::key_value_pair field : obj) {
      if (field.key == columns[col_idx].key) {
        found[col_idx] = true;
        values[col_idx] = field.value;
        value_metrics[col_idx] = (field_idx < row_metrics.children.size())
            ? &row_metrics.children[field_idx] : nullptr;
        break;
      }
      field_idx++;
    }
  }

  bool nested = table_row_is_nested(columns);
  format_.start_object();
  if (nested ? options_.nested_bracket_padding : options_.simple_bracket_padding) {
    format_.print_space();
  }
  format_table_row_columns(columns, found, values, value_metrics, depth);
  if (nested ? options_.nested_bracket_padding : options_.simple_bracket_padding) {
    format_.print_space();
  }
  format_.end_object();
}

inline void fractured_string_builder::format_table_array_row(
    const dom::array& arr, const element_metrics& row_metrics,
    const std::vector<table_column>& columns, size_t depth) {
  const size_t num_columns = columns.size();
  std::vector<bool> found(num_columns, false);
  std::vector<dom::element> values(num_columns);
  std::vector<const element_metrics*> value_metrics(num_columns, nullptr);

  size_t idx = 0;
  for (dom::element item : arr) {
    if (idx >= num_columns) break;
    found[idx] = true;
    values[idx] = item;
    value_metrics[idx] = (idx < row_metrics.children.size()) ? &row_metrics.children[idx] : nullptr;
    idx++;
  }

  bool nested = table_row_is_nested(columns);
  format_.start_array();
  if (nested ? options_.nested_bracket_padding : options_.simple_bracket_padding) {
    format_.print_space();
  }
  format_table_row_columns(columns, found, values, value_metrics, depth);
  if (nested ? options_.nested_bracket_padding : options_.simple_bracket_padding) {
    format_.print_space();
  }
  format_.end_array();
}

inline void fractured_string_builder::format_table_row(
    const dom::element& elem, const element_metrics& row_metrics,
    const std::vector<table_column>& columns, size_t depth) {
  if (elem.type() == dom::element_type::ARRAY) {
    dom::array arr;
    if (elem.get_array().get(arr) == SUCCESS) {
      format_table_array_row(arr, row_metrics, columns, depth);
    }
  } else {
    dom::object obj;
    if (elem.get_object().get(obj) == SUCCESS) {
      format_table_object_row(obj, row_metrics, columns, depth);
    }
  }
}

inline bool fractured_string_builder::comma_goes_before_padding(table_column_type column_type) const {
  switch (options_.comma_placement) {
    case table_comma_placement::before_padding: return true;
    case table_comma_placement::after_padding: return false;
    case table_comma_placement::before_padding_except_numbers:
    default:
      return column_type != table_column_type::number;
  }
}

inline void fractured_string_builder::format_table_leaf_value(
    const dom::element& elem, const element_metrics& vm, size_t width,
    table_column_type column_type, bool needs_comma, bool add_comma_space, size_t depth) {
  bool comma_before_pad = needs_comma && comma_goes_before_padding(column_type);
  bool comma_after_pad = needs_comma && !comma_before_pad;

  bool right_align = column_type == table_column_type::number &&
      options_.number_alignment == number_list_alignment::right;

  size_t value_len = vm.estimated_inline_len;
  size_t left_pad = 0;
  size_t right_pad = 0;
  if (right_align) {
    left_pad = (width > value_len) ? width - value_len : 0;
    comma_before_pad = needs_comma;
    comma_after_pad = false;
  } else {
    right_pad = (width > value_len) ? width - value_len : 0;
  }

  for (size_t i = 0; i < left_pad; i++) {
    format_.one_char(' ');
  }

  {
    scoped_single_line_mode single_line(format_);
    format_element(elem, vm, depth);
  }

  if (comma_before_pad) {
    format_.comma();
  }
  for (size_t i = 0; i < right_pad; i++) {
    format_.one_char(' ');
  }
  if (comma_after_pad) {
    format_.comma();
  }
  if (needs_comma && add_comma_space && options_.comma_padding) {
    format_.print_space();
  }
}

inline void fractured_string_builder::format_table_scalar_row(
    const dom::element& elem, const element_metrics& row_metrics, size_t width, size_t depth,
    table_column_type column_type) {
  format_table_leaf_value(elem, row_metrics, width, column_type,
                          /*needs_comma=*/false, /*add_comma_space=*/false, depth);
}

inline void fractured_string_builder::format_array_as_table(const dom::array& arr,
                                                             const element_metrics& metrics,
                                                             size_t depth) {
  if (!metrics.is_uniform_array) {
    format_array_expanded(arr, metrics, depth);
    return;
  }
  const std::vector<table_column>& columns = metrics.table_columns;

  format_.start_array();
  format_.print_newline();

  bool first_row = true;
  size_t child_idx = 0;
  for (dom::element elem : arr) {
    if (!first_row) {
      format_.comma();
      format_.print_newline();
    }
    first_row = false;

    format_.print_indents(depth + 1);

    const element_metrics& row_metrics = child_metrics_at(metrics.children, child_idx);
    if (columns.empty()) {
      format_table_scalar_row(elem, row_metrics, metrics.table_row_width, depth + 1,
                              metrics.scalar_column_type);
    } else {
      format_table_row(elem, row_metrics, columns, depth + 1);
    }
    child_idx++;
  }

  format_.print_newline();
  format_.print_indents(depth);
  format_.end_array();
}

inline void fractured_string_builder::format_array_expanded(const dom::array& arr,
                                                              const element_metrics& metrics,
                                                              size_t depth) {
  format_.start_array();

  bool empty = true;
  bool first = true;
  size_t child_idx = 0;

  for (dom::element elem : arr) {
    empty = false;
    if (!first) {
      format_.comma();
    }
    first = false;

    format_.print_newline();
    format_.print_indents(depth + 1);
    const element_metrics& child_metrics = child_metrics_at(metrics.children, child_idx);
    bool is_last = (child_idx + 1 == metrics.child_count);
    format_element(elem, child_metrics, depth + 1, !is_last);
    child_idx++;
  }

  if (!empty) {
    format_.print_newline();
    format_.print_indents(depth);
  }
  format_.end_array();
}

inline void fractured_string_builder::format_object(const dom::object& obj,
                                                     const element_metrics& metrics,
                                                     size_t depth,
                                                     bool has_trailing_comma) {
  layout_mode layout = structure_analyzer::decide_layout(metrics, depth, options_, has_trailing_comma);
  if (layout == layout_mode::single_line) {
    format_object_inline(obj, metrics);
  } else {
    format_object_expanded(obj, metrics, depth);
  }
}

inline void fractured_string_builder::format_object_inline(const dom::object& obj,
                                                             const element_metrics& metrics) {
  scoped_single_line_mode single_line(format_);

  format_.start_object();

  bool empty = true;
  bool first = true;
  size_t child_idx = 0;

  for (dom::key_value_pair field : obj) {
    empty = false;
    if (!first) {
      format_.comma();
      if (options_.comma_padding) {
        format_.print_space();
      }
    } else if (bracket_padding_for(metrics)) {
      format_.print_space();
    }
    first = false;

    format_.key(field.key);
    if (options_.colon_padding) {
      format_.print_space();
    }
    const element_metrics& child_metrics = child_metrics_at(metrics.children, child_idx);
    format_element(field.value, child_metrics, 0);
    child_idx++;
  }

  if (bracket_padding_for(metrics) && !empty) {
    format_.print_space();
  }
  format_.end_object();
}

inline void fractured_string_builder::format_object_expanded(const dom::object& obj,
                                                               const element_metrics& metrics,
                                                               size_t depth) {
  format_.start_object();

  bool empty = true;
  bool first = true;
  size_t child_idx = 0;

  for (dom::key_value_pair field : obj) {
    empty = false;
    if (!first) {
      format_.comma();
    }
    first = false;

    format_.print_newline();
    format_.print_indents(depth + 1);
    format_.key(field.key);
    if (options_.colon_padding) {
      format_.print_space();
    }
    const element_metrics& child_metrics = child_metrics_at(metrics.children, child_idx);
    bool is_last = (child_idx + 1 == metrics.child_count);
    format_element(field.value, child_metrics, depth + 1, !is_last);
    child_idx++;
  }

  if (!empty) {
    format_.print_newline();
    format_.print_indents(depth);
  }
  format_.end_object();
}

inline void fractured_string_builder::format_scalar(const dom::element& elem) {
  switch (elem.type()) {
    case dom::element_type::STRING: {
      std::string_view str;
      if (elem.get_string().get(str) == SUCCESS) {
        format_.string(str);
      }
      break;
    }
    case dom::element_type::INT64: {
      int64_t val;
      if (elem.get_int64().get(val) == SUCCESS) {
        format_.number(val);
      }
      break;
    }
    case dom::element_type::UINT64: {
      uint64_t val;
      if (elem.get_uint64().get(val) == SUCCESS) {
        format_.number(val);
      }
      break;
    }
    case dom::element_type::DOUBLE: {
      double val;
      if (elem.get_double().get(val) == SUCCESS) {
        format_.number(val);
      }
      break;
    }
    case dom::element_type::BOOL: {
      bool val;
      if (elem.get_bool().get(val) == SUCCESS) {
        val ? format_.true_atom() : format_.false_atom();
      }
      break;
    }
    case dom::element_type::NULL_VALUE:
      format_.null_atom();
      break;
    default:
      break;
  }
}

inline bool fractured_string_builder::bracket_padding_for(const element_metrics& metrics) const {
  return metrics.complexity >= 2 ? options_.nested_bracket_padding : options_.simple_bracket_padding;
}

} // namespace internal

//
// Public API Implementation
//

template <class T>
std::string fractured_json(T x) {
  return fractured_json(x, fractured_json_options{});
}

template <class T>
std::string fractured_json(T x, const fractured_json_options& options) {
  internal::fractured_string_builder sb(options);
  sb.append(x);
  std::string_view result = sb.str();
  return std::string(result.data(), result.size());
}

#if SIMDJSON_EXCEPTIONS
template <class T>
std::string fractured_json(simdjson_result<T> x) {
  if (x.error()) {
    throw simdjson_error(x.error());
  }
  return fractured_json(x.value());
}

template <class T>
std::string fractured_json(simdjson_result<T> x, const fractured_json_options& options) {
  if (x.error()) {
    throw simdjson_error(x.error());
  }
  return fractured_json(x.value(), options);
}
#endif

// Explicit template instantiations for common types
template std::string fractured_json(dom::element x);
template std::string fractured_json(dom::element x, const fractured_json_options& options);
template std::string fractured_json(dom::array x);
template std::string fractured_json(dom::array x, const fractured_json_options& options);
template std::string fractured_json(dom::object x);
template std::string fractured_json(dom::object x, const fractured_json_options& options);

#if SIMDJSON_EXCEPTIONS
template std::string fractured_json(simdjson_result<dom::element> x);
template std::string fractured_json(simdjson_result<dom::element> x, const fractured_json_options& options);
#endif

//
// String-based API for formatting any JSON string
//

inline std::string fractured_json_string(std::string_view json_str) {
  return fractured_json_string(json_str, fractured_json_options{});
}

inline std::string fractured_json_string(std::string_view json_str,
                                          const fractured_json_options& options) {
  // Parse the JSON string
  dom::parser parser;
  dom::element doc;
  // Need to pad the string for simdjson
  auto padded = padded_string(json_str);
  auto error = parser.parse(padded).get(doc);
  if (error) {
    // If parsing fails, return the original string
    return std::string(json_str);
  }
  return fractured_json(doc, options);
}

} // namespace simdjson

#endif // SIMDJSON_DOM_FRACTURED_JSON_INL_H
