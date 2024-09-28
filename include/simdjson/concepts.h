#ifndef SIMDJSON_CONCEPTS_H
#define SIMDJSON_CONCEPTS_H
#if SIMDJSON_SUPPORTS_DESERIALIZATION

#include <concepts>
#include <string>
#include <string_view>

namespace simdjson {
namespace concepts {

// std::vector and std::deque are pushable containers
template <typename T>
concept pushable_container =
    requires(T a, typename T::value_type val) {
      a.push_back(val);
    } && !std::is_same_v<T, std::string> &&
    !std::is_same_v<T, std::string_view> &&
    !std::is_same_v<T, const char*>;

} // namespace concepts
} // namespace simdjson
#endif // SIMDJSON_SUPPORTS_DESERIALIZATION
#endif //SIMDJSON_CONCEPTS_H
