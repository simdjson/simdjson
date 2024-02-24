/**
 * @file Base declarations for all simdjson headers
 * @private
 */
#ifndef SIMDJSON_BASE_H
#define SIMDJSON_BASE_H

#include "simdjson/common_defs.h"
#include "simdjson/compiler_check.h"
#include "simdjson/error.h"
#include "simdjson/portability.h"

/**
 * @brief The top level simdjson namespace, containing everything the library provides.
 */
namespace simdjson {

SIMDJSON_PUSH_DISABLE_UNUSED_WARNINGS

/** The maximum document size supported by simdjson. */
constexpr size_t SIMDJSON_MAXSIZE_BYTES = 0xFFFFFFFF;

/**
 * The amount of padding needed in a buffer to parse JSON.
 *
 * The input buf should be readable up to buf + SIMDJSON_PADDING
 * this is a stopgap; there should be a better description of the
 * main loop and its behavior that abstracts over this
 * See https://github.com/simdjson/simdjson/issues/174
 */
constexpr size_t SIMDJSON_PADDING = 64;

/**
 * By default, simdjson supports this many nested objects and arrays.
 *
 * This is the default for parser::max_depth().
 */
constexpr size_t DEFAULT_MAX_DEPTH = 1024;

SIMDJSON_POP_DISABLE_UNUSED_WARNINGS

class implementation;
struct padded_string;
class padded_string_view;
enum class stage1_mode;

/**
 * The string wrapping type used to represent an integer that does not fit in 64 bits.
 *
 * The value is not '\0' terminated.
 */
struct bigint_t : public std::string_view {
  bigint_t() {}
  bigint_t(const uint8_t *src, size_t len) : std::string_view(reinterpret_cast<const char*>(src), len) {}
  bigint_t(const char    *src, size_t len) : std::string_view(src, len) {}

  bool operator==(const std::string_view& rhs) const { return rhs == *reinterpret_cast<const std::string_view*>(this); }
  bool operator==(const bigint_t& rhs) const {
    return reinterpret_cast<const std::string_view&>(rhs) == *reinterpret_cast<const std::string_view*>(this);
  }
};

namespace internal {

template<typename T>
class atomic_ptr;
class dom_parser_implementation;
class escape_json_string;
class tape_ref;
struct value128;
enum class tape_type;

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_BASE_H
