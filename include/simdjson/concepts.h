#ifndef SIMDJSON_CONCEPTS_H
#define SIMDJSON_CONCEPTS_H
#if SIMDJSON_SUPPORTS_DESERIALIZATION

#include <concepts>
#include <type_traits>

namespace simdjson {
namespace concepts {

namespace details {
#define SIMDJSON_IMPL_CONCEPT(name, method)                                    \
  template <typename T>                                                        \
  concept supports_##name = !std::is_const_v<T> && requires {                  \
    typename std::remove_cvref_t<T>::value_type;                               \
    requires requires(typename std::remove_cvref_t<T>::value_type &&val,       \
                      T obj) {                                                 \
      obj.method(std::move(val));                                              \
      requires !requires { obj = std::move(val); };                            \
    };                                                                         \
  };

SIMDJSON_IMPL_CONCEPT(emplace_back, emplace_back)
SIMDJSON_IMPL_CONCEPT(emplace, emplace)
SIMDJSON_IMPL_CONCEPT(push_back, push_back)
SIMDJSON_IMPL_CONCEPT(add, add)
SIMDJSON_IMPL_CONCEPT(push, push)
SIMDJSON_IMPL_CONCEPT(append, append)
SIMDJSON_IMPL_CONCEPT(insert, insert)
SIMDJSON_IMPL_CONCEPT(op_append, operator+=)

#undef SIMDJSON_IMPL_CONCEPT
} // namespace details


template <typename T>
concept string_view_like = std::is_convertible_v<T, std::string_view> &&
                           !std::is_convertible_v<T, const char*>;

template<typename T>
concept constructible_from_string_view = std::is_constructible_v<T, std::string_view>
                                        && !std::is_same_v<T, std::string_view>
                                        && std::is_default_constructible_v<T>;

template<typename M>
concept string_view_keyed_map = string_view_like<typename M::key_type>
              && requires(std::remove_cvref_t<M>& m, typename M::key_type sv, typename M::mapped_type v) {
    { m.emplace(sv, v) } -> std::same_as<std::pair<typename M::iterator, bool>>;
};

/// Check if T is a container that we can append to, including:
///   std::vector, std::deque, std::list, std::string, ...
template <typename T>
concept appendable_containers =
    (details::supports_emplace_back<T> || details::supports_emplace<T> ||
    details::supports_push_back<T> || details::supports_push<T> ||
    details::supports_add<T> || details::supports_append<T> ||
    details::supports_insert<T>) && !string_view_keyed_map<T>;

/// Insert into the container however possible
template <appendable_containers T, typename... Args>
constexpr decltype(auto) emplace_one(T &vec, Args &&...args) {
  if constexpr (details::supports_emplace_back<T>) {
    return vec.emplace_back(std::forward<Args>(args)...);
  } else if constexpr (details::supports_emplace<T>) {
    return vec.emplace(std::forward<Args>(args)...);
  } else if constexpr (details::supports_push_back<T>) {
    return vec.push_back(std::forward<Args>(args)...);
  } else if constexpr (details::supports_push<T>) {
    return vec.push(std::forward<Args>(args)...);
  } else if constexpr (details::supports_add<T>) {
    return vec.add(std::forward<Args>(args)...);
  } else if constexpr (details::supports_append<T>) {
    return vec.append(std::forward<Args>(args)...);
  } else if constexpr (details::supports_insert<T>) {
    return vec.insert(std::forward<Args>(args)...);
  } else if constexpr (details::supports_op_append<T> && sizeof...(Args) == 1) {
    return vec.operator+=(std::forward<Args>(args)...);
  } else {
    static_assert(!sizeof(T *),
                  "We don't know how to add things to this container");
  }
}

/// This checks if the container will return a reference to the newly added
/// element after an insert which for example `std::vector::emplace_back` does
/// since C++17; this will allow some optimizations.
template <typename T>
concept returns_reference = appendable_containers<T> && requires {
  typename std::remove_cvref_t<T>::reference;
  requires requires(typename std::remove_cvref_t<T>::value_type &&val, T obj) {
    {
      emplace_one(obj, std::move(val))
    } -> std::same_as<typename std::remove_cvref_t<T>::reference>;
  };
};

template <typename T>
concept smart_pointer = requires(std::remove_cvref_t<T> ptr) {
  // Check if T has a member type named element_type
  typename std::remove_cvref_t<T>::element_type;

  // Check if T has a get() member function
  {
    ptr.get()
  } -> std::same_as<typename std::remove_cvref_t<T>::element_type *>;

  // Check if T can be dereferenced
  { *ptr } -> std::same_as<typename std::remove_cvref_t<T>::element_type &>;
};

template <typename T>
concept optional_type = requires(std::remove_cvref_t<T> obj) {
  typename std::remove_cvref_t<T>::value_type;
  { obj.value() } -> std::same_as<typename std::remove_cvref_t<T>::value_type&>;
  requires requires(typename std::remove_cvref_t<T>::value_type &&val) {
    obj.emplace(std::move(val));
    obj = std::move(val);
    {
      obj.value_or(val)
    } -> std::convertible_to<typename std::remove_cvref_t<T>::value_type>;
  };
  { static_cast<bool>(obj) } -> std::same_as<bool>; // convertible to bool
};



} // namespace concepts
} // namespace simdjson
#endif // SIMDJSON_SUPPORTS_DESERIALIZATION
#endif // SIMDJSON_CONCEPTS_H
