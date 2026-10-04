#ifndef SIMDJSON_GENERIC_BUILDER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_STRING_BUILDER_H
#include "simdjson/generic/builder/json_string_builder.h"
#include "simdjson/concepts.h"
#include "simdjson/annotations.h"
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
simdjson_inline char *write_double(char *p, double v) noexcept;
} // namespace internal
simdjson_really_inline size_t write_string_escaped(const std::string_view input, char *out);

SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_GCC_WARNING(-Warray-bounds)
#if !defined(__clang__)
SIMDJSON_DISABLE_GCC_WARNING(-Wstringop-overflow)
#endif

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
//
// basic_writer<false> (below) is the unchecked variant: the caller has
// already reserved enough capacity for everything the write chain can
// produce (see bound_detail::size_bound), so ensure() compiles away.
// =============================================================
template <bool Checked>
struct basic_writer {
  static constexpr bool checked = Checked;
  char *ptr;        // buffer pointer (refreshed after a grow)
  size_t pos;       // write position (local)
  size_t cap;       // capacity (refreshed after a grow)
  string_builder &sb;  // back-ref for grow / sync

  // Snapshot string_builder state into a writer for the duration of
  // a write chain.
  simdjson_really_inline basic_writer(string_builder &builder) noexcept
      : ptr(builder.unsafe_data())
      , pos(builder.unsafe_position())
      , cap(builder.unsafe_capacity())
      , sb(builder) {}

  // Write the local position back to the underlying string_builder.
  // Caller is responsible for invoking before the writer is dropped
  // (otherwise data is lost). Idempotent.
  simdjson_really_inline void sync() noexcept {
    sb.unsafe_set_position(pos);
  }

  // Ensure at least `n` more bytes of free capacity. Grows the
  // underlying buffer if needed (rare path). Returns false on
  // allocation failure.
  simdjson_really_inline bool ensure(size_t n) noexcept {
    // pos <= cap, and cap is the size of a live allocation, so pos + n
    // cannot wrap when n is a small constant or a compile-time length.
    // Callers passing a size derived from input (the string atoms) must
    // bound it against pos themselves. Keep the `pos + n <= cap` form:
    // `n <= cap - pos` is measurably slower once the serializer is inlined.
    if (simdjson_likely(pos + n <= cap)) { return true; }
    return grow_slow(n);
  }

  simdjson_never_inline bool grow_slow(size_t n) noexcept {
    // Detect overflow.
    // This is pedantic except maybe on 32-bit targets.
    if (simdjson_unlikely(pos + n < pos)) return false;
    sb.unsafe_set_position(pos);
    // even if 2*capacity overflows, the (std::max) below will pick the needed value,
    // so we do not need a separate overflow check here.
    if (!sb.unsafe_grow((std::max)(cap * 2, pos + n))) {
      // The string_builder freed its buffer and is now invalid (null buffer,
      // zero capacity and position). Mirror that state so that every later
      // ensure() fails too: callers only return from the current atom, and
      // their callers keep writing.
      ptr = nullptr;
      pos = 0;
      cap = 0;
      return false;
    }
    ptr = sb.unsafe_data();
    cap = sb.unsafe_capacity();
    return true;
  }
};

// The unchecked writer writes into a raw buffer that the caller sized with
// serialized_size_bound: it never grows and needs no string_builder.
template <>
struct basic_writer<false> {
  static constexpr bool checked = false;
  char *ptr;
  size_t pos;

  simdjson_really_inline basic_writer(char *buffer, size_t position) noexcept
      : ptr(buffer), pos(position) {}

  simdjson_really_inline bool ensure(size_t) const noexcept { return true; }
};

using writer = basic_writer<true>;
using unchecked_writer = basic_writer<false>;

// Bytes reserved past the size bound for an unchecked writer: it may then
// write a little past the end of what it produces (e.g., copy keys as whole
// 16-byte blocks).
inline constexpr size_t unchecked_slack = 64;

consteval size_t padded_key_length(size_t length) {
  return (length + 15) / 16 * 16;
}

