#ifndef SIMDJSON_GENERIC_IMPLEMENTATION_SIMDJSON_RESULT_BASE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_IMPLEMENTATION_SIMDJSON_RESULT_BASE_H
#include "simdjson/generic/base.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {

// This is a near copy of include/error.h's implementation_simdjson_result_base, except it doesn't use std::pair
// so we can avoid inlining errors
// TODO reconcile these!
/**
 * The result of a simdjson operation that could fail.
 *
 * Gives the option of reading error codes, or throwing an exception by casting to the desired result.
 *
 * This is a base class for implementations that want to add functions to the result type for
 * chaining.
 *
 * Override like:
 *
 *   struct simdjson_result<T> : public internal::implementation_simdjson_result_base<T> {
 *     simdjson_result() noexcept : internal::implementation_simdjson_result_base<T>() {}
 *     simdjson_result(error_code error) noexcept : internal::implementation_simdjson_result_base<T>(error) {}
 *     simdjson_result(T &&value) noexcept : internal::implementation_simdjson_result_base<T>(std::forward(value)) {}
 *     simdjson_result(T &&value, error_code error) noexcept : internal::implementation_simdjson_result_base<T>(value, error) {}
 *     // Your extra methods here
 *   }
 *
 * Then any method returning simdjson_result<T> will be chainable with your methods.
 */
template<typename T>
struct implementation_simdjson_result_base {

  /**
   * Create a new empty result with error = UNINITIALIZED.
   */
  simdjson_inline implementation_simdjson_result_base() noexcept = default;

  /**
   * Create a new error result.
   */
  simdjson_inline implementation_simdjson_result_base(error_code error) noexcept;

  /**
   * Create a new successful result.
   */
  simdjson_inline implementation_simdjson_result_base(T &&value) noexcept;

  /**
   * Create a new result with both things (use if you don't want to branch when creating the result).
   */
  simdjson_inline implementation_simdjson_result_base(T &&value, error_code error) noexcept;

  /**
   * Move the value and the error to the provided variables.
   *
   * @param value The variable to assign the value to. May not be set if there is an error.
   * @param error The variable to assign the error to. Set to SUCCESS if there is no error.
   */
  simdjson_inline void tie(T &value, error_code &error) && noexcept;

  /**
   * Move the value to the provided variable.
   *
   * @param value The variable to assign the value to. May not be set if there is an error.
   */
  simdjson_inline error_code get(T &value) && noexcept;

  /**
   * The error.
   */
  simdjson_inline error_code error() const noexcept;

#if SIMDJSON_EXCEPTIONS

  /**
   * Get the result value.
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_inline T& value() & noexcept(false);

  /**
   * Take the result value (move it).
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_inline T&& value() && noexcept(false);

  /**
   * Take the result value (move it).
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_inline T&& take_value() && noexcept(false);

  /**
   * Cast to the value (will throw on error).
   *
   * @throw simdjson_error if there was an error.
   */
  simdjson_inline operator T&&() && noexcept(false);


#endif // SIMDJSON_EXCEPTIONS

  /**
   * Get the result value. This function is safe if and only
   * the error() method returns a value that evaluates to false.
   */
  simdjson_inline const T& value_unsafe() const& noexcept;
  /**
   * Get the result value. This function is safe if and only
   * the error() method returns a value that evaluates to false.
   */
  simdjson_inline T& value_unsafe() & noexcept;
  /**
   * Take the result value (move it). This function is safe if and only
   * the error() method returns a value that evaluates to false.
   */
  simdjson_inline T&& value_unsafe() && noexcept;
protected:
  /** users should never directly access first and second. **/
  T first{}; /** Users should never directly access 'first'. **/
  error_code second{UNINITIALIZED}; /** Users should never directly access 'second'. **/
}; // struct implementation_simdjson_result_base

} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_IMPLEMENTATION_SIMDJSON_RESULT_BASE_H