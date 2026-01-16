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

inline element_metrics structure_analyzer::analyze_element(const dom::element& elem, size_t depth) {
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

inline element_metrics structure_analyzer::analyze_scalar(const dom::element& elem) {
  element_metrics metrics;
  metrics.complexity = 0;
  metrics.child_count = 0;
  metrics.can_inline = true;
  metrics.recommended_layout = layout_mode::INLINE;

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
                                                          size_t depth) {
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

  // Check if can inline
  metrics.can_inline = (metrics.complexity <= current_opts_->max_inline_complexity) &&
                       (metrics.estimated_inline_len <= current_opts_->max_inline_length);

  // Check for uniform array (table formatting)
  if (current_opts_->enable_table_format &&
      metrics.child_count >= current_opts_->min_table_rows) {
    metrics.is_uniform_array = check_array_uniformity(arr, metrics.common_keys);
  }

  // Decide layout
  if (metrics.child_count == 0) {
    metrics.recommended_layout = layout_mode::INLINE;
  } else if (metrics.can_inline) {
    metrics.recommended_layout = layout_mode::INLINE;
  } else if (metrics.is_uniform_array && !metrics.common_keys.empty()) {
    metrics.recommended_layout = layout_mode::TABLE;
  } else if (current_opts_->enable_compact_multiline &&
             max_child_complexity <= current_opts_->max_compact_array_complexity) {
    metrics.recommended_layout = layout_mode::COMPACT_MULTILINE;
  } else {
    metrics.recommended_layout = layout_mode::EXPANDED;
  }

  return metrics;
}

inline element_metrics structure_analyzer::analyze_object(const dom::object& obj,
                                                           size_t depth) {
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

  metrics.can_inline = (metrics.complexity <= current_opts_->max_inline_complexity) &&
                       (metrics.estimated_inline_len <= current_opts_->max_inline_length);

  // Objects use inline or expanded (no table/compact for objects)
  if (metrics.child_count == 0 || metrics.can_inline) {
    metrics.recommended_layout = layout_mode::INLINE;
  } else {
    metrics.recommended_layout = layout_mode::EXPANDED;
  }

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
  if (std::isnan(d) || std::isinf(d)) {
    return 4; // "null" for invalid numbers
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

inline bool structure_analyzer::check_array_uniformity(const dom::array& arr,
                                                        std::vector<std::string>& common_keys) const {
  common_keys.clear();

  std::set<std::string> shared_keys;
  dom::object first_obj;
  bool have_first = false;
  size_t object_count = 0;

  for (dom::element elem : arr) {
    if (elem.type() != dom::element_type::OBJECT) {
      return false; // Not all elements are objects
    }

    dom::object obj;
    if (elem.get_object().get(obj) != SUCCESS) {
      return false;
    }

    std::set<std::string> current_keys;
    for (dom::key_value_pair field : obj) {
      current_keys.insert(std::string(field.key));
    }

    if (!have_first) {
      shared_keys = current_keys;
      first_obj = obj;
      have_first = true;
    } else {
      // Check similarity threshold against the first object
      double similarity = compute_object_similarity(first_obj, obj);
      if (similarity < current_opts_->table_similarity_threshold) {
        return false; // Objects are too dissimilar for table format
      }

      // Intersect with current keys
      std::set<std::string> intersection;
      std::set_intersection(shared_keys.begin(), shared_keys.end(),
                            current_keys.begin(), current_keys.end(),
                            std::inserter(intersection, intersection.begin()));
      shared_keys = intersection;
    }

    object_count++;
  }

  if (object_count < current_opts_->min_table_rows) {
    return false;
  }

  // Require at least one common key for table formatting
  if (shared_keys.empty()) {
    return false;
  }

  common_keys.assign(shared_keys.begin(), shared_keys.end());
  return true;
}

inline double structure_analyzer::compute_object_similarity(const dom::object& a,
                                                             const dom::object& b) const {
  std::set<std::string> keys_a, keys_b;
  for (dom::key_value_pair field : a) {
    keys_a.insert(std::string(field.key));
  }
  for (dom::key_value_pair field : b) {
    keys_b.insert(std::string(field.key));
  }

  std::set<std::string> intersection;
  std::set_intersection(keys_a.begin(), keys_a.end(),
                        keys_b.begin(), keys_b.end(),
                        std::inserter(intersection, intersection.begin()));

  std::set<std::string> union_set;
  std::set_union(keys_a.begin(), keys_a.end(),
                 keys_b.begin(), keys_b.end(),
                 std::inserter(union_set, union_set.begin()));

  if (union_set.empty()) return 1.0;
  return static_cast<double>(intersection.size()) / static_cast<double>(union_set.size());
}

inline layout_mode structure_analyzer::decide_layout(const element_metrics& metrics,
                                                      size_t depth,
                                                      size_t available_width) const {
  if (metrics.child_count == 0) {
    return layout_mode::INLINE;
  }

  // Check inline feasibility
  size_t indent_width = depth * current_opts_->indent_spaces;
  if (metrics.can_inline &&
      metrics.estimated_inline_len + indent_width <= available_width) {
    return layout_mode::INLINE;
  }

  // Check table mode
  if (metrics.is_uniform_array && !metrics.common_keys.empty()) {
    return layout_mode::TABLE;
  }

  // Check compact multiline
  if (current_opts_->enable_compact_multiline &&
      metrics.complexity <= current_opts_->max_compact_array_complexity + 1) {
    return layout_mode::COMPACT_MULTILINE;
  }

  return layout_mode::EXPANDED;
}

//
// Fractured Formatter Implementation
//

inline fractured_formatter::fractured_formatter(const fractured_json_options& opts)
    : options_(opts), column_widths_{} {}

simdjson_inline void fractured_formatter::print_newline() {
  if (current_layout_ == layout_mode::INLINE) {
    return; // No newlines in inline mode
  }
  one_char('\n');
  current_line_length_ = 0;
}

simdjson_inline void fractured_formatter::print_indents(size_t depth) {
  if (current_layout_ == layout_mode::INLINE) {
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

inline void fractured_formatter::set_depth(size_t depth) {
  current_depth_ = depth;
}

inline size_t fractured_formatter::get_depth() const {
  return current_depth_;
}

inline void fractured_formatter::track_line_length(size_t chars) {
  current_line_length_ += chars;
}

inline void fractured_formatter::reset_line_length() {
  current_line_length_ = 0;
}

inline size_t fractured_formatter::get_line_length() const {
  return current_line_length_;
}

inline bool fractured_formatter::should_break_line(size_t upcoming_length) const {
  return (current_line_length_ + upcoming_length) > options_.max_total_line_length;
}

inline const fractured_json_options& fractured_formatter::options() const {
  return options_;
}

inline void fractured_formatter::begin_table_row() {
  in_table_mode_ = true;
  current_column_ = 0;
}

inline void fractured_formatter::end_table_row() {
  in_table_mode_ = false;
  current_column_ = 0;
}

inline void fractured_formatter::set_column_widths(const std::vector<size_t>& widths) {
  column_widths_ = widths;
}

inline size_t fractured_formatter::get_column_index() const {
  return current_column_;
}

inline void fractured_formatter::next_column() {
  current_column_++;
}

inline void fractured_formatter::align_to_column_width(size_t actual_width) {
  if (current_column_ < column_widths_.size()) {
    size_t target_width = column_widths_[current_column_];
    while (actual_width < target_width) {
      one_char(' ');
      actual_width++;
      current_line_length_++;
    }
  }
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
                                                       size_t depth) {
  switch (elem.type()) {
    case dom::element_type::ARRAY: {
      dom::array arr;
      if (elem.get_array().get(arr) == SUCCESS) {
        format_array(arr, metrics, depth);
      }
      break;
    }
    case dom::element_type::OBJECT: {
      dom::object obj;
      if (elem.get_object().get(obj) == SUCCESS) {
        format_object(obj, metrics, depth);
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
                                                    size_t depth) {
  switch (metrics.recommended_layout) {
    case layout_mode::INLINE:
      format_array_inline(arr, metrics);
      break;
    case layout_mode::COMPACT_MULTILINE:
      format_array_compact_multiline(arr, metrics, depth);
      break;
    case layout_mode::TABLE:
      format_array_as_table(arr, metrics, depth);
      break;
    case layout_mode::EXPANDED:
    default:
      format_array_expanded(arr, metrics, depth);
      break;
  }
}

inline void fractured_string_builder::format_array_inline(const dom::array& arr,
                                                            const element_metrics& metrics) {
  layout_mode prev_layout = format_.get_layout_mode();
  format_.set_layout_mode(layout_mode::INLINE);

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
    } else if (options_.simple_bracket_padding) {
      format_.print_space();
    }
    first = false;
    const element_metrics& child_metrics = (child_idx < metrics.children.size())
        ? metrics.children[child_idx] : element_metrics{};
    format_element(elem, child_metrics, 0);
    child_idx++;
  }

  if (options_.simple_bracket_padding && !empty) {
    format_.print_space();
  }
  format_.end_array();

  format_.set_layout_mode(prev_layout);
}

inline void fractured_string_builder::format_array_compact_multiline(const dom::array& arr,
                                                                       const element_metrics& metrics,
                                                                       size_t depth) {
  format_.start_array();
  format_.print_newline();
  format_.print_indents(depth + 1);

  size_t items_on_line = 0;
  bool first = true;
  size_t child_idx = 0;

  for (dom::element elem : arr) {
    if (!first) {
      format_.comma();

      // Check if we should break to new line
      if (items_on_line >= options_.max_items_per_line ||
          format_.should_break_line(20)) { // 20 is rough estimate for next item
        format_.print_newline();
        format_.print_indents(depth + 1);
        items_on_line = 0;
      } else if (options_.comma_padding) {
        format_.print_space();
      }
    }
    first = false;

    // Format element inline
    layout_mode prev_layout = format_.get_layout_mode();
    format_.set_layout_mode(layout_mode::INLINE);
    const element_metrics& child_metrics = (child_idx < metrics.children.size())
        ? metrics.children[child_idx] : element_metrics{};
    format_element(elem, child_metrics, depth + 1);
    format_.set_layout_mode(prev_layout);

    items_on_line++;
    child_idx++;
  }

  format_.print_newline();
  format_.print_indents(depth);
  format_.end_array();
}

inline void fractured_string_builder::format_array_as_table(const dom::array& arr,
                                                             const element_metrics& metrics,
                                                             size_t depth) {
  const std::vector<std::string>& columns = metrics.common_keys;
  if (columns.empty()) {
    format_array_expanded(arr, metrics, depth);
    return;
  }

  // Calculate column widths for alignment
  std::vector<size_t> col_widths = calculate_column_widths(arr, columns);
  format_.set_column_widths(col_widths);

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
    format_.begin_table_row();

    // Format object as inline with aligned columns
    dom::object obj;
    if (elem.get_object().get(obj) != SUCCESS) {
      child_idx++;
      continue;
    }

    // Get child metrics for this row (object)
    const element_metrics& row_metrics = (child_idx < metrics.children.size())
        ? metrics.children[child_idx] : element_metrics{};

    format_.start_object();
    if (options_.simple_bracket_padding) {
      format_.print_space();
    }

    bool first_col = true;
    const size_t num_columns = columns.size();

    for (size_t col_idx = 0; col_idx < num_columns; col_idx++) {
      const std::string& key = columns[col_idx];
      const bool is_last_col = (col_idx == num_columns - 1);

      if (!first_col) {
        format_.comma();
        if (options_.comma_padding) {
          format_.print_space();
        }
      }
      first_col = false;

      // Write key
      format_.key(key);
      if (options_.colon_padding) {
        format_.print_space();
      }

      // Find the value for this key and its metrics
      dom::element value;
      bool found = false;
      size_t field_idx = 0;
      for (dom::key_value_pair field : obj) {
        if (field.key == key) {
          value = field.value;
          found = true;
          break;
        }
        field_idx++;
      }

      // Write value
      if (found) {
        layout_mode prev_layout = format_.get_layout_mode();
        format_.set_layout_mode(layout_mode::INLINE);
        const element_metrics& value_metrics = (field_idx < row_metrics.children.size())
            ? row_metrics.children[field_idx] : element_metrics{};
        format_element(value, value_metrics, depth + 1);
        format_.set_layout_mode(prev_layout);
      } else {
        format_.null_atom();
      }

      // Only pad non-last columns to align values across rows
      if (!is_last_col) {
        size_t actual_len = found ? measure_value_length(value) : 4; // 4 for "null"
        size_t target_width = col_widths[col_idx];
        while (actual_len < target_width) {
          format_.one_char(' ');
          actual_len++;
        }
      }

      format_.next_column();
    }

    if (options_.simple_bracket_padding) {
      format_.print_space();
    }
    format_.end_object();
    format_.end_table_row();
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
    const element_metrics& child_metrics = (child_idx < metrics.children.size())
        ? metrics.children[child_idx] : element_metrics{};
    format_element(elem, child_metrics, depth + 1);
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
                                                     size_t depth) {
  if (metrics.recommended_layout == layout_mode::INLINE || metrics.can_inline) {
    format_object_inline(obj, metrics);
  } else {
    format_object_expanded(obj, metrics, depth);
  }
}

inline void fractured_string_builder::format_object_inline(const dom::object& obj,
                                                             const element_metrics& metrics) {
  layout_mode prev_layout = format_.get_layout_mode();
  format_.set_layout_mode(layout_mode::INLINE);

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
    } else if (options_.simple_bracket_padding) {
      format_.print_space();
    }
    first = false;

    format_.key(field.key);
    if (options_.colon_padding) {
      format_.print_space();
    }
    const element_metrics& child_metrics = (child_idx < metrics.children.size())
        ? metrics.children[child_idx] : element_metrics{};
    format_element(field.value, child_metrics, 0);
    child_idx++;
  }

  if (options_.simple_bracket_padding && !empty) {
    format_.print_space();
  }
  format_.end_object();

  format_.set_layout_mode(prev_layout);
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
    const element_metrics& child_metrics = (child_idx < metrics.children.size())
        ? metrics.children[child_idx] : element_metrics{};
    format_element(field.value, child_metrics, depth + 1);
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

inline size_t fractured_string_builder::measure_value_length(const dom::element& elem) const {
  switch (elem.type()) {
    case dom::element_type::STRING: {
      std::string_view str;
      if (elem.get_string().get(str) == SUCCESS) {
        // Count actual escaped length
        size_t len = 2; // quotes
        for (char c : str) {
          if (c == '"' || c == '\\' || static_cast<unsigned char>(c) < 32) {
            len += 2; // escape sequence
          } else {
            len += 1;
          }
        }
        return len;
      }
      return 2;
    }
    case dom::element_type::INT64: {
      int64_t val;
      if (elem.get_int64().get(val) == SUCCESS) {
        if (val == 0) return 1;
        // Handle INT64_MIN specially to avoid overflow when negating
        if (val == INT64_MIN) return 20; // "-9223372036854775808" is 20 characters
        size_t len = (val < 0) ? 1 : 0;
        int64_t abs_val = (val < 0) ? -val : val;
        while (abs_val > 0) { len++; abs_val /= 10; }
        return len;
      }
      return 1;
    }
    case dom::element_type::UINT64: {
      uint64_t val;
      if (elem.get_uint64().get(val) == SUCCESS) {
        if (val == 0) return 1;
        size_t len = 0;
        while (val > 0) { len++; val /= 10; }
        return len;
      }
      return 1;
    }
    case dom::element_type::DOUBLE: {
      double val;
      if (elem.get_double().get(val) == SUCCESS) {
        char buf[32];
        int len = snprintf(buf, sizeof(buf), "%.17g", val);
        return len > 0 ? static_cast<size_t>(len) : 1;
      }
      return 1;
    }
    case dom::element_type::BOOL: {
      bool val;
      if (elem.get_bool().get(val) == SUCCESS) {
        return val ? 4 : 5; // "true" or "false"
      }
      return 5;
    }
    case dom::element_type::NULL_VALUE:
      return 4; // "null"
    default:
      return 4;
  }
}

inline std::vector<size_t> fractured_string_builder::calculate_column_widths(
    const dom::array& arr,
    const std::vector<std::string>& columns) const {

  std::vector<size_t> widths(columns.size(), 0);

  for (dom::element elem : arr) {
    dom::object obj;
    if (elem.get_object().get(obj) != SUCCESS) {
      continue;
    }

    for (size_t col_idx = 0; col_idx < columns.size(); col_idx++) {
      const std::string& key = columns[col_idx];

      for (dom::key_value_pair field : obj) {
        if (field.key == key) {
          // Measure actual value length
          size_t len = measure_value_length(field.value);
          widths[col_idx] = (std::max)(widths[col_idx], len);
          break;
        }
      }
    }
  }

  return widths;
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
