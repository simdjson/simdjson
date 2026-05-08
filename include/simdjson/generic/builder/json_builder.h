#ifndef SIMDJSON_GENERIC_BUILDER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_STRING_BUILDER_H
#include "simdjson/generic/builder/json_string_builder.h"
#include "simdjson/concepts.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE
#if SIMDJSON_STATIC_REFLECTION

#include <charconv>
#include <cstring>
#include <meta>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
// #include <static_reflection> // for std::define_static_string - header not available yet

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace builder {

// Forward-declare helpers defined in json_string_builder-inl.h so the
// writer-based atom code below can call them (the -inl.h is not yet
// included at the point this header is parsed; without these forwards,
// name lookup falls back to the wrong outer namespace).
namespace internal {
simdjson_really_inline char *write_uint_jeaiii(char *p, uint64_t v) noexcept;
} // namespace internal
inline size_t write_string_escaped(const std::string_view input, char *out);

// =============================================================
// `writer`: position-as-local hot-path writer used by the reflection
// atom code below. Holds the buffer pointer, write position and
// capacity in three fields that, once `writer` itself is a stack-local
// in the caller and all atom() functions are inlined, become true
// register-resident locals after SROA. Glaze achieves the same effect
// by passing `B&& b, auto&& ix` through every helper. Holding `pos`
// in a register (rather than as a member of string_builder) is what
// breaks the strict-aliasing penalty on every char* write through the
// buffer, which forces a reload of `b.position` and `b.capacity`
// after every byte.
// =============================================================
struct writer {
  char *ptr;        // buffer pointer (refreshed after a grow)
  size_t pos;       // write position (local)
  size_t cap;       // capacity (refreshed after a grow)
  string_builder *sb;  // back-ref for grow / sync

  // Snapshot string_builder state into a writer for the duration of
  // a write chain.
  simdjson_really_inline writer(string_builder &builder) noexcept
      : ptr(builder.unsafe_data())
      , pos(builder.unsafe_position())
      , cap(builder.unsafe_capacity())
      , sb(&builder) {}

  // Write the local position back to the underlying string_builder.
  // Caller is responsible for invoking before the writer is dropped
  // (otherwise data is lost). Idempotent.
  simdjson_really_inline void sync() noexcept {
    sb->unsafe_set_position(pos);
  }

  // Ensure at least `n` more bytes of free capacity. Grows the
  // underlying buffer if needed (rare path). Returns false on
  // allocation failure.
  simdjson_really_inline bool ensure(size_t n) noexcept {
    // Use subtraction (relying on the pos <= cap invariant) so a huge n
    // cannot wrap pos + n to a small value that spuriously passes the test.
    // This is pedantic except maybe on 32-bit targets.
    if (simdjson_likely(n <= cap - pos)) return true;
    return grow_slow(n);
  }

  // Slow path of ensure(). Out-of-line via simdjson_inline (not
  // simdjson_really_inline) to keep the hot path short.
  simdjson_inline bool grow_slow(size_t n) noexcept {
    // Detect overflow.
    // This is pedantic except maybe on 32-bit targets.
    if (simdjson_unlikely(pos + n < pos)) return false;
    sb->unsafe_set_position(pos);
    // even if 2*capacity overflows, the (std::max) below will pick the needed value,
    // so we do not need a separate overflow check here.
    if (!sb->unsafe_grow((std::max)(cap * 2, pos + n))) {
      return false;
    }
    ptr = sb->unsafe_data();
    cap = sb->unsafe_capacity();
    return true;
  }
};

// === Helper: invoke a string_builder member that writes variable-length
// content (escape_and_append_with_quotes etc), syncing the writer's local
// state before the call and reloading after. Used for string fields where
// rewriting the entire SIMD escape path through the writer would be a much
// bigger refactor.
template <class F>
simdjson_really_inline void call_through_string_builder(writer &w, F &&f) noexcept {
  w.sync();
  f(*w.sb);
  w.ptr = w.sb->unsafe_data();
  w.pos = w.sb->unsafe_position();
  w.cap = w.sb->unsafe_capacity();
}

template <class T>
  requires(concepts::container_but_not_string<T> && ! concepts::optional_type<T> && !require_custom_serialization<T>)
simdjson_really_inline constexpr void atom(writer &w, const T &t) {
  auto it = t.begin();
  auto end = t.end();
  if (it == end) {
    if (!w.ensure(2)) return;
    std::memcpy(w.ptr + w.pos, "[]", 2);
    w.pos += 2;
    return;
  }
  if (!w.ensure(1)) return;
  w.ptr[w.pos++] = '[';
  atom(w, *it);
  ++it;
  for (; it != end; ++it) {
    if (!w.ensure(1)) return;
    w.ptr[w.pos++] = ',';
    atom(w, *it);
  }
  if (!w.ensure(1)) return;
  w.ptr[w.pos++] = ']';
}

template <class T>
  requires(std::is_same_v<T, std::string> ||
           std::is_same_v<T, std::string_view> ||
           std::is_same_v<T, const char *> ||
           std::is_same_v<T, char>)
simdjson_really_inline constexpr void atom(writer &w, const T &t) {
  // Inline the escape path through the writer so we never round-trip
  // pos through memory for string fields (Twitter is dominated by
  // these — sync/reload around each string was a real cost).
  std::string_view input;
  if constexpr (std::is_same_v<T, char>) {
    input = std::string_view(&t, 1);
  } else {
    input = std::string_view(t);
  }
  // Worst-case escape: every byte expands to \uXXXX (6 chars), plus 2 quotes.
  // Guard against 2 + 6 * input.size() wrapping for huge inputs — if it
  // wrapped to a small value, ensure() would spuriously succeed and the
  // subsequent escape would overflow the buffer.
  // Note that this is pedantic except maybe on 32-bit targets.
  if (simdjson_unlikely(input.size() > ((std::numeric_limits<size_t>::max)() - 2) / 6)) { return; }
  if (!w.ensure(2 + 6 * input.size())) { return; }
  w.ptr[w.pos++] = '"';
  w.pos += write_string_escaped(input, w.ptr + w.pos);
  w.ptr[w.pos++] = '"';
}

template <concepts::string_view_keyed_map T>
  requires(!require_custom_serialization<T>)
simdjson_really_inline constexpr void atom(writer &w, const T &m) {
  if (m.empty()) {
    if (!w.ensure(2)) return;
    std::memcpy(w.ptr + w.pos, "{}", 2);
    w.pos += 2;
    return;
  }
  if (!w.ensure(1)) return;
  w.ptr[w.pos++] = '{';
  bool first = true;
  for (const auto& [key, value] : m) {
    if (!first) {
      if (!w.ensure(1)) return;
      w.ptr[w.pos++] = ',';
    }
    first = false;
    // Keys must be convertible to string_view per the concept.
    std::string_view key_sv(key);
    // Guard against 3 + 6 * key_sv.size() wrapping for huge keys, if it
    // wrapped to a small value, ensure() would spuriously succeed and the
    // subsequent escape would overflow the buffer.
    // Note that this is pedantic except maybe on 32-bit targets.
    if (simdjson_unlikely(key_sv.size() > ((std::numeric_limits<size_t>::max)() - 3) / 6)) { return; }
    if (!w.ensure(2 + 6 * key_sv.size() + 1)) { return; }
    w.ptr[w.pos++] = '"';
    w.pos += write_string_escaped(key_sv, w.ptr + w.pos);
    w.ptr[w.pos++] = '"';
    w.ptr[w.pos++] = ':';
    atom(w, value);
  }
  if (!w.ensure(1)) return;
  w.ptr[w.pos++] = '}';
}


template<typename number_type,
         typename = typename std::enable_if<std::is_arithmetic<number_type>::value && !std::is_same_v<number_type, char>>::type>
simdjson_really_inline constexpr void atom(writer &w, const number_type t) {
  // Booleans / floats: defer to string_builder (rare path; keeps writer hot
  // path free of float-formatter machinery). For integers, write directly
  // via jeaiii using local pos.
  if constexpr (std::is_same_v<number_type, bool>) {
    if (t) {
      if (!w.ensure(4)) return;
      std::memcpy(w.ptr + w.pos, "true", 4);
      w.pos += 4;
    } else {
      if (!w.ensure(5)) return;
      std::memcpy(w.ptr + w.pos, "false", 5);
      w.pos += 5;
    }
  } else if constexpr (std::is_floating_point_v<number_type>) {
    call_through_string_builder(w, [&](string_builder &b) { b.append(t); });
  } else if constexpr (std::is_unsigned_v<number_type>) {
    if (!w.ensure(20)) return;
    char *end = internal::write_uint_jeaiii(
        w.ptr + w.pos, static_cast<uint64_t>(t));
    w.pos = static_cast<size_t>(end - w.ptr);
  } else {
    // signed integral
    if (!w.ensure(20)) return;
    using U = typename std::make_unsigned<number_type>::type;
    bool negative = t < 0;
    U pv = negative ? U(0) - static_cast<U>(t) : static_cast<U>(t);
    w.ptr[w.pos] = '-';
    w.pos += negative;
    char *end = internal::write_uint_jeaiii(
        w.ptr + w.pos, static_cast<uint64_t>(pv));
    w.pos = static_cast<size_t>(end - w.ptr);
  }
}

template <class T>
  requires(std::is_class_v<T> && !concepts::container_but_not_string<T> &&
           !concepts::string_view_keyed_map<T> &&
           !concepts::optional_type<T> &&
           !concepts::smart_pointer<T> &&
           !concepts::appendable_containers<T> &&
           !std::is_same_v<T, std::string> &&
           !std::is_same_v<T, std::string_view> &&
           !std::is_same_v<T, const char*> &&
           !std::is_same_v<T, char> && !require_custom_serialization<T>)
simdjson_really_inline constexpr void atom(writer &w, const T &t) {
  // Per-field block: ensure key+value worst case, then write key + value
  // through the writer's local pos. For arithmetic fields, the integer
  // write happens directly via write_uint_jeaiii on w.ptr+w.pos, so pos
  // never round-trips through memory.
  int i = 0;
  if (!w.ensure(1)) return;
  w.ptr[w.pos++] = '{';
  template for (constexpr auto dm : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
    constexpr auto first_key = std::define_static_string(
        constevalutil::consteval_to_quoted_escaped(std::meta::identifier_of(dm)) + ":");
    constexpr auto rest_key = std::define_static_string(
        std::string(",") + constevalutil::consteval_to_quoted_escaped(std::meta::identifier_of(dm)) + ":");
    constexpr size_t first_key_len = std::char_traits<char>::length(first_key);
    constexpr size_t rest_key_len = std::char_traits<char>::length(rest_key);
    if (!w.ensure(rest_key_len)) return;
    if (i == 0) {
      std::memcpy(w.ptr + w.pos, first_key, first_key_len);
      w.pos += first_key_len;
    } else {
      std::memcpy(w.ptr + w.pos, rest_key, rest_key_len);
      w.pos += rest_key_len;
    }
    atom(w, t.[:dm:]);
    i++;
  };
  if (!w.ensure(1)) return;
  w.ptr[w.pos++] = '}';
}

// Support for optional types (std::optional, etc.)
template <concepts::optional_type T>
  requires(!require_custom_serialization<T>)
simdjson_really_inline constexpr void atom(writer &w, const T &opt) {
  if (opt) {
    atom(w, opt.value());
  } else {
    if (!w.ensure(4)) return;
    std::memcpy(w.ptr + w.pos, "null", 4);
    w.pos += 4;
  }
}

// Support for smart pointers (std::unique_ptr, std::shared_ptr, etc.)
template <concepts::smart_pointer T>
  requires(!require_custom_serialization<T>)
simdjson_really_inline constexpr void atom(writer &w, const T &ptr) {
  if (ptr) {
    atom(w, *ptr);
  } else {
    if (!w.ensure(4)) return;
    std::memcpy(w.ptr + w.pos, "null", 4);
    w.pos += 4;
  }
}

// Support for enums - serialize as string representation using expand approach from P2996R12
template <typename T>
  requires(std::is_enum_v<T> && !require_custom_serialization<T>)
simdjson_really_inline void atom(writer &w, const T &e) {
#if SIMDJSON_STATIC_REFLECTION
  static constexpr auto enumerators = std::define_static_array(std::meta::enumerators_of(^^T));
  template for (constexpr auto enum_val : enumerators) {
    constexpr auto enum_str = std::define_static_string(constevalutil::consteval_to_quoted_escaped(std::meta::identifier_of(enum_val)));
    constexpr size_t enum_str_len = std::char_traits<char>::length(enum_str);
    if (e == [:enum_val:]) {
      if (!w.ensure(enum_str_len)) return;
      std::memcpy(w.ptr + w.pos, enum_str, enum_str_len);
      w.pos += enum_str_len;
      return;
    }
  };
  // Fallback to integer if enum value not found
  atom(w, static_cast<std::underlying_type_t<T>>(e));
#else
  // Fallback: serialize as integer if reflection not available
  atom(w, static_cast<std::underlying_type_t<T>>(e));
#endif
}

// Support for appendable containers that don't have operator[] (sets, etc.)
template <concepts::appendable_containers T>
  requires(!concepts::container_but_not_string<T> && !concepts::string_view_keyed_map<T> &&
           !concepts::optional_type<T> && !concepts::smart_pointer<T> &&
           !std::is_same_v<T, std::string> &&
           !std::is_same_v<T, std::string_view> && !std::is_same_v<T, const char*> && !require_custom_serialization<T>)
simdjson_really_inline constexpr void atom(writer &w, const T &container) {
  if (container.empty()) {
    if (!w.ensure(2)) return;
    std::memcpy(w.ptr + w.pos, "[]", 2);
    w.pos += 2;
    return;
  }
  if (!w.ensure(1)) return;
  w.ptr[w.pos++] = '[';
  bool first = true;
  for (const auto& item : container) {
    if (!first) {
      if (!w.ensure(1)) return;
      w.ptr[w.pos++] = ',';
    }
    first = false;
    atom(w, item);
  }
  if (!w.ensure(1)) return;
  w.ptr[w.pos++] = ']';
}

// append() — top-level entry. Each overload constructs a stack-local
// writer, runs atom(w, t) through the inlined call chain, then syncs
// the local position back into the string_builder.
template <class T>
  requires(std::is_arithmetic_v<T> && !std::is_same_v<T, char>)
simdjson_inline void append(string_builder &b, const T &t) {
  writer w(b);
  atom(w, t);
  w.sync();
}

template <class T>
  requires(std::is_same_v<T, std::string> ||
           std::is_same_v<T, std::string_view> ||
           std::is_same_v<T, const char *> ||
           std::is_same_v<T, char>)
simdjson_inline void append(string_builder &b, const T &t) {
  writer w(b);
  atom(w, t);
  w.sync();
}

template <concepts::optional_type T>
  requires(!require_custom_serialization<T>)
simdjson_inline void append(string_builder &b, const T &t) {
  writer w(b);
  atom(w, t);
  w.sync();
}

template <concepts::smart_pointer T>
  requires(!require_custom_serialization<T>)
simdjson_inline void append(string_builder &b, const T &t) {
  writer w(b);
  atom(w, t);
  w.sync();
}

template <concepts::appendable_containers T>
  requires(!concepts::container_but_not_string<T> && !concepts::string_view_keyed_map<T> &&
           !concepts::optional_type<T> && !concepts::smart_pointer<T> &&
           !std::is_same_v<T, std::string> &&
           !std::is_same_v<T, std::string_view> && !std::is_same_v<T, const char*> && !require_custom_serialization<T>)
simdjson_inline void append(string_builder &b, const T &t) {
  writer w(b);
  atom(w, t);
  w.sync();
}

template <concepts::string_view_keyed_map T>
  requires(!require_custom_serialization<T>)
simdjson_inline void append(string_builder &b, const T &t) {
  writer w(b);
  atom(w, t);
  w.sync();
}

// works for struct
template <class Z>
  requires(std::is_class_v<Z> && !concepts::container_but_not_string<Z> &&
           !concepts::string_view_keyed_map<Z> &&
           !concepts::optional_type<Z> &&
           !concepts::smart_pointer<Z> &&
           !concepts::appendable_containers<Z> &&
           !std::is_same_v<Z, std::string> &&
           !std::is_same_v<Z, std::string_view> &&
           !std::is_same_v<Z, const char*> &&
           !std::is_same_v<Z, char> && !require_custom_serialization<Z>)
simdjson_inline void append(string_builder &b, const Z &z) {
  writer w(b);
  atom(w, z);
  w.sync();
}

// works for container that have begin() and end() iterators
template <class Z>
  requires(concepts::container_but_not_string<Z> && !require_custom_serialization<Z>)
simdjson_inline void append(string_builder &b, const Z &z) {
  writer w(b);
  atom(w, z);
  w.sync();
}

template <class Z>
  requires (require_custom_serialization<Z>)
void append(string_builder &b, const Z &z) {
  b.append(z);
}


template <class Z>
simdjson_warn_unused simdjson_result<std::string> to_json_string(const Z &z, size_t initial_capacity = string_builder::DEFAULT_INITIAL_CAPACITY) {
  string_builder b(initial_capacity);
  append(b, z);
  std::string_view s;
  if(auto e = b.view().get(s); e) { return e; }
  return std::string(s);
}

template <class Z>
simdjson_warn_unused error_code to_json(const Z &z, std::string &s, size_t initial_capacity = string_builder::DEFAULT_INITIAL_CAPACITY) {
  string_builder b(initial_capacity);
  append(b, z);
  std::string_view view;
  if(auto e = b.view().get(view); e) { return e; }
  s.assign(view);
  return SUCCESS;
}

template <class Z>
string_builder& operator<<(string_builder& b, const Z& z) {
  append(b, z);
  return b;
}

// extract_from: Serialize only specific fields from a struct to JSON
template<constevalutil::fixed_string... FieldNames, typename T>
  requires(std::is_class_v<T> && (sizeof...(FieldNames) > 0))
void extract_from(string_builder &b, const T &obj) {
  writer w(b);
  if (!w.ensure(1)) { w.sync(); return; }
  w.ptr[w.pos++] = '{';
  bool first = true;
  // Iterate through all members of T using reflection
  static constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()));
  template for (constexpr auto mem : members) {
    if constexpr (std::meta::is_public(mem)) {
      static constexpr std::string_view key = std::define_static_string(std::meta::identifier_of(mem));

      // Only serialize this field if it's in our list of requested fields
      if constexpr (((FieldNames.view() == key) || ...)) {
        static constexpr auto first_key = std::define_static_string(
            constevalutil::consteval_to_quoted_escaped(std::meta::identifier_of(mem)) + ":");
        static constexpr auto rest_key = std::define_static_string(
            std::string(",") + constevalutil::consteval_to_quoted_escaped(std::meta::identifier_of(mem)) + ":");
        constexpr size_t first_key_len = std::char_traits<char>::length(first_key);
        constexpr size_t rest_key_len = std::char_traits<char>::length(rest_key);
        if (!w.ensure(rest_key_len)) { w.sync(); return; }
        if (first) {
          std::memcpy(w.ptr + w.pos, first_key, first_key_len);
          w.pos += first_key_len;
        } else {
          std::memcpy(w.ptr + w.pos, rest_key, rest_key_len);
          w.pos += rest_key_len;
        }
        first = false;
        atom(w, obj.[:mem:]);
      }
    }
  };

  if (!w.ensure(1)) { w.sync(); return; }
  w.ptr[w.pos++] = '}';
  w.sync();
}

