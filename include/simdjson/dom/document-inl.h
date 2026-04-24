#ifndef SIMDJSON_DOCUMENT_INL_H
#define SIMDJSON_DOCUMENT_INL_H

// Inline implementations go in here.

#include "simdjson/dom/base.h"
#include "simdjson/dom/document.h"
#include "simdjson/dom/element-inl.h"
#include "simdjson/internal/tape_ref-inl.h"
#include "simdjson/internal/jsonformatutils.h"

#include <algorithm>
#include <cstring>

namespace simdjson {
namespace dom {

//
// document inline implementation
//
inline element document::root() const noexcept {
  return element(internal::tape_ref(this, 1));
}
simdjson_warn_unused
inline size_t document::capacity() const noexcept {
  return allocated_capacity;
}

simdjson_warn_unused
inline error_code document::allocate(size_t capacity) {
  if (capacity == 0) {
    string_buf.reset();
    tape.reset();
    allocated_capacity = 0;
    return SUCCESS;
  }
  if (capacity > SIMDJSON_MAXSIZE_BYTES) {
    return CAPACITY;
  }

  // a pathological input like "[[[[..." would generate capacity tape elements, so
  // need a capacity of at least capacity + 1, but it is also possible to do
  // worse with "[7,7,7,7,6,7,7,7,6,7,7,6,[7,7,7,7,6,7,7,7,6,7,7,6,7,7,7,7,7,7,6"
  //where capacity + 1 tape elements are
  // generated, see issue https://github.com/simdjson/simdjson/issues/345
  if(capacity + 3 < capacity) {
    return CAPACITY; // overflow, only happen on legacy 32-bit systems with very large capacity
  }
  size_t tape_capacity = SIMDJSON_ROUNDUP_N(capacity + 3, 64);
  // a document with only zero-length strings... could have capacity/3 string
  // and we would need capacity/3 * 5 bytes on the string buffer
  if(5 * (capacity / 3) + SIMDJSON_PADDING < SIMDJSON_PADDING) {
    return CAPACITY; // overflow, only happen on legacy 32-bit systems with very large capacity
  }
  size_t string_capacity = SIMDJSON_ROUNDUP_N(5 * (capacity / 3) + SIMDJSON_PADDING, 64);

  // Allocate into locals first (strong exception safety): if either
  // allocation throws, the original string_buf/tape/allocated_capacity are
  // untouched.
  auto new_string_buf = internal::make_allocated_buffer<uint8_t>(string_capacity, *_allocator);
  auto new_tape = internal::make_allocated_buffer<uint64_t>(tape_capacity, *_allocator);
  if (!new_string_buf || !new_tape) { return MEMALLOC; }

  string_buf = std::move(new_string_buf);
  tape = std::move(new_tape);
  // Inverse of the forward formulas above. The forward step uses
  // 5*floor(c/3); the inverse maximises c such that 5*floor(c/3) <= cap-PAD,
  // which is 3*floor((cap-PAD)/5) + 2.
  size_t cap_s = 3 * ((string_buf.capacity() - SIMDJSON_PADDING) / 5) + 2;
  size_t cap_t = tape.capacity() - 3;
  allocated_capacity = std::min(cap_t, cap_s);
  return SUCCESS;
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
      std::memcpy(&string_length, string_buf.get() + payload, sizeof(uint32_t));
      os << internal::escape_json_string(std::string_view(
        reinterpret_cast<const char *>(string_buf.get() + payload + sizeof(uint32_t)),
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
      std::memcpy(&answer, &tape[++tape_idx], sizeof(answer));
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
      os << "{\t// pointing to next tape location " << uint32_t(payload)
         << " (first node after the scope), "
         << " saturated count "
         << ((payload >> 32) & internal::JSON_COUNT_MASK)<< "\n";
      break;    case '}': // we end an object
      os << "}\t// pointing to previous tape location " << uint32_t(payload)
         << " (start of the scope)\n";
      break;
    case '[': // we start an array
      os << "[\t// pointing to next tape location " << uint32_t(payload)
         << " (first node after the scope), "
         << " saturated count "
         << ((payload >> 32) & internal::JSON_COUNT_MASK)<< "\n";
      break;
    case ']': // we end an array
      os << "]\t// pointing to previous tape location " << uint32_t(payload)
         << " (start of the scope)\n";
      break;
    case 'r': // we start and end with the root node
      // should we be hitting the root node?
      return false;
    case 'Z': // we have a big integer
      os << "bigint ";
      std::memcpy(&string_length, string_buf.get() + payload, sizeof(uint32_t));
      os << std::string_view(
        reinterpret_cast<const char *>(string_buf.get() + payload + sizeof(uint32_t)),
        string_length
      );
      os << '\n';
      break;
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
} // namespace simdjson

#endif // SIMDJSON_DOCUMENT_INL_H
