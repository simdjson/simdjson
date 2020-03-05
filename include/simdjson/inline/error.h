#ifndef SIMDJSON_INLINE_ERROR_H
#define SIMDJSON_INLINE_ERROR_H

#include <map>
#include <string>

namespace simdjson {

namespace internal {

inline std::string error_messages[] {
  "No error", // SUCCESS
  "No error and buffer still has more data", // SUCCESS_AND_HAS_MORE
  "This parser can't support a document that big", // CAPACITY
  "Error allocating memory, we're most likely out of memory", // MEMALLOC
  "Something went wrong while writing to the tape", // TAPE_ERROR
  "The JSON document was too deep (too many nested objects and arrays)", // DEPTH_ERROR
  "Problem while parsing a string", // STRING_ERROR
  "Problem while parsing an atom starting with the letter 't'", // T_ATOM_ERROR
  "Problem while parsing an atom starting with the letter 'f'", // F_ATOM_ERROR
  "Problem while parsing an atom starting with the letter 'n'", // N_ATOM_ERROR
  "Problem while parsing a number", // NUMBER_ERROR
  "The input is not valid UTF-8", // UTF8_ERROR
  "Uninitialized", // UNINITIALIZED
  "Empty: no JSON found", // EMPTY
  "Within strings, some characters must be escaped, we found unescaped characters", // UNESCAPED_CHARS
  "A string is opened, but never closed.", // UNCLOSED_STRING
  "simdjson does not have an implementation supported by this CPU architecture (perhaps it's a non-SIMD CPU?).", // UNSUPPORTED_ARCHITECTURE
  "The JSON element does not have the requested type.", // INCORRECT_TYPE
  "The JSON number is too large or too small to fit within the requested type.", // NUMBER_OUT_OF_RANGE
  "The JSON field referenced does not exist in this object.", // NO_SUCH_FIELD
  "Unexpected error, consider reporting this problem as you may have found a bug in simdjson" // UNEXPECTED_ERROR
};

}

//
// error_code inline implementation
//
inline const std::string & error_code::error_message() const noexcept {
  // It's impossible to construct an error_code without it existing in the array, so this is always safe
  return internal::error_messages[code];
}
constexpr error_code::operator bool() const noexcept { return code; }
constexpr bool error_code::operator!() const noexcept { return !code; }
constexpr bool error_code::operator==(const error_code &other) const noexcept { return code == other.code; }
constexpr bool error_code::operator!=(const error_code &other) const noexcept { return code != other.code; }
constexpr error_code::operator int() const noexcept { return code; }
constexpr error_code::error_code() noexcept : code{UNINITIALIZED.code} {}

constexpr bool operator==(const error_code &a, int b) noexcept { return int(a) == b; }
constexpr bool operator!=(const error_code &a, int b) noexcept { return int(a) != b; }
constexpr bool operator==(int a, const error_code &b) noexcept { return a == int(b); }
constexpr bool operator!=(int a, const error_code &b) noexcept { return a != int(b); }
inline std::ostream& operator<<(std::ostream& o, const error_code &error) noexcept { return o << error.error_message(); }

//
// simdjson_error inline implementation
//

inline simdjson_error::simdjson_error(error_code code) noexcept : _code{code} {}
constexpr error_code simdjson_error::code() const noexcept { return _code; }
inline const char *simdjson_error::what() const noexcept { return error_message().c_str(); }
constexpr bool simdjson_error::operator==(const error_code &other) const noexcept { return code() == other; }
constexpr bool simdjson_error::operator!=(const error_code &other) const noexcept { return code() == other; }
inline const std::string & simdjson_error::error_message() const noexcept {
  return code().error_message();
}
inline std::ostream& operator<<(std::ostream& o, const simdjson_error &error) noexcept { return o << error.code(); }

inline const std::string &error_message(int error) noexcept {
  if (error < 0 || error >= int(sizeof(internal::error_messages) / sizeof(std::string))) {
    return UNEXPECTED_ERROR.error_message();
  }
  return error_code(error).error_message();
}

} // namespace simdjson

#endif // SIMDJSON_INLINE_ERROR_H