// === Helper: invoke a string_builder member that writes variable-length
// content (escape_and_append_with_quotes etc), syncing the writer's local
// state before the call and reloading after. Used for string fields where
// rewriting the entire SIMD escape path through the writer would be a much
// bigger refactor. f may be user code (a with<Adapter> serializer) that
// throws: the exception then propagates to the caller.
template <class W, class F>
simdjson_really_inline void call_through_string_builder(W &w, F &&f) noexcept(noexcept(f(w.sb))) {
  w.sync();
  f(w.sb);
  w.ptr = w.sb.unsafe_data();
  w.pos = w.sb.unsafe_position();
  w.cap = w.sb.unsafe_capacity();
}

// Helpers implementing the serialization side of the annotations (see
// simdjson/annotations.h) for reflected structures.
namespace annotation_detail {

// A member is serialized unless it is annotated with skip or skip_serializing.
consteval bool is_serialized_member(std::meta::info dm) {
  return !simdjson::detail::has_annotation(dm, ^^simdjson::detail::skip_tag)
      && !simdjson::detail::has_annotation(dm, ^^simdjson::detail::skip_serializing_tag);
}

// False when the member has a skip_serializing_if<pred> annotation and
// pred(value) is true.
template <auto dm, typename V>
simdjson_really_inline bool should_serialize(const V &value) {
  constexpr std::meta::info skip_if_type = simdjson::detail::annotation_of_template(dm, ^^simdjson::detail::skip_serializing_if_t);
  if constexpr (skip_if_type != std::meta::info{}) {
    using skip_if = typename [: skip_if_type :];
    return !skip_if::predicate(value);
  } else {
    (void)value;
    return true;
  }
}

// Serialize a member value, through its with<Adapter> annotation when the
// adapter provides a serialize function.
template <auto dm, class W, typename V>
simdjson_really_inline void atom_member(W &w, const V &value) {
  constexpr std::meta::info with_type = simdjson::detail::annotation_of_template(dm, ^^simdjson::detail::with_t);
  if constexpr (with_type != std::meta::info{}) {
    using adapter = typename [: with_type :]::adapter;
    if constexpr (requires(string_builder &b) { adapter::serialize(b, value); }) {
      call_through_string_builder(w, [&](string_builder &b) { adapter::serialize(b, value); });
    } else {
      atom(w, value);
    }
  } else {
    atom(w, value);
  }
}

// Write the "key":value pairs of the members of t (without the braces), each
// preceded by a comma unless it is the first one. The members of a member
// annotated with flatten are written in its place.
template <class W, class T>
simdjson_really_inline void atom_fields(W &w, const T &t, bool &first) {
  // Per-field block: ensure key+value worst case, then write key + value
  // through the writer's local pos. For arithmetic fields, the integer
  // write happens directly via write_uint_jeaiii on w.ptr+w.pos, so pos
  // never round-trips through memory.
  template for (constexpr auto dm : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
    if constexpr (is_serialized_member(dm)) {
      if (should_serialize<dm>(t.[:dm:])) {
        if constexpr (simdjson::detail::has_annotation(dm, ^^simdjson::detail::flatten_tag)) {
          static_assert(std::meta::is_class_type(simdjson::detail::flattened_type(dm)));
          using flattened = std::remove_cvref_t<decltype(t.[:dm:])>;
          static_assert(!concepts::container_but_not_string<flattened> && !concepts::string_view_keyed_map<flattened> &&
                        !concepts::appendable_containers<flattened> && !concepts::optional_type<flattened> &&
                        !concepts::smart_pointer<flattened> && !std::is_same_v<flattened, std::string> &&
                        !std::is_same_v<flattened, std::string_view> && !require_custom_serialization<flattened>,
                        "simdjson::flatten requires a member whose type is a structure serialized member by member");
          atom_fields(w, t.[:dm:], first);
        } else {
          // Copy the key as whole 16-byte blocks from a zero-padded copy (one
          // load and one store); ensure() reserves the padded length, and the
          // unchecked writer has slack past its bound.
          constexpr const char* key_name = simdjson::get_json_key_name<dm>();
          constexpr size_t first_key_len = constevalutil::consteval_to_quoted_escaped(key_name).size() + 1;
          constexpr size_t rest_key_len = first_key_len + 1;
          constexpr auto first_key = std::define_static_string(
              constevalutil::consteval_to_quoted_escaped(key_name) + ":" +
              std::string(padded_key_length(first_key_len) - first_key_len, '\0'));
          constexpr auto rest_key = std::define_static_string(
              std::string(",") + constevalutil::consteval_to_quoted_escaped(key_name) + ":" +
              std::string(padded_key_length(rest_key_len) - rest_key_len, '\0'));
          if (!w.ensure(padded_key_length(rest_key_len))) { return; }
          if (first) {
            std::memcpy(w.ptr + w.pos, first_key, padded_key_length(first_key_len));
            w.pos += first_key_len;
          } else {
            std::memcpy(w.ptr + w.pos, rest_key, padded_key_length(rest_key_len));
            w.pos += rest_key_len;
          }
          first = false;
          atom_member<dm>(w, t.[:dm:]);
        }
      }
    }
  };
}

} // namespace annotation_detail

