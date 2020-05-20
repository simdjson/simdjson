#ifndef SIMDJSON_INLINE_DOCUMENT_H
#define SIMDJSON_INLINE_DOCUMENT_H

// Inline implementations go in here.

#include "simdjson/dom/document.h"
#include "simdjson/dom/element.h"
#include "simdjson/internal/tape_ref.h"
#include "simdjson/internal/jsonformatutils.h"
#include <ostream>
#include <cstring>

namespace simdjson {
namespace dom {

//
// document inline implementation
//
inline element document::root() const noexcept {
  return element(this, 1);
}

WARN_UNUSED
inline error_code document::allocate(size_t capacity) noexcept {
  if (capacity == 0) {
    string_buf.reset();
    tape.reset();
    return SUCCESS;
  }

  // a pathological input like "[[[[..." would generate len tape elements, so
  // need a capacity of at least len + 1, but it is also possible to do
  // worse with "[7,7,7,7,6,7,7,7,6,7,7,6,[7,7,7,7,6,7,7,7,6,7,7,6,7,7,7,7,7,7,6"
  //where len + 1 tape elements are
  // generated, see issue https://github.com/lemire/simdjson/issues/345
  size_t tape_capacity = ROUNDUP_N(capacity + 2, 64);
  // a document with only zero-length strings... could have len/3 string
  // and we would need len/3 * 5 bytes on the string buffer
  size_t string_capacity = ROUNDUP_N(5 * capacity / 3 + 32, 64);
  string_buf.reset( new (std::nothrow) uint8_t[string_capacity]);
  tape.reset(new (std::nothrow) uint64_t[tape_capacity]);
  return string_buf && tape ? SUCCESS : MEMALLOC;
}

inline bool document::dump_raw_tape(std::ostream &os) const noexcept {
  uint32_t string_length;
  size_t tape_idx = 0;
  uint64_t tape_val = tape[tape_idx];
  uint8_t type = uint8_t(tape_val >> 56);
  os << tape_idx << " : " << type;
  tape_idx++;
  size_t how_many = 0;
  if (type == 'r') {
    how_many = size_t(tape_val & internal::JSON_VALUE_MASK);
  } else {
    // Error: no starting root node?
    return false;
  }
  os << "\t// pointing to " << how_many << " (right after last node)\n";
  uint64_t payload;
  for (; tape_idx < how_many; tape_idx++) {
    os << tape_idx << " : ";
    tape_val = tape[tape_idx];
    payload = tape_val & internal::JSON_VALUE_MASK;
    type = uint8_t(tape_val >> 56);
    switch (type) {
    case '"': // we have a string
      os << "string \"";
      memcpy(&string_length, string_buf.get() + payload, sizeof(uint32_t));
      os << internal::escape_json_string(std::string_view(
        (const char *)(string_buf.get() + payload + sizeof(uint32_t)),
        string_length
      ));
      os << '"';
      os << '\n';
      break;
    case 'l': // we have a long int
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << "integer " << static_cast<int64_t>(tape[++tape_idx]) << "\n";
      break;
    case 'u': // we have a long uint
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      os << "unsigned integer " << tape[++tape_idx] << "\n";
      break;
    case 'd': // we have a double
      os << "float ";
      if (tape_idx + 1 >= how_many) {
        return false;
      }
      double answer;
      memcpy(&answer, &tape[++tape_idx], sizeof(answer));
      os << answer << '\n';
      break;
    case 'n': // we have a null
      os << "null\n";
      break;
    case 't': // we have a true
      os << "true\n";
      break;
    case 'f': // we have a false
      os << "false\n";
      break;
    case '{': // we have an object
      os << "{\t// pointing to next tape location " << payload
         << " (first node after the scope) \n";
      break;
    case '}': // we end an object
      os << "}\t// pointing to previous tape location " << payload
         << " (start of the scope) \n";
      break;
    case '[': // we start an array
      os << "[\t// pointing to next tape location " << payload
         << " (first node after the scope) \n";
      break;
    case ']': // we end an array
      os << "]\t// pointing to previous tape location " << payload
         << " (start of the scope) \n";
      break;
    case 'r': // we start and end with the root node
      // should we be hitting the root node?
      return false;
    default:
      return false;
    }
  }
  tape_val = tape[tape_idx];
  payload = tape_val & internal::JSON_VALUE_MASK;
  type = uint8_t(tape_val >> 56);
  os << tape_idx << " : " << type << "\t// pointing to " << payload
     << " (start root)\n";
  return true;
}

} // namespace dom
<<<<<<< HEAD


//
// minify inline implementation
//

template<>
inline std::ostream& minify<dom::element>::print(std::ostream& out) {
  using tape_type=internal::tape_type;
  size_t depth = 0;
  constexpr size_t MAX_DEPTH = 16;
  bool is_object[MAX_DEPTH];
  is_object[0] = false;
  bool after_value = false;

  internal::tape_ref iter(value);
  do {
    // print commas after each value
    if (after_value) {
      out << ",";
    }
    // If we are in an object, print the next key and :, and skip to the next value.
    if (is_object[depth]) {
      out << '"' << internal::escape_json_string(iter.get_string_view()) << "\":";
      iter.json_index++;
    }
    switch (iter.tape_ref_type()) {

    // Arrays
    case tape_type::START_ARRAY: {
      // If we're too deep, we need to recurse to go deeper.
      depth++;
      if (unlikely(depth >= MAX_DEPTH)) {
        out << minify<dom::array>(dom::array(iter.doc, iter.json_index));
        iter.json_index = iter.matching_brace_index() - 1; // Jump to the ]
        depth--;
        break;
      }

      // Output start [
      out << '[';
      iter.json_index++;

      // Handle empty [] (we don't want to come back around and print commas)
      if (iter.tape_ref_type() == tape_type::END_ARRAY) {
        out << ']';
        depth--;
        break;
      }

      is_object[depth] = false;
      after_value = false;
      continue;
    }

    // Objects
    case tape_type::START_OBJECT: {
      // If we're too deep, we need to recurse to go deeper.
      depth++;
      if (unlikely(depth >= MAX_DEPTH)) {
        out << minify<dom::object>(dom::object(iter.doc, iter.json_index));
        iter.json_index = iter.matching_brace_index() - 1; // Jump to the }
        depth--;
        break;
      }

      // Output start {
      out << '{';
      iter.json_index++;

      // Handle empty {} (we don't want to come back around and print commas)
      if (iter.tape_ref_type() == tape_type::END_OBJECT) {
        out << '}';
        depth--;
        break;
      }

      is_object[depth] = true;
      after_value = false;
      continue;
    }

    // Scalars
    case tape_type::STRING:
      out << '"' << internal::escape_json_string(iter.get_string_view()) << '"';
      break;
    case tape_type::INT64:
      out << iter.next_tape_value<int64_t>();
      iter.json_index++; // numbers take up 2 spots, so we need to increment extra
      break;
    case tape_type::UINT64:
      out << iter.next_tape_value<uint64_t>();
      iter.json_index++; // numbers take up 2 spots, so we need to increment extra
      break;
    case tape_type::DOUBLE:
      out << iter.next_tape_value<double>();
      iter.json_index++; // numbers take up 2 spots, so we need to increment extra
      break;
    case tape_type::TRUE_VALUE:
      out << "true";
      break;
    case tape_type::FALSE_VALUE:
      out << "false";
      break;
    case tape_type::NULL_VALUE:
      out << "null";
      break;

    // These are impossible
     case tape_type::END_ARRAY:
     case tape_type::END_OBJECT:
     case tape_type::ROOT:
       out << "unexpected content!!!"; // abort() usage is forbidden in the library
    }
    iter.json_index++;
    after_value = true;

    // Handle multiple ends in a row
    while (depth != 0 && (iter.tape_ref_type() == tape_type::END_ARRAY || iter.tape_ref_type() == tape_type::END_OBJECT)) {
      out << char(iter.tape_ref_type());
      depth--;
      iter.json_index++;
    }

    // Stop when we're at depth 0
  } while (depth != 0);

  return out;
}
template<>
inline std::ostream& minify<dom::object>::print(std::ostream& out) {
  out << '{';
  auto pair = value.begin();
  auto end = value.end();
  if (pair != end) {
    out << minify<dom::key_value_pair>(*pair);
    for (++pair; pair != end; ++pair) {
      out << "," << minify<dom::key_value_pair>(*pair);
    }
  }
  return out << '}';
}
template<>
inline std::ostream& minify<dom::array>::print(std::ostream& out) {
  out << '[';
  auto iter = value.begin();
  auto end = value.end();
  if (iter != end) {
    out << minify<dom::element>(*iter);
    for (++iter; iter != end; ++iter) {
      out << "," << minify<dom::element>(*iter);
    }
  }
  return out << ']';
}
template<>
inline std::ostream& minify<dom::key_value_pair>::print(std::ostream& out) {
  return out << '"' << internal::escape_json_string(value.key) << "\":" << value.value;
}

#if SIMDJSON_EXCEPTIONS

template<>
inline std::ostream& minify<simdjson_result<dom::element>>::print(std::ostream& out) {
  if (value.error()) { throw simdjson_error(value.error()); }
  return out << minify<dom::element>(value.first);
}
template<>
inline std::ostream& minify<simdjson_result<dom::array>>::print(std::ostream& out) {
  if (value.error()) { throw simdjson_error(value.error()); }
  return out << minify<dom::array>(value.first);
}
template<>
inline std::ostream& minify<simdjson_result<dom::object>>::print(std::ostream& out) {
  if (value.error()) { throw simdjson_error(value.error()); }
  return out << minify<dom::object>(value.first);
}

#endif


namespace internal {

//
// tape_ref inline implementation
//
really_inline tape_ref::tape_ref() noexcept : doc{nullptr}, json_index{0} {}
really_inline tape_ref::tape_ref(const document *_doc, size_t _json_index) noexcept : doc{_doc}, json_index{_json_index} {}



// Some value types have a specific on-tape word value. It can be faster
// to check the type by doing a word-to-word comparison instead of extracting the
// most significant 8 bits.

really_inline bool tape_ref::is_double() const noexcept {
  constexpr uint64_t tape_double = uint64_t(tape_type::DOUBLE)<<56;
  return doc->tape[json_index] == tape_double;
}
really_inline bool tape_ref::is_int64() const noexcept {
  constexpr uint64_t tape_int64 = uint64_t(tape_type::INT64)<<56;
  return doc->tape[json_index] == tape_int64;
}
really_inline bool tape_ref::is_uint64() const noexcept {
  constexpr uint64_t tape_uint64 = uint64_t(tape_type::UINT64)<<56;
  return doc->tape[json_index] == tape_uint64;
}
really_inline bool tape_ref::is_false() const noexcept {
  constexpr uint64_t tape_false = uint64_t(tape_type::FALSE_VALUE)<<56;
  return doc->tape[json_index] == tape_false;
}
really_inline bool tape_ref::is_true() const noexcept {
  constexpr uint64_t tape_true = uint64_t(tape_type::TRUE_VALUE)<<56;
  return doc->tape[json_index] == tape_true;
}
really_inline bool tape_ref::is_null_on_tape() const noexcept {
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
really_inline tape_type tape_ref::tape_ref_type() const noexcept {
  return static_cast<tape_type>(doc->tape[json_index] >> 56);
}
really_inline uint64_t internal::tape_ref::tape_value() const noexcept {
  return doc->tape[json_index] & internal::JSON_VALUE_MASK;
}
really_inline uint32_t internal::tape_ref::matching_brace_index() const noexcept {
  return uint32_t(doc->tape[json_index]);
}
really_inline uint32_t internal::tape_ref::scope_count() const noexcept {
  return uint32_t((doc->tape[json_index] >> 32) & internal::JSON_COUNT_MASK);
}

template<typename T>
really_inline T tape_ref::next_tape_value() const noexcept {
  static_assert(sizeof(T) == sizeof(uint64_t), "next_tape_value() template parameter must be 64-bit");
  // Though the following is tempting...
  //  return *reinterpret_cast<const T*>(&doc->tape[json_index + 1]);
  // It is not generally safe. It is safer, and often faster to rely
  // on memcpy. Yes, it is uglier, but it is also encapsulated.
  T x;
  memcpy(&x,&doc->tape[json_index + 1],sizeof(uint64_t));
  return x;
}

really_inline uint32_t internal::tape_ref::get_string_length() const noexcept {
  uint64_t string_buf_index = size_t(tape_value());
  uint32_t len;
  memcpy(&len, &doc->string_buf[string_buf_index], sizeof(len));
  return len;
}

really_inline const char * internal::tape_ref::get_c_str() const noexcept {
  uint64_t string_buf_index = size_t(tape_value());
  return reinterpret_cast<const char *>(&doc->string_buf[string_buf_index + sizeof(uint32_t)]);
}

inline std::string_view internal::tape_ref::get_string_view() const noexcept {
  return std::string_view(
      get_c_str(),
      get_string_length()
  );
}

} // namespace internal
=======
>>>>>>> master
} // namespace simdjson

#endif // SIMDJSON_INLINE_DOCUMENT_H