template<constevalutil::fixed_string... FieldNames, typename T>
  requires(std::is_class_v<T> && (sizeof...(FieldNames) > 0))
simdjson_warn_unused simdjson_result<std::string> extract_from(const T &obj, size_t initial_capacity = string_builder::DEFAULT_INITIAL_CAPACITY) {
  string_builder b(initial_capacity);
  extract_from<FieldNames...>(b, obj);
  std::string_view s;
  if(auto e = b.view().get(s); e) { return e; }
  return std::string(s);
}

} // namespace builder
} // namespace SIMDJSON_IMPLEMENTATION
// Alias the function template to 'to' in the global namespace
template <class Z>
simdjson_warn_unused simdjson_result<std::string> to_json(const Z &z, size_t initial_capacity = SIMDJSON_IMPLEMENTATION::builder::string_builder::DEFAULT_INITIAL_CAPACITY) {
  SIMDJSON_IMPLEMENTATION::builder::string_builder b(initial_capacity);
  SIMDJSON_IMPLEMENTATION::builder::append(b, z);
  std::string_view s;
  if(auto e = b.view().get(s); e) { return e; }
  return std::string(s);
}
template <class Z>
simdjson_warn_unused error_code to_json(const Z &z, std::string &s, size_t initial_capacity = SIMDJSON_IMPLEMENTATION::builder::string_builder::DEFAULT_INITIAL_CAPACITY) {
  SIMDJSON_IMPLEMENTATION::builder::string_builder b(initial_capacity);
  SIMDJSON_IMPLEMENTATION::builder::append(b, z);
  std::string_view view;
  if(auto e = b.view().get(view); e) { return e; }
  s.assign(view);
  return SUCCESS;
}
// Global namespace function for extract_from
template<constevalutil::fixed_string... FieldNames, typename T>
  requires(std::is_class_v<T> && (sizeof...(FieldNames) > 0))
simdjson_warn_unused simdjson_result<std::string> extract_from(const T &obj, size_t initial_capacity = SIMDJSON_IMPLEMENTATION::builder::string_builder::DEFAULT_INITIAL_CAPACITY) {
  SIMDJSON_IMPLEMENTATION::builder::string_builder b(initial_capacity);
  SIMDJSON_IMPLEMENTATION::builder::extract_from<FieldNames...>(b, obj);
  std::string_view s;
  if(auto e = b.view().get(s); e) { return e; }
  return std::string(s);
}

} // namespace simdjson

#endif // SIMDJSON_STATIC_REFLECTION

#endif