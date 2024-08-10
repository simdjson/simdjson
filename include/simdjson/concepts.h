#ifndef SIMDJSON_CONCEPTS_H
#define SIMDJSON_CONCEPTS_H
#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

#ifdef __cpp_concepts
#include <utility>
#include <vector>

namespace simdjson {
namespace concepts {
template <typename C> struct is_vector : std::false_type {};
template <typename T> struct is_vector<std::vector<T>> : std::true_type {};
template <typename C>
concept std_vector = is_vector<C>::value;
} // namespace concepts
} // namespace simdjson
#endif

#endif