#ifndef SIMDJSON_GENERIC_SINGLESTAGE_SERIALIZATION_H

#ifndef SIMDJSON_AMALGAMATED
#define SIMDJSON_GENERIC_SINGLESTAGE_SERIALIZATION_H
#include "simdjson/generic/singlestage/base.h"
#endif // SIMDJSON_AMALGAMATED

namespace simdjson {
/**
 * Create a string-view instance out of a document instance. The string-view instance
 * contains JSON text that is suitable to be parsed as JSON again. It does not
 * validate the content.
 */
inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::singlestage::document& x) noexcept;
/**
 * Create a string-view instance out of a value instance. The string-view instance
 * contains JSON text that is suitable to be parsed as JSON again. The value must
 * not have been accessed previously. It does not
 * validate the content.
 */
inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::singlestage::value& x) noexcept;
/**
 * Create a string-view instance out of an object instance. The string-view instance
 * contains JSON text that is suitable to be parsed as JSON again. It does not
 * validate the content.
 */
inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::singlestage::object& x) noexcept;
/**
 * Create a string-view instance out of an array instance. The string-view instance
 * contains JSON text that is suitable to be parsed as JSON again. It does not
 * validate the content.
 */
inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::singlestage::array& x) noexcept;
inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::document> x);
inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::value> x);
inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::object> x);
inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::singlestage::array> x);
} // namespace simdjson

/**
 * We want to support argument-dependent lookup (ADL).
 * Hence we should define operator<< in the namespace
 * where the argument (here value, object, etc.) resides.
 * Credit: @madhur4127
 * See https://github.com/simdjson/simdjson/issues/1768
 */
namespace simdjson { namespace SIMDJSON_IMPLEMENTATION { namespace singlestage {

/**
 * Print JSON to an output stream.  It does not
 * validate the content.
 *
 * @param out The output stream.
 * @param value The element.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::singlestage::value x);
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::singlestage::value> x);
#endif
/**
 * Print JSON to an output stream. It does not
 * validate the content.
 *
 * @param out The output stream.
 * @param value The array.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::singlestage::array value);
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::singlestage::array> x);
#endif
/**
 * Print JSON to an output stream. It does not
 * validate the content.
 *
 * @param out The output stream.
 * @param value The array.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::singlestage::document& value);
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::singlestage::document>&& x);
#endif
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::singlestage::document_reference& value);
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::singlestage::document_reference>&& x);
#endif
/**
 * Print JSON to an output stream. It does not
 * validate the content.
 *
 * @param out The output stream.
 * @param value The object.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::singlestage::object value);
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::singlestage::object> x);
#endif
}}} // namespace simdjson::SIMDJSON_IMPLEMENTATION::singlestage

#endif // SIMDJSON_GENERIC_SINGLESTAGE_SERIALIZATION_H