template <class W, class T>
  requires(concepts::container_but_not_string<T> && !require_custom_serialization<T>)
simdjson_really_inline constexpr void atom(W &w, const T &t) {
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

template <class W, class T>
  requires(std::is_same_v<T, std::string> ||
           std::is_same_v<T, std::string_view> ||
           std::is_same_v<T, const char *> ||
           std::is_same_v<T, char>)
simdjson_really_inline constexpr void atom(W &w, const T &t) {
  // Inline the escape path through the writer so we never round-trip
  // pos through memory for string fields (Twitter is dominated by
  // these -- sync/reload around each string was a real cost).
  std::string_view input;
  if constexpr (std::is_same_v<T, char>) {
    input = std::string_view(&t, 1);
  } else {
    input = std::string_view(t);
  }
  // Worst-case escape: every byte expands to \uXXXX (6 chars), plus 2 quotes.
  // Guard against w.pos + 2 + 6 * input.size() wrapping for huge inputs -- if
  // it wrapped to a small value, ensure() would spuriously succeed and the
  // subsequent escape would overflow the buffer. max - w.pos cannot wrap, and
  // size < (max - pos) / 6 implies pos + 6 * size + 6 <= max.
  // Note that this is pedantic except maybe on 32-bit targets.
  if constexpr (W::checked) {
    if (simdjson_unlikely(input.size() >= ((std::numeric_limits<size_t>::max)() - w.pos) / 6)) { return; }
    if (!w.ensure(2 + 6 * input.size())) { return; }
  }
  w.ptr[w.pos++] = '"';
  w.pos += write_string_escaped(input, w.ptr + w.pos);
  w.ptr[w.pos++] = '"';
}

template <class W, concepts::string_view_keyed_map T>
  requires(!require_custom_serialization<T>)
simdjson_really_inline constexpr void atom(W &w, const T &m) {
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
    // Guard against w.pos + 3 + 6 * key_sv.size() wrapping for huge keys, if
    // it wrapped to a small value, ensure() would spuriously succeed and the
    // subsequent escape would overflow the buffer. max - w.pos cannot wrap, and
    // size < (max - pos) / 6 implies pos + 6 * size + 6 <= max.
    // Note that this is pedantic except maybe on 32-bit targets.
    if constexpr (W::checked) {
      if (simdjson_unlikely(key_sv.size() >= ((std::numeric_limits<size_t>::max)() - w.pos) / 6)) { return; }
      if (!w.ensure(2 + 6 * key_sv.size() + 1)) { return; }
    }
    w.ptr[w.pos++] = '"';
    w.pos += write_string_escaped(key_sv, w.ptr + w.pos);
    w.ptr[w.pos++] = '"';
    w.ptr[w.pos++] = ':';
    atom(w, value);
  }
  if (!w.ensure(1)) return;
  w.ptr[w.pos++] = '}';
}


template<class W, typename number_type,
         typename = typename std::enable_if<std::is_arithmetic<number_type>::value && !std::is_same_v<number_type, char>>::type>
