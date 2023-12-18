#ifndef SIMDJSON_GENERIC_ONDEMAND_JSON_TYPE_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_JSON_TYPE_H
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/implementation_simdjson_result_base.h"
#include "simdjson/generic/numberparsing.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

/**
 * The type of a JSON value.
 */
enum class json_type {
    // Start at 1 to catch uninitialized / default values more easily
    array=1, ///< A JSON array   ( [ 1, 2, 3 ... ] )
    object,  ///< A JSON object  ( { "a": 1, "b" 2, ... } )
    number,  ///< A JSON number  ( 1 or -2.3 or 4.5e6 ...)
    string,  ///< A JSON string  ( "a" or "hello world\n" ...)
    boolean, ///< A JSON boolean (true or false)
    null     ///< A JSON null    (null)
};

/**
 * A type representing a JSON number.
 * The design of the struct is deliberately straight-forward. All
 * functions return standard values with no error check.
 */
struct number {

  /**
   * return the automatically determined type of
   * the number: number_type::floating_point_number,
   * number_type::signed_integer or number_type::unsigned_integer.
   *
   *    enum class number_type {
   *        floating_point_number=1, /// a binary64 number
   *        signed_integer,          /// a signed integer that fits in a 64-bit word using two's complement
   *        unsigned_integer         /// a positive integer larger or equal to 1<<63
   *    };
   */
  simdjson_inline ondemand::number_type get_number_type() const noexcept;
  /**
   * return true if the automatically determined type of
   * the number is number_type::unsigned_integer.
   */
  simdjson_inline bool is_uint64() const noexcept;
  /**
   * return the value as a uint64_t, only valid if is_uint64() is true.
   */
  simdjson_inline uint64_t get_uint64() const noexcept;
  simdjson_inline operator uint64_t() const noexcept;

  /**
   * return true if the automatically determined type of
   * the number is number_type::signed_integer.
   */
  simdjson_inline bool is_int64() const noexcept;
  /**
   * return the value as a int64_t, only valid if is_int64() is true.
   */
  simdjson_inline int64_t get_int64() const noexcept;
  simdjson_inline operator int64_t() const noexcept;


  /**
   * return true if the automatically determined type of
   * the number is number_type::floating_point_number.
   */
  simdjson_inline bool is_double() const noexcept;
  /**
   * return the value as a double, only valid if is_double() is true.
   */
  simdjson_inline double get_double() const noexcept;
  simdjson_inline operator double() const noexcept;

  /**
   * Convert the number to a double. Though it always succeed, the conversion
   * may be lossy if the number cannot be represented exactly.
   */
  simdjson_inline double as_double() const noexcept;


protected:
  /**
   * The next block of declaration is designed so that we can call the number parsing
   * functions on a number type. They are protected and should never be used outside
   * of the core simdjson library.
   */
  friend class value_iterator;
  template<typename W>
  friend error_code numberparsing::write_float(const uint8_t *const src, bool negative, uint64_t i, const uint8_t * start_digits, size_t digit_count, int64_t exponent, W &writer);
  template<typename W>
  friend error_code numberparsing::parse_number(const uint8_t *const src, W &writer);
  /** Store a signed 64-bit value to the number. */
  simdjson_inline void append_s64(int64_t value) noexcept;
  /** Store an unsigned 64-bit value to the number. */
  simdjson_inline void append_u64(uint64_t value) noexcept;
  /** Store a double value to the number. */
  simdjson_inline void append_double(double value) noexcept;
  /** Specifies that the value is a double, but leave it undefined. */
  simdjson_inline void skip_double() noexcept;
  /**
   * End of friend declarations.
   */

  /**
   * Our attributes are a union type (size = 64 bits)
   * followed by a type indicator.
   */
  union {
    double floating_point_number;
    int64_t signed_integer;
    uint64_t unsigned_integer;
  } payload{0};
  number_type type{number_type::signed_integer};
};

/**
 * Write the JSON type to the output stream
 *
 * @param out The output stream.
 * @param type The json_type.
 */
inline std::ostream& operator<<(std::ostream& out, json_type type) noexcept;

#if SIMDJSON_EXCEPTIONS
/**
 * Send JSON type to an output stream.
 *
 * @param out The output stream.
 * @param type The json_type.
 * @throw simdjson_error if the result being printed has an error. If there is an error with the
 *        underlying output stream, that error will be propagated (simdjson_error will not be
 *        thrown).
 */
inline std::ostream& operator<<(std::ostream& out, simdjson_result<json_type> &type) noexcept(false);
#endif

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

template<>
struct simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_type> : public SIMDJSON_IMPLEMENTATION::implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_type> {
public:
  simdjson_inline simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::json_type &&value) noexcept; ///< @private
  simdjson_inline simdjson_result(error_code error) noexcept; ///< @private
  simdjson_inline simdjson_result() noexcept = default;
  simdjson_inline ~simdjson_result() noexcept = default; ///< @private
};

} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_JSON_TYPE_H