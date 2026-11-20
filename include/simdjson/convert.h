
#ifndef SIMDJSON_CONVERT_H
#define SIMDJSON_CONVERT_H

#include "simdjson/ondemand.h"
#include <optional>

#if SIMDJSON_SUPPORTS_CONCEPTS


namespace simdjson {
namespace convert {
namespace internal {

/**
 * A utility class for automatically parsing JSON documents.
 * This template is NOT part of our public API.
 * It is subject to changes.
 * @private
 */
template <typename parser_type = ondemand::parser*>
struct auto_parser {
private:
	parser_type m_parser;
	ondemand::document m_doc;
	error_code m_error{SUCCESS};
	// Original buffer, when this auto_parser was built from a string. Used to
	// direct-parse atoms and arrays of atoms without a structural index.
	// Lifetime is the caller's buffer, same rule as ondemand::iterate.
	const uint8_t *m_direct_buf{nullptr};
	size_t m_direct_len{0};
	size_t m_direct_cap{0};
	// True when m_doc was produced by iterate (or supplied by the caller).
	// String constructors leave this false so result() can direct-parse first.
	bool m_doc_ready{false};

	template <typename T>
	static constexpr bool is_nothrow_gettable = requires(ondemand::document doc) {
		{ doc.get<T>() } noexcept;
	};
	template <typename T>
	static constexpr bool views_document = std::is_same_v<T, ondemand::array> || std::is_same_v<T, ondemand::object>
		|| std::is_same_v<T, ondemand::value> || std::is_same_v<T, ondemand::document_reference>;
public:
	explicit auto_parser(parser_type &&parser, ondemand::document &&doc) noexcept requires(!std::is_pointer_v<parser_type>);
	explicit auto_parser(parser_type &&parser, padded_string_view const str) noexcept requires(!std::is_pointer_v<parser_type>);
	explicit auto_parser(std::remove_pointer_t<parser_type> &parser, ondemand::document &&doc) noexcept requires(std::is_pointer_v<parser_type>);
	explicit auto_parser(std::remove_pointer_t<parser_type> &parser, padded_string_view const str) noexcept requires(std::is_pointer_v<parser_type>);
	explicit auto_parser(padded_string_view const str) noexcept requires(std::is_pointer_v<parser_type>);
	explicit auto_parser(parser_type parser, ondemand::document &&doc) noexcept requires(std::is_pointer_v<parser_type>);
	auto_parser(auto_parser const &) = delete;
	auto_parser &operator=(auto_parser const &) = delete;
	~auto_parser() = default;
    // Prevent moving
    auto_parser(auto_parser&&) = delete;
	auto_parser &operator=(auto_parser &&) noexcept = delete;
	simdjson_warn_unused std::remove_pointer_t<parser_type> &parser() noexcept;

	// An array, an object, a value or a document_reference points into the document
	// held by this auto_parser, so it cannot be taken from a temporary:
	// simdjson::from(json).array() would dangle once the full-expression ends.
	template <typename T>
	simdjson_warn_unused simdjson_inline simdjson_result<T> result() & noexcept(is_nothrow_gettable<T>);
	template <typename T>
	simdjson_warn_unused simdjson_inline simdjson_result<T> result() && noexcept(is_nothrow_gettable<T>) requires(!views_document<T>);
	template <typename T>
	simdjson_result<T> result() && requires(views_document<T>) = delete;
	template <typename T>
	simdjson_warn_unused simdjson_inline error_code get(T &value) && noexcept(is_nothrow_gettable<T>) requires(!views_document<T>);
	template <typename T>
	error_code get(T &value) && requires(views_document<T>) = delete;

	simdjson_warn_unused simdjson_inline simdjson_result<ondemand::array> array() & noexcept;
	simdjson_warn_unused simdjson_inline simdjson_result<ondemand::object> object() & noexcept;
	simdjson_result<ondemand::array> array() && = delete;
	simdjson_result<ondemand::object> object() && = delete;
	simdjson_warn_unused simdjson_inline simdjson_result<ondemand::number> number() noexcept;


#if SIMDJSON_EXCEPTIONS
	template <typename T>
	simdjson_warn_unused simdjson_inline explicit(false) operator T() & noexcept(false);
	template <typename T>
	simdjson_warn_unused simdjson_inline explicit(false) operator T() && noexcept(false) requires(!views_document<T>);
	template <typename T>
	explicit(false) operator T() && requires(views_document<T>) = delete;
#endif // SIMDJSON_EXCEPTIONS

	template <typename T>
	simdjson_warn_unused simdjson_inline std::optional<T> optional() & noexcept(is_nothrow_gettable<T>);
	template <typename T>
	simdjson_warn_unused simdjson_inline std::optional<T> optional() && noexcept(is_nothrow_gettable<T>) requires(!views_document<T>);
	template <typename T>
	std::optional<T> optional() && requires(views_document<T>) = delete;
};


/**
 * A utility class for adapting values for the `auto_parser`.
 * This template is not part of our public API. It is subject to changes.
 * @private
 */
template <typename T = void>
struct to_adaptor {
	T operator()(simdjson_result<ondemand::value> &val) const noexcept;
	auto operator()(padded_string_view const str) const noexcept;
	auto operator()(ondemand::parser &parser, padded_string_view const str) const noexcept;
};
// deduction guide
auto_parser(padded_string_view const str) -> auto_parser<ondemand::parser*>;
} // namespace internal
} // namespace convert

/**
 * The simdjson::from instance is EXPERIMENTAL AND SUBJECT TO CHANGES.
 *
 * The `from` instance is a utility adaptor for parsing JSON strings into objects.
 *
 * The string must be a simdjson::padded_string_view, which can be created from a std::string
 * with simdjson::pad(), from a simdjson::padded_string, or string literal using the `_padded`
 * user-defined literal.
 *
 * The `from` instance provides a convenient way to convert JSON data into C++ objects using the `auto_parser`.
 *
 * Example usage:
 *
 * ```cpp
 * std::map<std::string, std::string> obj =
 *   simdjson::from(R"({"key": "value"})"_padded);
 * ```
 *
 * This will parse the JSON string and return an object representation. By default, we
 * use the simdjson::ondemand::parser::get_parser() instance. A parser instance should
 * be used for just one document at a time.
 *
 * You can also pass you own parser instance:
 * ```cpp
 * simdjson::ondemand::parser parser;
 * std::map<std::string, std::string> obj =
 *   simdjson::from(parser, R"({"key": "value"})"_padded);
 * ```
 * The parser instance can be reused.
 *
 * This functionality requires C++20 or better.
 */
static constexpr convert::internal::to_adaptor<> from{};

} // namespace simdjson

#endif // SIMDJSON_SUPPORTS_CONCEPTS
#endif // SIMDJSON_CONVERT_H