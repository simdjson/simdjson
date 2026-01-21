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
#include <set>

namespace simdjson {
namespace internal {

/**
 * Layout mode for fractured JSON formatting.
 */
enum class layout_mode {
  INLINE,              // Single line: [1, 2, 3] or {"a": 1}
  COMPACT_MULTILINE,   // Multiple items per line with breaks
  TABLE,               // Tabular format for arrays of similar objects
  EXPANDED             // Traditional multi-line with indentation
};

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

  /** For uniform arrays of objects: the common keys */
  std::vector<std::string> common_keys{};

  /** Recommended layout mode based on analysis */
  layout_mode recommended_layout = layout_mode::EXPANDED;

  /** Child metrics for arrays and objects (in order of iteration) */
  std::vector<element_metrics> children{};
};

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

private:
  const fractured_json_options* current_opts_ = nullptr;

  /** Recursive analysis implementation */
  element_metrics analyze_element(const dom::element& elem, size_t depth);

  /** Analyze scalar values (strings, numbers, booleans, null) */
  element_metrics analyze_scalar(const dom::element& elem);

  /** Analyze an array element */
  element_metrics analyze_array(const dom::array& arr, size_t depth);

  /** Analyze an object element */
  element_metrics analyze_object(const dom::object& obj, size_t depth);

  /** Estimate inline length for a string (including quotes and escaping) */
  size_t estimate_string_length(std::string_view s) const;

  /** Estimate inline length for a number */
  size_t estimate_number_length(double d) const;
  size_t estimate_number_length(int64_t i) const;
  size_t estimate_number_length(uint64_t u) const;

  /**
   * Check if an array contains uniform objects suitable for table formatting.
   * @param arr The array to check
   * @param common_keys Output: keys common to all objects
   * @return true if the array is suitable for table formatting
   */
  bool check_array_uniformity(const dom::array& arr,
                               std::vector<std::string>& common_keys) const;

  /**
   * Compute similarity between two objects.
   * @return Fraction of keys that are common (0.0 to 1.0)
   */
  double compute_object_similarity(const dom::object& a,
                                   const dom::object& b) const;

  /**
   * Decide the recommended layout mode based on metrics and options.
   */
  layout_mode decide_layout(const element_metrics& metrics,
                            size_t depth,
                            size_t available_width) const;
};

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_JSON_STRUCTURE_ANALYZER_H
