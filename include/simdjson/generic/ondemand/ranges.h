#ifndef SIMDJSON_GENERIC_ONDEMAND_RANGES_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_RANGES_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/array.h"
#include "simdjson/generic/ondemand/array_iterator.h"
#include "simdjson/generic/ondemand/object.h"
#include "simdjson/generic/ondemand/object_iterator.h"
#include "simdjson/generic/ondemand/field.h"
#include "simdjson/generic/ondemand/value.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_SUPPORTS_RANGES

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * A ranges-compatible iterator adapter for JSON arrays.
 *
 * Wraps array_iterator to satisfy std::input_iterator by providing:
 * - const operator* (via mutable internal state)
 * - post-increment operator
 * - iterator_concept tag
 *
 * The mutable approach is standard for single-pass input iterators that
 * read from external sources (similar to std::istream_iterator).
 */
class array_range_iterator {
public:
  using iterator_concept = std::input_iterator_tag;
  using value_type = simdjson_result<value>;
  using reference = simdjson_result<value>;
  using difference_type = std::ptrdiff_t;

  simdjson_inline array_range_iterator() noexcept = default;
  simdjson_inline explicit array_range_iterator(array_iterator iter) noexcept;

  /**
   * Get the current element. Const-qualified for std::indirectly_readable;
   * internally delegates to the mutable wrapped iterator.
   */
  simdjson_inline simdjson_result<value> operator*() const noexcept;
  simdjson_inline array_range_iterator& operator++() noexcept;
SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
  simdjson_inline void operator++(int) noexcept;
SIMDJSON_POP_DISABLE_WARNINGS
  /**
   * Comparison delegates to array_iterator::operator==, which checks
   * whether the underlying parser has finished the array (depth-based).
   */
  simdjson_inline friend bool operator==(const array_range_iterator& a,
                                         const array_range_iterator& b) noexcept {
    return a.iter_ == b.iter_;
  }

private:
  mutable array_iterator iter_{};
};

/**
 * A std::ranges::view over a JSON array.
 *
 * Wraps an ondemand::array and exposes begin()/end() that return
 * array_range_iterator (satisfying std::input_iterator), enabling
 * use with std::views::transform and other range adaptors.
 *
 * If the array's begin() returns an error (only possible under
 * SIMDJSON_DEVELOPMENT_CHECKS), the range will be empty and error()
 * will return the error code.
 *
 * Usage:
 *   ondemand::parser parser;
 *   auto doc = parser.iterate(json);
 *   auto arr = doc.get_array().value();
 *   for (auto elem : ondemand::get_range(arr)) { ... }
 */
class array_range {
public:
  simdjson_inline array_range() noexcept = default;
  simdjson_inline explicit array_range(array& arr) noexcept;

  simdjson_inline array_range_iterator begin() noexcept;
  simdjson_inline array_range_iterator end() noexcept;

  /** Returns SUCCESS if the range was created successfully, or the error code otherwise. */
  simdjson_inline error_code error() const noexcept { return error_; }

private:
  array_iterator begin_{};
  array_iterator end_{};
  error_code error_{SUCCESS};
};

/**
 * A ranges-compatible iterator adapter for JSON objects.
 *
 * Wraps object_iterator to satisfy std::input_iterator, yielding
 * simdjson_result<field> elements (key-value pairs).
 */
class object_range_iterator {
public:
  using iterator_concept = std::input_iterator_tag;
  using value_type = simdjson_result<field>;
  using reference = simdjson_result<field>;
  using difference_type = std::ptrdiff_t;

  simdjson_inline object_range_iterator() noexcept = default;
  simdjson_inline explicit object_range_iterator(object_iterator iter) noexcept;

  simdjson_inline simdjson_result<field> operator*() const noexcept;
  simdjson_inline object_range_iterator& operator++() noexcept;
SIMDJSON_PUSH_DISABLE_ALL_WARNINGS
  simdjson_inline void operator++(int) noexcept;
SIMDJSON_POP_DISABLE_WARNINGS
  simdjson_inline friend bool operator==(const object_range_iterator& a,
                                         const object_range_iterator& b) noexcept {
    return a.iter_ == b.iter_;
  }

private:
  mutable object_iterator iter_{};
};

/**
 * A std::ranges::view over a JSON object.
 *
 * Wraps an ondemand::object and exposes begin()/end() that return
 * object_range_iterator, enabling use with range adaptors.
 *
 * If the object's begin() returns an error, the range will be empty
 * and error() will return the error code.
 */
class object_range {
public:
  simdjson_inline object_range() noexcept = default;
  simdjson_inline explicit object_range(object& obj) noexcept;

  simdjson_inline object_range_iterator begin() noexcept;
  simdjson_inline object_range_iterator end() noexcept;

  /** Returns SUCCESS if the range was created successfully, or the error code otherwise. */
  simdjson_inline error_code error() const noexcept { return error_; }

private:
  object_iterator begin_{};
  object_iterator end_{};
  error_code error_{SUCCESS};
};

/** Get a std::ranges compatible view over a JSON array. */
simdjson_inline array_range get_range(array& arr) noexcept;

/** Get a std::ranges compatible view over a JSON object (key-value pairs). */
simdjson_inline object_range get_key_value_range(object& obj) noexcept;

#if SIMDJSON_EXCEPTIONS
/** Get a std::ranges compatible view, unwrapping the simdjson_result (throws on error). */
simdjson_inline array_range get_range(simdjson_result<array> result);

/** Get a std::ranges compatible view, unwrapping the simdjson_result (throws on error). */
simdjson_inline object_range get_key_value_range(simdjson_result<object> result);
#endif // SIMDJSON_EXCEPTIONS

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace std {
namespace ranges {
template<>
inline constexpr bool enable_view<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array_range> = true;
template<>
inline constexpr bool enable_view<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object_range> = true;
} // namespace ranges
} // namespace std

#endif // SIMDJSON_SUPPORTS_RANGES

#endif // SIMDJSON_GENERIC_ONDEMAND_RANGES_H
