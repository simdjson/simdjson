#ifndef SIMDJSON_CONCEPTS_H
#define SIMDJSON_CONCEPTS_H
#if SIMDJSON_SUPPORTS_CONCEPTS

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
concept is_pair = requires { typename T::first_type; typename T::second_type; } &&
                  std::same_as<T, std::pair<typename T::first_type, typename T::second_type>>;
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
    {
      obj.value_or(val)
    } -> std::convertible_to<typename std::remove_cvref_t<T>::value_type>;
  };
  { static_cast<bool>(obj) } -> std::same_as<bool>; // convertible to bool
  { obj.reset() } noexcept -> std::same_as<void>;
};



// Types we serialize as JSON strings (not as containers)
template <typename T>
concept string_like =
  std::is_same_v<std::remove_cvref_t<T>, std::string> ||
  std::is_same_v<std::remove_cvref_t<T>, std::string_view> ||
  std::is_same_v<std::remove_cvref_t<T>, const char*> ||
  std::is_same_v<std::remove_cvref_t<T>, char*>;

// Concept that checks if a type is a container but not a string (because
// strings handling must be handled differently)
// Now uses iterator-based approach for broader container support
template <typename T>
concept container_but_not_string =
  std::ranges::input_range<T> && !string_like<T> && !concepts::string_view_keyed_map<T>;



// Concept: Indexable container that is not a string or associative container
// Accepts: std::vector, std::array, std::deque (have operator[], value_type, not string_like)
// Rejects: std::string (string_like), std::list (no operator[]), std::map (has key_type)
template<typename Container>
concept indexable_container = requires {
  typename Container::value_type;
  requires !concepts::string_like<Container>;
  requires !requires { typename Container::key_type; };  // Reject maps/sets
  requires requires(Container& c, std::size_t i) {
    { c[i] } -> std::convertible_to<typename Container::value_type>;
  };
};


// Variable template to use with std::meta::substitute
template<typename Container>
constexpr bool indexable_container_v = indexable_container<Container>;

} // namespace concepts


/**
 * We use tag_invoke as our customization point mechanism.
 */
template <typename Tag, typename... Args>
concept tag_invocable = requires(Tag tag, Args... args) {
  tag_invoke(std::forward<Tag>(tag), std::forward<Args>(args)...);
};

template <typename Tag, typename... Args>
concept nothrow_tag_invocable =
    tag_invocable<Tag, Args...> && requires(Tag tag, Args... args) {
      {
        tag_invoke(std::forward<Tag>(tag), std::forward<Args>(args)...)
      } noexcept;
    };

} // namespace simdjson
#endif // SIMDJSON_SUPPORTS_CONCEPTS
#endif // SIMDJSON_CONCEPTS_H
