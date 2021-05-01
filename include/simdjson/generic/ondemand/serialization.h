#include "simdjson/dom/serialization.h"
#include "simdjson/error.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

template <class formatter = simdjson::internal::mini_formatter>
class string_builder {
public:
  /** Append an document to the builder (to be printed), numbers are
   * assumed to be 64-bit floating-point numbers.
   **/
  inline simdjson::error_code append(document& value) noexcept;
  /** Append an element to the builder (to be printed) **/
  inline simdjson::error_code append(value element) noexcept;
  /** Append an array to the builder (to be printed) **/
  inline simdjson::error_code append(array value) noexcept;
  /** Append an object to the builder (to be printed) **/
  inline simdjson::error_code append(object value) noexcept;
  /** Append a field to the builder (to be printed) **/
  inline simdjson::error_code append(field value) noexcept;
  /** Reset the builder (so that it would print the empty string) **/
  simdjson_really_inline void clear();
  /**
   * Get access to the string. The string_view is owned by the builder
   * and it is invalid to use it after the string_builder has been
   * destroyed.
   * However you can make a copy of the string_view on memory that you
   * own.
   */
  simdjson_really_inline std::string_view str() const;
private:
  formatter format{};
};

/**
 * Print JSON to an output stream.
 *
 * @param out The output stream.
 * @param value The element.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, value x) {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::string_builder<> sb;
    auto err = sb.append(x);
    if(err == simdjson::SUCCESS) {
      return (out << sb.str());
    } else {
      throw simdjson::simdjson_error(err);
    }
}
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<value> x) {
    if (x.error()) { throw simdjson::simdjson_error(x.error()); }
    return (out << x.value());
}
#else
inline std::ostream& operator<<(std::ostream& out, value x) {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::string_builder<> sb;
    auto error = sb.append(x);
    if(error == simdjson::SUCCESS) {
      return (out << sb.str());
    } else {
      return (out << error);
    }
}
#endif
/**
 * Print JSON to an output stream.
 *
 * @param out The output stream.
 * @param value The array.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, array value) {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::string_builder<> sb;
    auto err = sb.append(value);
    if(err == simdjson::SUCCESS) {
      return (out << sb.str());
    } else {
      throw simdjson::simdjson_error(err);
    }
}
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<array> x) {
    if (x.error()) { throw simdjson::simdjson_error(x.error()); }
    return (out << x.value());
}
#else
inline std::ostream& operator<<(std::ostream& out, array value) {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::string_builder<> sb;
    auto error = sb.append(value);
    if(error == simdjson::SUCCESS) {
      return (out << sb.str());
    } else {
      return (out << error);
    }
}
#endif
/**
 * Print JSON to an output stream.
 *
 * @param out The output stream.
 * @param value The array.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, document& value)  {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::string_builder<> sb;
    auto err = sb.append(value);
    if(err == simdjson::SUCCESS) {
      return (out << sb.str());
    } else {
      throw simdjson::simdjson_error(err);
    }
}
inline std::ostream& operator<<(std::ostream& out, simdjson::simdjson_result<document> x) {
    if (x.error()) { throw simdjson::simdjson_error(x.error()); }
    return (out << x.value());
}
#else
inline std::ostream& operator<<(std::ostream& out, document& value)  {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::string_builder<> sb;
    auto error = sb.append(value);
    if(error == simdjson::SUCCESS) {
      return (out << sb.str());
    } else {
      return (out << error);
    }
}
#endif
/**
 * Print JSON to an output stream.
 *
 * @param out The output stream.
 * @param value The object.
 * @throw if there is an error with the underlying output stream. simdjson itself will not throw.
 */
#if SIMDJSON_EXCEPTIONS
inline std::ostream& operator<<(std::ostream& out, object value) {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::string_builder<> sb;
    auto err = sb.append(value);
    if(err == simdjson::SUCCESS) {
      return (out << sb.str());
    } else {
       throw simdjson::simdjson_error(err);
    }
}
inline std::ostream& operator<<(std::ostream& out,  simdjson::simdjson_result<object> x) {
    if (x.error()) { throw  simdjson::simdjson_error(x.error()); }
    return (out << x.value());
}
#else
inline std::ostream& operator<<(std::ostream& out, object value) {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::string_builder<> sb;
    auto error = sb.append(value);
    if(error == simdjson::SUCCESS) {
      return (out << sb.str());
    } else {
      return (out << error);
    }
}
#endif

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

inline simdjson::simdjson_result<std::string> to_string(simdjson::SIMDJSON_IMPLEMENTATION::ondemand::document& x) {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::string_builder<> sb;
    auto error = sb.append(x);
    if(error != simdjson::SUCCESS) { return error; }
    std::string_view answer = sb.str();
    return std::string(answer.data(), answer.size());
}

inline simdjson::simdjson_result<std::string> to_string(simdjson::SIMDJSON_IMPLEMENTATION::ondemand::value& x) {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::string_builder<> sb;
    auto error = sb.append(x);
    if(error != simdjson::SUCCESS) { return error; }
    std::string_view answer = sb.str();
    return std::string(answer.data(), answer.size());
}

inline simdjson::simdjson_result<std::string> to_string(simdjson::SIMDJSON_IMPLEMENTATION::ondemand::object& x) {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::string_builder<> sb;
    auto error = sb.append(x);
    if(error != simdjson::SUCCESS) { return error; }
    std::string_view answer = sb.str();
    return std::string(answer.data(), answer.size());
}

inline simdjson::simdjson_result<std::string> to_string(simdjson::SIMDJSON_IMPLEMENTATION::ondemand::array& x) {
    simdjson::SIMDJSON_IMPLEMENTATION::ondemand::string_builder<> sb;
    auto error = sb.append(x);
    if(error != simdjson::SUCCESS) { return error; }
    std::string_view answer = sb.str();
    return std::string(answer.data(), answer.size());
}

} // namespace simdjson
