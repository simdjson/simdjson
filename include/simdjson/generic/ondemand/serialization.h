#include "simdjson/dom/serialization.h"
#include "simdjson/error.h"

namespace simdjson {
/**
 * Create a string-view instance out of a document instance. The string-view instance
 * contains JSON text that is suitable to be parsed as JSON again.
 */
inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::ondemand::document& x) noexcept;
/**
 * Create a string-view instance out of a value instance. The string-view instance
 * contains JSON text that is suitable to be parsed as JSON again. The value must
 * not have been accessed previously.
 */
inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::ondemand::value& x) noexcept;
/**
 * Create a string-view instance out of an object instance. The string-view instance
 * contains JSON text that is suitable to be parsed as JSON again.
 */
inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::ondemand::object& x) noexcept;
/**
 * Create a string-view instance out of an array instance. The string-view instance
 * contains JSON text that is suitable to be parsed as JSON again.
 */
inline simdjson_result<std::string_view> to_json_string(SIMDJSON_IMPLEMENTATION::ondemand::array& x) noexcept;
inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::document> x);
inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::value> x);
inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::object> x);
inline simdjson_result<std::string_view> to_json_string(simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::array> x);
} // namespace simdjson


/**
 * Print JSON to an output stream.
 *
 * @param out The output stream.
 * @param value The element.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value x);
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value> x);
#endif
/**
 * Print JSON to an output stream.
 *
 * @param out The output stream.
 * @param value The array.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array value);
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array> x);
#endif
/**
 * Print JSON to an output stream.
 *
 * @param out The output stream.
 * @param value The array.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::document& value);
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::document>&& x);
#endif
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::document_reference& value);
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::document_reference>&& x);
#endif
/**
 * Print JSON to an output stream.
 *
 * @param out The output stream.
 * @param value The object.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
inline std::ostream& operator<<(std::ostream& out, simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object value);
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object> x);
#endif
