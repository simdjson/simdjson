#ifndef SIMDJSON_ERROR_H
#define SIMDJSON_ERROR_H

#include "simdjson/common_defs.h"
#include <string>
#include <utility>

namespace simdjson {

/**
 * All possible errors returned by simdjson.
 */
enum error_code {
  SUCCESS = 0,              ///< No error
  SUCCESS_AND_HAS_MORE,     ///< No error and buffer still has more data
  CAPACITY,                 ///< This parser can't support a document that big
  MEMALLOC,                 ///< Error allocating memory, most likely out of memory
  TAPE_ERROR,               ///< Something went wrong while writing to the tape (stage 2), this is a generic error
  DEPTH_ERROR,              ///< Your document exceeds the user-specified depth limitation
  STRING_ERROR,             ///< Problem while parsing a string
  T_ATOM_ERROR,             ///< Problem while parsing an atom starting with the letter 't'
  F_ATOM_ERROR,             ///< Problem while parsing an atom starting with the letter 'f'
  N_ATOM_ERROR,             ///< Problem while parsing an atom starting with the letter 'n'
  NUMBER_ERROR,             ///< Problem while parsing a number
  UTF8_ERROR,               ///< the input is not valid UTF-8
  UNINITIALIZED,            ///< unknown error, or uninitialized document
  EMPTY,                    ///< no structural element found
  UNESCAPED_CHARS,          ///< found unescaped characters in a string.
  UNCLOSED_STRING,          ///< missing quote at the end
  UNSUPPORTED_ARCHITECTURE, ///< unsupported architecture
  INCORRECT_TYPE,           ///< JSON element has a different type than user expected
  NUMBER_OUT_OF_RANGE,      ///< JSON number does not fit in 64 bits
  NO_SUCH_FIELD,            ///< JSON field not found in object
  IO_ERROR,                 ///< Error reading a file
  UNEXPECTED_ERROR,         ///< indicative of a bug in simdjson
  /** @private Number of error codes */
  NUM_ERROR_CODES
};

/**
 * Get the error message for the given error code.
 *
 *   auto [doc, error] = document::parse("foo");
 *   if (error) { printf("Error: %s\n", error_message(error)); }
 *
 * @return The error message.
 */
inline const char *error_message(error_code error) noexcept;

/**
 * Write the error message to the output stream
 */
inline std::ostream& operator<<(std::ostream& out, error_code error) noexcept;

/**
 * Exception thrown when an exception-supporting simdjson method is called
 */
struct simdjson_error : public std::exception {
  /**
   * Create an exception from a simdjson error code.
   * @param error The error code
   */
  simdjson_error(error_code error) noexcept : _error{error} { }
  /** The error message */
  const char *what() const noexcept { return error_message(error()); }
  /** The error code */
  error_code error() const noexcept { return _error; }
private:
  /** The error code that was used */
  error_code _error;
};

/**
 * The result of a simd operation that could fail.
 *
 * Gives the option of reading error codes, or throwing an exception by casting to the desired result.
 */
template<typename T>
struct simdjson_result : public std::pair<T, error_code> {

  /**
  * Returns a reference to the base class (std::pair). This is
  * syntaxic sugar to implement a workaround for libc++,
  * where std:tie(a,b) = simdjson_result instance does not work
  * because libc++ expects exactly either an std::pair or std::tuple,
  * and excludes derived classes/struct. It will happily accept
  * a reference to std::pair pair which this function provides.
  * So you can do std::tie(a,b) = x.ref() where x is of type
  * simdjson_result in a portable manner (including libc++).
  * The alternative is to do std::tie(a,b) =
  * static_cast<std::pair<T, error_code>&>(x) which is obviously
  * undesirable.
  * Note that libc++ is the default standard library under macOS/Xcode.
  * We have to worry about this, but users who do not need to
  * support libc++ can omit the ref call.
  * See https://github.com/simdjson/simdjson/issues/578
  */
  std::pair<T, error_code>& ref() {
    return static_cast<std::pair<T, error_code>&>(*this);
  }

  /**
   * The error.
   */
  error_code error() const { return this->second; }

#if SIMDJSON_EXCEPTIONS

  /**
   * The value of the function.
   *
   * @throw simdjson_error if there was an error.
   */
  T get() noexcept(false) {
    if (error()) { throw simdjson_error(error()); }
    return this->first;
  };

  /**
   * Cast to the value (will throw on error).
   *
   * @throw simdjson_error if there was an error.
   */
  operator T() noexcept(false) { return get(); }

#endif // SIMDJSON_EXCEPTIONS

  /**
   * Create a new error result.
   */
  simdjson_result(error_code _error) noexcept : std::pair<T, error_code>({}, _error) {}

  /**
   * Create a new successful result.
   */
  simdjson_result(T _value) noexcept : std::pair<T, error_code>(_value, SUCCESS) {}

  /**
   * Create a new result with both things (use if you don't want to branch when creating the result).
   */
  simdjson_result(T value, error_code error) noexcept : std::pair<T, error_code>(value, error) {}
};

/**
 * The result of a simd operation that could fail.
 *
 * This class is for values that must be *moved*, like padded_string and document.
 *
 * Gives the option of reading error codes, or throwing an exception by casting to the desired result.
 */
template<typename T>
struct simdjson_move_result : std::pair<T, error_code> {

  /**
  * Returns a reference to the base class (std::pair). This is
  * syntaxic sugar to implement a workaround for libc++,
  * where std:tie(a,b) = simdjson_result instance does not work
  * because libc++ expects exactly either an std::pair or std::tuple,
  * and excludes derived classes/struct. It will happily accept
  * a reference to std::pair pair which this function provides.
  * So you can do std::tie(a,b) = x.ref() where x is of type
  * simdjson_move_result in a portable manner (including libc++).
  * The alternative is to do std::tie(a,b) =
  * static_cast<std::pair<T, error_code>&>(x) which is obviously
  * undesirable.
  * Note that libc++ is the default standard library under macOS/Xcode.
  * We have to worry about this, but users who do not need to
  * support libc++ can omit the ref call.
  * See https://github.com/simdjson/simdjson/issues/578
  */
  std::pair<T, error_code>& ref() {
    return static_cast<std::pair<T, error_code>&>(*this);
  }

  /**
   * The error.
   */
  error_code error() const { return this->second; }

#if SIMDJSON_EXCEPTIONS

  /**
   * The value of the function.
   *
   * @throw simdjson_error if there was an error.
   */
  T move() noexcept(false) {
    if (error()) { throw simdjson_error(error()); }
    return std::move(this->first);
  };

  /**
   * Cast to the value (will throw on error).
   *
   * @throw simdjson_error if there was an error.
   */
  operator T() noexcept(false) { return move(); }

#endif

  /**
   * Create a new error result.
   */
  simdjson_move_result(error_code error) noexcept : std::pair<T, error_code>(T(), error) {}

  /**
   * Create a new successful result.
   */
  simdjson_move_result(T value) noexcept : std::pair<T, error_code>(std::move(value), SUCCESS) {}

  /**
   * Create a new result with both things (use if you don't want to branch when creating the result).
   */
  simdjson_move_result(T value, error_code error) noexcept : std::pair<T, error_code>(std::move(value), error) {}
};

/**
 * @deprecated This is an alias and will be removed, use error_code instead
 */
using ErrorValues = error_code;

/**
 * @deprecated Error codes should be stored and returned as `error_code`, use `error_message()` instead.
 */
inline const std::string &error_message(int error) noexcept;

} // namespace simdjson

#endif // SIMDJSON_ERROR_H
