
#ifndef SIMDJSON_CONVERT_H
#define SIMDJSON_CONVERT_H

#include "simdjson/ondemand.h"
#include <optional>
#include <string_view>
#include <tuple>

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

	template <typename T>
	simdjson_warn_unused simdjson_inline simdjson_result<T> result() noexcept(is_nothrow_gettable<T>);
	template <typename T>
	simdjson_warn_unused simdjson_inline error_code get(T &value) && noexcept(is_nothrow_gettable<T>);


	simdjson_warn_unused simdjson_inline simdjson_result<ondemand::array> array() noexcept;
	simdjson_warn_unused simdjson_inline simdjson_result<ondemand::object> object() noexcept;
	simdjson_warn_unused simdjson_inline simdjson_result<ondemand::number> number() noexcept;


#if SIMDJSON_EXCEPTIONS
	template <typename T>
	simdjson_warn_unused simdjson_inline explicit(false) operator T() noexcept(false);
#endif // SIMDJSON_EXCEPTIONS

	template <typename T>
	simdjson_warn_unused simdjson_inline std::optional<T> optional() noexcept(is_nothrow_gettable<T>);
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

/**
 * JSON object key for direct_field. Construct from a string literal.
 * Keys longer than 63 characters are not supported on this path.
 * Requires C++20 class-type non-type template parameters.
 */
struct direct_key {
  char data[64]{};
  size_t len{};
  template <size_t N>
  constexpr direct_key(const char (&s)[N]) noexcept : len(N - 1) {
    static_assert(N <= 64, "direct_key is limited to 63 characters");
    for (size_t i = 0; i < N; ++i) { data[i] = s[i]; }
  }
  constexpr size_t size() const noexcept { return len; }
};

/**
 * One field of a direct_schema<T>::fields tuple. Prefer this form over
 * for_each_field: the in-order parser unrolls it.
 * `direct_field<direct_key{"make"}, &Car::make>{}`
 */
template <direct_key Key, auto MemberPtr>
struct direct_field {
  static constexpr auto member = MemberPtr;
  static constexpr direct_key key_storage = Key;
  static constexpr std::string_view name{key_storage.data, key_storage.len};
};

/**
 * Opt-in description of a type for the single-pass `simdjson::from` parser.
 *
 * Specialize this in namespace `simdjson` to let `from<T>` (and
 * `from<std::vector<T>>`) parse `T` in a single pass, without building a
 * structural index. Anything the direct parser cannot accept (malformed
 * JSON, a value of the wrong type, an escaped key, an unpaired surrogate)
 * falls back to On Demand, which uses the deserializer of T (tag_invoke or
 * static reflection). T must therefore also be deserializable by On
 * Demand.
 *
 * ```cpp
 * struct Car { std::string make, model; int64_t year; std::vector<double> tire_pressure; };
 * namespace simdjson {
 * template<> struct direct_schema<Car> {
 *   static constexpr bool enabled = true;
 *   static constexpr auto fields = std::tuple{
 *     direct_field<direct_key{"make"}, &Car::make>{},
 *     direct_field<direct_key{"model"}, &Car::model>{},
 *     direct_field<direct_key{"year"}, &Car::year>{},
 *     direct_field<direct_key{"tire_pressure"}, &Car::tire_pressure>{},
 *   };
 *   // Alternate form, instead of fields:
 *   // template<typename F> static void for_each_field(F &&f) {
 *   //   f(std::string_view("make"), &Car::make);
 *   //   ...
 *   // }
 * };
 * }
 * ```
 *
 * Supported field types: bool, standard integers (not char), double,
 * std::string, std::vector of those, and
 * other types that themselves specialize direct_schema (a type may hold a
 * std::vector of itself).
 *
 * Keys may come in any order; keys in schema order are fastest. Missing
 * keys keep the value-initialized member, duplicate keys overwrite (the
 * last one wins), and unknown keys are skipped. When the document falls
 * back to On Demand, these cases follow the deserializer of T instead: with
 * static reflection, for example, a missing key is NO_SUCH_FIELD and the
 * first of duplicate keys wins. Write the deserializer of T with the same
 * rules if the result must not depend on the path taken.
 *
 * This customization is experimental.
 */
template <typename T>
struct direct_schema {
  static constexpr bool enabled = false;
};

} // namespace simdjson

#endif // SIMDJSON_SUPPORTS_CONCEPTS
#endif // SIMDJSON_CONVERT_H