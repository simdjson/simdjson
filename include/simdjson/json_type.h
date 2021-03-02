#ifndef SIMDJSON_JSON_TYPE_H
#define SIMDJSON_JSON_TYPE_H

#include "simdjson/common_defs.h"

namespace simdjson {

/**
 * The type of a JSON value.
 */
enum class json_type {
    // Start at 1 to catch uninitialized / default values more easily
    array=1, ///< A JSON array   ( [ 1, 2, 3 ... ] )
    object,  ///< A JSON object  ( { "a": 1, "b" 2, ... } )
    number,  ///< A JSON number  ( 1 or -2.3 or 4.5e6 ...)
    string,  ///< A JSON string  ( "a" or "å" or "hello world\n" ...)
    boolean, ///< A JSON boolean (true or false)
    null     ///< A JSON null    (null)
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

} // namespace simdjson

#endif // SIMDJSON_JSON_TYPE_H
