#ifndef SIMDJSON_ERROR_H
#define SIMDJSON_ERROR_H

#include <string>

namespace simdjson {

/**
 * An error somewhere in simdjson.
 */
struct error_code {
public:
  constexpr error_code &operator=(const error_code &other) noexcept { code = other.code; return *this; }

  /** Get the error message for this error code */
  inline const std::string &error_message() const noexcept;

  /** Whether this is successful or not */
  constexpr operator bool() const noexcept;
  /** Whether this is an error or not */
  constexpr bool operator!() const noexcept;

  /** Whether these codes represent the same error */
  constexpr bool operator==(const error_code &other) const noexcept;
  /** Whether these codes represent different errors */
  constexpr bool operator!=(const error_code &other) const noexcept;

  /** @deprecated An integer representing the error code */
  constexpr explicit operator int() const noexcept;

  /** @private */
  constexpr explicit error_code(int _code) noexcept : code{_code} {}

  /** Initialize this error_code to UNINITIALIZED */
  constexpr error_code() noexcept;

private:

  int code;

  friend const std::string &error_message(int error) noexcept;
};

constexpr const error_code SUCCESS                  (0);  ///< No errors
constexpr const error_code SUCCESS_AND_HAS_MORE     (1);  ///< No errors and buffer still has more data
constexpr const error_code CAPACITY                 (2);  ///< This parser can't support a document that big
constexpr const error_code MEMALLOC                 (3);  ///< Error allocating memory, most likely out of memory
constexpr const error_code TAPE_ERROR               (4);  ///< Something went wrong while writing to the tape (stage 2), this is a generic error
constexpr const error_code DEPTH_ERROR              (5);  ///< Your document exceeds the user-specified depth limitation
constexpr const error_code STRING_ERROR             (6);  ///< Problem while parsing a string
constexpr const error_code T_ATOM_ERROR             (7);  ///< Problem while parsing an atom starting with the letter 't'
constexpr const error_code F_ATOM_ERROR             (8);  ///< Problem while parsing an atom starting with the letter 'f'
constexpr const error_code N_ATOM_ERROR             (9);  ///< Problem while parsing an atom starting with the letter 'n'
constexpr const error_code NUMBER_ERROR             (10); ///< Problem while parsing a number
constexpr const error_code UTF8_ERROR               (11); ///< the input is not valid UTF-8
constexpr const error_code UNINITIALIZED            (12); ///< unknown error, or uninitialized document
constexpr const error_code EMPTY                    (13); ///< no structural element found
constexpr const error_code UNESCAPED_CHARS          (14); ///< found unescaped characters in a string.
constexpr const error_code UNCLOSED_STRING          (15); ///< missing quote at the end
constexpr const error_code UNSUPPORTED_ARCHITECTURE (16); ///< unsupported architecture
constexpr const error_code INCORRECT_TYPE           (17); ///< JSON element has a different type than user expected
constexpr const error_code NUMBER_OUT_OF_RANGE      (18); ///< JSON number does not fit in 64 bits
constexpr const error_code NO_SUCH_FIELD            (19); ///< JSON field not found in object
constexpr const error_code UNEXPECTED_ERROR         (20); ///< indicative of a bug in simdjson


/**
 * Exception thrown when there is a simdjson error.
 */
class simdjson_error : std::exception {
public:
  /**
   * Create a simdjson_error exception with the given code.
   *
   * @param code The error code.
   */
  inline simdjson_error(error_code code) noexcept;

  /** The error_code for this error. */
  constexpr error_code code() const noexcept;

  /** Get the error message for this error. */
  inline const char *what() const noexcept;

  /**
   * Whether this error has the given error_code.
   *
   * @param other The code to compare against.
   */
  constexpr bool operator==(const error_code &other) const noexcept;

  /**
   * Whether this error has a different error code than given.
   *
   * @param other The code to compare against.
   */
  constexpr bool operator!=(const error_code &other) const noexcept;

  /** Get the error message for this error */
  inline const std::string & error_message() const noexcept;

private:
  const error_code _code;
};

/** @deprecated */
constexpr bool operator==(const error_code &a, int b) noexcept;
/** @deprecated */
constexpr bool operator!=(const error_code &a, int b) noexcept;
/** @deprecated */
constexpr bool operator==(int a, const error_code &b) noexcept;
/** @deprecated */
constexpr bool operator!=(int a, const error_code &b) noexcept;
/** Print the error message */
inline std::ostream& operator<<(std::ostream& o, const error_code &error) noexcept;
inline std::ostream& operator<<(std::ostream& o, const simdjson_error &error) noexcept;

// TODO these are deprecated, remove
/** @deprecated Use error_code instead */
using ErrorValues = error_code;
/** @deprecated use error_code::error_message() instead */
inline const std::string &error_message(int error) noexcept;

} // namespace simdjson

#endif // SIMDJSON_ERROR_H