simdjson_really_inline constexpr void atom(W &w, const number_type t) {
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
    if constexpr (W::checked) {
      call_through_string_builder(w, [&](string_builder &b) { b.append(t); });
    } else {
      w.pos = size_t(internal::write_double(w.ptr + w.pos, double(t)) - w.ptr);
    }
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

template <class W, class T>
  requires(std::is_class_v<T> && !concepts::container_but_not_string<T> &&
           !concepts::string_view_keyed_map<T> &&
           !concepts::optional_type<T> &&
           !concepts::smart_pointer<T> &&
           !concepts::appendable_containers<T> &&
           !std::is_same_v<T, std::string> &&
           !std::is_same_v<T, std::string_view> &&
           !std::is_same_v<T, const char*> &&
           !std::is_same_v<T, char> && !require_custom_serialization<T>)
simdjson_really_inline constexpr void atom(W &w, const T &t) {
  if constexpr (simdjson::detail::has_annotation(^^T, ^^simdjson::detail::transparent_tag)) {
    // A transparent structure is serialized as its single member.
    constexpr auto dm = simdjson::detail::transparent_member(^^T);
    annotation_detail::atom_member<dm>(w, t.[:dm:]);
  } else {
    bool first = true;
    if (!w.ensure(1)) { return; }
    w.ptr[w.pos++] = '{';
    annotation_detail::atom_fields(w, t, first);
    if (!w.ensure(1)) { return; }
    w.ptr[w.pos++] = '}';
  }
}

// Support for optional types (std::optional, etc.)
template <class W, concepts::optional_type T>
  requires(!require_custom_serialization<T>)
simdjson_really_inline constexpr void atom(W &w, const T &opt) {
  if (opt) {
    atom(w, opt.value());
  } else {
    if (!w.ensure(4)) return;
    std::memcpy(w.ptr + w.pos, "null", 4);
    w.pos += 4;
  }
}

// Support for smart pointers (std::unique_ptr, std::shared_ptr, etc.)
template <class W, concepts::smart_pointer T>
  requires(!require_custom_serialization<T>)
simdjson_really_inline constexpr void atom(W &w, const T &ptr) {
  if (ptr) {
    atom(w, *ptr);
  } else {
    if (!w.ensure(4)) return;
    std::memcpy(w.ptr + w.pos, "null", 4);
    w.pos += 4;
  }
}

// Support for enums - serialize as string representation using expand approach from P2996R12
template <class W, typename T>
  requires(std::is_enum_v<T> && !require_custom_serialization<T>)
simdjson_really_inline void atom(W &w, const T &e) {
#if SIMDJSON_STATIC_REFLECTION
  static constexpr auto enumerators = std::define_static_array(std::meta::enumerators_of(^^T));
  template for (constexpr auto enum_val : enumerators) {
    constexpr auto enum_str = std::define_static_string(constevalutil::consteval_to_quoted_escaped(simdjson::get_json_key_name<enum_val>()));
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
template <class W, concepts::appendable_containers T>
  requires(!concepts::container_but_not_string<T> && !concepts::string_view_keyed_map<T> &&
           !concepts::optional_type<T> && !concepts::smart_pointer<T> &&
           !std::is_same_v<T, std::string> &&
           !std::is_same_v<T, std::string_view> && !std::is_same_v<T, const char*> && !require_custom_serialization<T>)
simdjson_really_inline constexpr void atom(W &w, const T &container) {
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

// =============================================================
// Size bound: an upper bound on the number of bytes that atom(w, t) writes.
// Computing it first lets append() reserve the capacity once and then run
// the whole write chain through an unchecked_writer, without a capacity
// check before every write. It mirrors the atom() overloads above.
// =============================================================
namespace bound_detail {

// Whether size_bound covers everything that atom() writes for T: not when a
// member is serialized by a with<Adapter> serializer, which writes an unknown
// amount through the string_builder.
template <class T>
consteval bool is_bounded() {
  if constexpr (require_custom_serialization<T>) {
    return false;
  } else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> ||
                       std::is_same_v<T, const char *> || std::is_arithmetic_v<T> || std::is_enum_v<T>) {
    return true;
  } else if constexpr (concepts::optional_type<T> || concepts::smart_pointer<T>) {
    return is_bounded<std::remove_cvref_t<decltype(*std::declval<const T &>())>>();
  } else if constexpr (concepts::string_view_keyed_map<T>) {
    return is_bounded<std::remove_cvref_t<typename T::mapped_type>>();
  } else if constexpr (concepts::container_but_not_string<T> || concepts::appendable_containers<T>) {
    return is_bounded<std::remove_cvref_t<std::ranges::range_value_t<T>>>();
  } else {
    bool bounded = true;
    template for (constexpr auto dm : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
      if constexpr (annotation_detail::is_serialized_member(dm)) {
        bounded = bounded && simdjson::detail::annotation_of_template(dm, ^^simdjson::detail::with_t) == std::meta::info{} &&
                  is_bounded<std::remove_cvref_t<decltype(std::declval<const T &>().[:dm:])>>();
      }
    };
    return bounded;
  }
}

template <class T>
consteval size_t enum_bound() {
  size_t bound = 20; // the integer fallback
  template for (constexpr auto enum_val : std::define_static_array(std::meta::enumerators_of(^^T))) {
    constexpr size_t len = std::char_traits<char>::length(std::define_static_string(
        constevalutil::consteval_to_quoted_escaped(simdjson::get_json_key_name<enum_val>())));
    bound = (std::max)(bound, len);
  };
  return bound;
}

template <class T>
simdjson_really_inline size_t size_bound(const T &t) noexcept;

// Bound for the "key":value pairs of a structure, commas included.
template <class T>
simdjson_really_inline size_t fields_bound(const T &t) noexcept {
  size_t bound = 0;
  template for (constexpr auto dm : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
    if constexpr (annotation_detail::is_serialized_member(dm)) {
      if constexpr (simdjson::detail::has_annotation(dm, ^^simdjson::detail::flatten_tag)) {
        bound += fields_bound(t.[:dm:]);
      } else {
        constexpr size_t rest_key_len = constevalutil::consteval_to_quoted_escaped(simdjson::get_json_key_name<dm>()).size() + 2;
        bound += rest_key_len + size_bound(t.[:dm:]);
      }
    }
  };
  return bound;
}

template <class T>
simdjson_really_inline size_t size_bound([[maybe_unused]] const T &t) noexcept {
  if constexpr (std::is_same_v<T, char>) {
    return 2 + 6;
  } else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> ||
                       std::is_same_v<T, const char *>) {
    // Every byte may become \uXXXX, plus the quotes.
    return 2 + 6 * std::string_view(t).size();
  } else if constexpr (std::is_same_v<T, bool>) {
    return 5;
  } else if constexpr (std::is_floating_point_v<T>) {
    return simdjson::internal::to_chars_buffer_size;
  } else if constexpr (std::is_arithmetic_v<T>) {
    return 20;
  } else if constexpr (std::is_enum_v<T>) {
    return enum_bound<T>();
  } else if constexpr (concepts::optional_type<T> || concepts::smart_pointer<T>) {
    return t ? size_bound(*t) : 4;
  } else if constexpr (concepts::string_view_keyed_map<T>) {
    size_t bound = 2;
    for (const auto &[key, value] : t) {
      // comma, quotes, colon
      bound += 4 + 6 * std::string_view(key).size() + size_bound(value);
    }
    return bound;
  } else if constexpr (concepts::container_but_not_string<T> || concepts::appendable_containers<T>) {
    using value_type = std::remove_cvref_t<std::ranges::range_value_t<T>>;
    if constexpr (std::is_arithmetic_v<value_type> && !std::is_same_v<value_type, char>) {
      // A fixed bound per element: no need to visit them.
      return 2 + size_t(std::ranges::distance(t)) * (1 + size_bound(value_type{}));
    } else {
      size_t bound = 2;
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC novector // a vector loop is slower on short containers
#endif
#pragma GCC unroll 4
      for (const auto &item : t) {
        bound += 1 + size_bound(item);
      }
      return bound;
    }
  } else if constexpr (simdjson::detail::has_annotation(^^T, ^^simdjson::detail::transparent_tag)) {
    constexpr auto dm = simdjson::detail::transparent_member(^^T);
    return size_bound(t.[:dm:]);
  } else {
    return 2 + fields_bound(t);
  }
}

} // namespace bound_detail

// Write t through an unchecked writer when its size bound is available,
// reserving that many bytes first, and through the checked writer otherwise.
template <class T>
simdjson_really_inline void append_bounded(string_builder &b, const T &t) {
  // On 32-bit systems, the bound could overflow: keep the checked writer.
  if constexpr (sizeof(size_t) >= 8 && bound_detail::is_bounded<T>()) {
    const size_t bound = bound_detail::size_bound(t) + unchecked_slack;
    const size_t pos = b.unsafe_position();
    // The bound is a sum of in-memory sizes times a small constant: it cannot
    // overflow on a 64-bit system. Be pedantic elsewhere.
    if (sizeof(size_t) >= 8 || bound <= (std::numeric_limits<size_t>::max)() - pos) {
      const size_t cap = b.unsafe_capacity();
      // Grow geometrically so that many small appends stay amortized.
      if (pos + bound <= cap || b.unsafe_grow((std::max)(cap * 2, pos + bound))) {
        unchecked_writer w(b.unsafe_data(), pos);
        atom(w, t);
        b.unsafe_set_position(w.pos);
      }
      return;
    }
  }
  writer w(b);
  atom(w, t);
  w.sync();
}

// append() -- top-level entry. Each overload constructs a stack-local
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
  append_bounded(b, t);
}

template <concepts::smart_pointer T>
  requires(!require_custom_serialization<T>)
simdjson_inline void append(string_builder &b, const T &t) {
  append_bounded(b, t);
}

template <concepts::appendable_containers T>
  requires(!concepts::container_but_not_string<T> && !concepts::string_view_keyed_map<T> &&
           !concepts::optional_type<T> && !concepts::smart_pointer<T> &&
           !std::is_same_v<T, std::string> &&
           !std::is_same_v<T, std::string_view> && !std::is_same_v<T, const char*> && !require_custom_serialization<T>)
simdjson_inline void append(string_builder &b, const T &t) {
  append_bounded(b, t);
}

template <concepts::string_view_keyed_map T>
  requires(!require_custom_serialization<T>)
simdjson_inline void append(string_builder &b, const T &t) {
  append_bounded(b, t);
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
  append_bounded(b, z);
}

// works for container that have begin() and end() iterators
template <class Z>
  requires(concepts::container_but_not_string<Z> && !require_custom_serialization<Z>)
simdjson_inline void append(string_builder &b, const Z &z) {
  append_bounded(b, z);
}

template <class Z>
  requires (require_custom_serialization<Z>)
void append(string_builder &b, const Z &z) {
  b.append(z);
}


template <class Z>
simdjson_warn_unused error_code to_json(const Z &z, std::string &s, size_t initial_capacity = string_builder::DEFAULT_INITIAL_CAPACITY) {
  if constexpr (sizeof(size_t) >= 8 && bound_detail::is_bounded<Z>()) {
    // Write straight into s, sized by the bound: no intermediate buffer, no copy.
    (void)initial_capacity;
    const size_t bound = bound_detail::size_bound(z) + unchecked_slack;
    auto write = [&z](char *p) noexcept {
      unchecked_writer w(p, 0);
      atom(w, z);
      return w.pos;
    };
#if defined(__cpp_lib_string_resize_and_overwrite) && __cpp_lib_string_resize_and_overwrite >= 202110L
    s.resize_and_overwrite(bound, [&write](char *p, size_t) noexcept { return write(p); });
#else
    s.resize(bound);
    s.resize(write(s.data()));
#endif
    return SUCCESS;
  } else {
    string_builder b(initial_capacity);
    append(b, z);
    std::string_view view;
    if(auto e = b.view().get(view); e) { return e; }
    s.assign(view);
    return SUCCESS;
  }
}

template <class Z>
simdjson_warn_unused simdjson_result<std::string> to_json_string(const Z &z, size_t initial_capacity = string_builder::DEFAULT_INITIAL_CAPACITY) {
  std::string s;
  if(auto e = to_json(z, s, initial_capacity); e) { return e; }
  return s;
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

SIMDJSON_POP_DISABLE_WARNINGS

} // namespace builder
} // namespace SIMDJSON_IMPLEMENTATION
// Alias the function template to 'to' in the global namespace
template <class Z>
simdjson_warn_unused simdjson_result<std::string> to_json(const Z &z, size_t initial_capacity = SIMDJSON_IMPLEMENTATION::builder::string_builder::DEFAULT_INITIAL_CAPACITY) {
  return SIMDJSON_IMPLEMENTATION::builder::to_json_string(z, initial_capacity);
}
template <class Z>
simdjson_warn_unused error_code to_json(const Z &z, std::string &s, size_t initial_capacity = SIMDJSON_IMPLEMENTATION::builder::string_builder::DEFAULT_INITIAL_CAPACITY) {
  return SIMDJSON_IMPLEMENTATION::builder::to_json(z, s, initial_capacity);
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