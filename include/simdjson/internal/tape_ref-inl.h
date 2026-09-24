#ifndef SIMDJSON_TAPE_REF_INL_H
#define SIMDJSON_TAPE_REF_INL_H

#include "simdjson/dom/document.h"
#include "simdjson/internal/tape_ref.h"
#include "simdjson/internal/tape_type.h"

#include <cstring>

namespace simdjson {
namespace internal {

constexpr const uint64_t JSON_VALUE_MASK = 0x00FFFFFFFFFFFFFF;
constexpr const uint32_t JSON_COUNT_MASK = 0xFFFFFF;

//
// tape_ref inline implementation
//
simdjson_inline tape_ref::tape_ref() noexcept : doc{nullptr}, json_index{0} {}
simdjson_inline tape_ref::tape_ref(const dom::document *_doc, size_t _json_index) noexcept : doc{_doc}, json_index{_json_index} {}


simdjson_inline bool tape_ref::is_document_root() const noexcept {
  return json_index == 1; // should we ever change the structure of the tape, this should get updated.
}
simdjson_inline bool tape_ref::usable() const noexcept {
  return doc != nullptr; // when the document pointer is null, this tape_ref is uninitialized (should not be accessed).
}
// Some value types have a specific on-tape word value. It can be faster
// to check the type by doing a word-to-word comparison instead of extracting the
// most significant 8 bits.

simdjson_inline bool tape_ref::is_double() const noexcept {
  constexpr uint64_t tape_double = uint64_t(tape_type::DOUBLE)<<56;
  return doc->tape[json_index] == tape_double;
}
simdjson_inline bool tape_ref::is_int64() const noexcept {
  constexpr uint64_t tape_int64 = uint64_t(tape_type::INT64)<<56;
  return doc->tape[json_index] == tape_int64;
}
simdjson_inline bool tape_ref::is_uint64() const noexcept {
  constexpr uint64_t tape_uint64 = uint64_t(tape_type::UINT64)<<56;
  return doc->tape[json_index] == tape_uint64;
}
simdjson_inline bool tape_ref::is_false() const noexcept {
  constexpr uint64_t tape_false = uint64_t(tape_type::FALSE_VALUE)<<56;
  return doc->tape[json_index] == tape_false;
}
simdjson_inline bool tape_ref::is_true() const noexcept {
  constexpr uint64_t tape_true = uint64_t(tape_type::TRUE_VALUE)<<56;
  return doc->tape[json_index] == tape_true;
}
simdjson_inline bool tape_ref::is_null_on_tape() const noexcept {
  constexpr uint64_t tape_null = uint64_t(tape_type::NULL_VALUE)<<56;
  return doc->tape[json_index] == tape_null;
}

inline size_t tape_ref::after_element() const noexcept {
  switch (tape_ref_type()) {
    case tape_type::START_ARRAY:
    case tape_type::START_OBJECT:
      return matching_brace_index();
    case tape_type::UINT64:
    case tape_type::INT64:
    case tape_type::DOUBLE:
      return json_index + 2;
    default:
      return json_index + 1;
  }
}
simdjson_inline tape_type tape_ref::tape_ref_type() const noexcept {
  return static_cast<tape_type>(doc->tape[json_index] >> 56);
}
simdjson_inline size_t tape_ref::before_element(size_t array_start) const noexcept {
  SIMDJSON_DEVELOPMENT_ASSERT(usable());
  SIMDJSON_DEVELOPMENT_ASSERT(json_index > array_start);
  tape_ref previous(doc, json_index - 1);
  if (previous.json_index == array_start) { return array_start; }
  // An exact numeric marker cannot end an element unless it is itself the
  // payload of a number. In that case its header is immediately before it.
  if (previous.is_int64() || previous.is_uint64() || previous.is_double()) {
    return previous.json_index - 1;
  }
  tape_ref probe(doc, previous.json_index - 1);

  // Validate both container links before examining its contents. A candidate
  // opening tag preceded by an even run of numeric markers is a real tag,
  // not a numeric payload. Scan both candidate boundaries together so that a
  // forged opening tag inside a nested value cannot cause an unbounded detour.
  // For a real container, the opening probe visits preceding siblings. For a
  // numeric payload, the other probe does. Stopping at the shorter run bounds
  // the work by the array's immediate elements, rather than nested contents.
  const auto type = previous.tape_ref_type();
  if (type == tape_type::END_ARRAY || type == tape_type::END_OBJECT) {
    const size_t start = previous.matching_brace_index();
    if (start > array_start && start < previous.json_index) {
      tape_ref opening(doc, start);
      const auto expected = type == tape_type::END_ARRAY
          ? tape_type::START_ARRAY : tape_type::START_OBJECT;
      if (opening.tape_ref_type() == expected &&
          opening.matching_brace_index() == previous.json_index + 1) {
        tape_ref before_opening(doc, start - 1);
        while ((before_opening.is_int64() || before_opening.is_uint64() ||
                before_opening.is_double()) &&
               (probe.is_int64() || probe.is_uint64() || probe.is_double())) {
          --before_opening.json_index;
          --probe.json_index;
        }
        if (!before_opening.is_int64() && !before_opening.is_uint64() &&
            !before_opening.is_double() &&
            (start - before_opening.json_index) % 2 == 1) {
          return start;
        }
      }
    }
  }

  // Numeric payloads can have ANY bit pattern, including another type's tag.
  // A run of exact numeric markers starts with a header, then alternates
  // between payload and header. An odd run before this word makes it a payload.
  // Subsequent reverse increments through exact-marker payloads take the
  // constant-time numeric-payload branch above.
  while (probe.is_int64() || probe.is_uint64() || probe.is_double()) {
    --probe.json_index;
  }
  if ((previous.json_index - probe.json_index) % 2 == 0) {
    return previous.json_index - 1;
  }

  // Once distinguished from numeric payloads, closing container tags link
  // directly back to their opening tags.
  switch (previous.tape_ref_type()) {
    case tape_type::END_ARRAY:
    case tape_type::END_OBJECT:
      return previous.matching_brace_index();
    default:
      return previous.json_index;
  }
}
simdjson_inline uint64_t internal::tape_ref::tape_value() const noexcept {
  return doc->tape[json_index] & internal::JSON_VALUE_MASK;
}
simdjson_inline uint32_t internal::tape_ref::matching_brace_index() const noexcept {
  return uint32_t(doc->tape[json_index]);
}
simdjson_inline uint32_t internal::tape_ref::scope_count() const noexcept {
  return uint32_t((doc->tape[json_index] >> 32) & internal::JSON_COUNT_MASK);
}

template<typename T>
simdjson_inline T tape_ref::next_tape_value() const noexcept {
  static_assert(sizeof(T) == sizeof(uint64_t), "next_tape_value() template parameter must be 64-bit");
  // Though the following is tempting...
  //  return *reinterpret_cast<const T*>(&doc->tape[json_index + 1]);
  // It is not generally safe. It is safer, and often faster to rely
  // on memcpy. Yes, it is uglier, but it is also encapsulated.
  T x;
  std::memcpy(&x,&doc->tape[json_index + 1],sizeof(uint64_t));
  return x;
}

simdjson_inline uint32_t internal::tape_ref::get_string_length() const noexcept {
  size_t string_buf_index = size_t(tape_value());
  uint32_t len;
  std::memcpy(&len, &doc->string_buf[string_buf_index], sizeof(len));
  return len;
}

simdjson_inline const char * internal::tape_ref::get_c_str() const noexcept {
  size_t string_buf_index = size_t(tape_value());
  return reinterpret_cast<const char *>(&doc->string_buf[string_buf_index + sizeof(uint32_t)]);
}

inline std::string_view internal::tape_ref::get_string_view() const noexcept {
  return std::string_view(
      get_c_str(),
      get_string_length()
  );
}

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_TAPE_REF_INL_H
