
#ifndef SIMDJSON_SERIALIZATION_INL_H
#define SIMDJSON_SERIALIZATION_INL_H

#include "simdjson/dom/serialization.h"

#include <cinttypes>
#include <type_traits>

namespace simdjson {
namespace dom {
inline bool parser::print_json(std::ostream &os) const noexcept {
  if (!valid) { return false; }
  simdjson::dom::string_builder<> sb;
  sb.append(doc.root());
  std::string_view answer = sb.str();
  os << answer;
  return true;
}
/***
 * Number utility functions
 **/


namespace {
char *fast_itoa(char *output, int64_t value) noexcept {
  // This is a standard implementation of itoa.
  // We first write in reverse order and then reverse.
  if(value < 0) {
    *output++ = '-';
    value = -value;
  }
  char *write_pointer = output;
  do {
    *write_pointer++ = char('0' + (value % 10));
    value /= 10;
  } while (value != 0);
  // then we reverse the result
  char *const answer = write_pointer;
  char *second_write_pointer = output;
  write_pointer -= 1;
  while (second_write_pointer < write_pointer) {
    char c1 = *write_pointer;
    char c2 = *second_write_pointer;
    *second_write_pointer = c1;
    *write_pointer = c2;
    write_pointer--;
    second_write_pointer++;
  }
  return answer;
}
char *fast_itoa(char *output, uint64_t value) noexcept {
  // This is a standard implementation of itoa.
  // We first write in reverse order and then reverse.
  char *write_pointer = output;
  do {
    *write_pointer++ = char('0' + (value % 10));
    value /= 10;
  } while (value != 0);
  // then we reverse the result
  char *const answer = write_pointer;
  char *second_write_pointer = output;
  write_pointer -= 1;
  while (second_write_pointer < write_pointer) {
    char c1 = *write_pointer;
    char c2 = *second_write_pointer;
    *second_write_pointer = c1;
    *write_pointer = c2;
    write_pointer--;
    second_write_pointer++;
  }
  return answer;
}
} // anonymous namespace


/***
 * Minifier/formatter code.
 **/

simdjson_really_inline void mini_formatter::number(uint64_t x) {
  char number_buffer[24];
  char *newp = fast_itoa(number_buffer, x);
  buffer.insert(buffer.end(), number_buffer, newp);
}

simdjson_really_inline void mini_formatter::number(int64_t x) {
  char number_buffer[24];
  char *newp = fast_itoa(number_buffer, x);
  buffer.insert(buffer.end(), number_buffer, newp);
}

simdjson_really_inline void mini_formatter::number(double x) {
  char number_buffer[24];
  // Currently, passing the nullptr to the second argument is
  // safe because our implementation does not check the second 
  // argument.
  char *newp = internal::to_chars(number_buffer, nullptr, x);
  buffer.insert(buffer.end(), number_buffer, newp);
}

simdjson_really_inline void mini_formatter::start_array() { one_char('['); }
simdjson_really_inline void mini_formatter::end_array() { one_char(']'); }
simdjson_really_inline void mini_formatter::start_object() { one_char('{'); }
simdjson_really_inline void mini_formatter::end_object() { one_char('}'); }
simdjson_really_inline void mini_formatter::comma() { one_char(','); }
simdjson_really_inline void mini_formatter::c_str(const char *c) {
  for (; *c != '\0'; c++) {
    one_char(*c);
  }
}

simdjson_really_inline void mini_formatter::true_atom() { 
  const char * s = "true";
  buffer.insert(buffer.end(), s, s + 4);
}
simdjson_really_inline void mini_formatter::false_atom() {
  const char * s = "false";
  buffer.insert(buffer.end(), s, s + 5);
}
simdjson_really_inline void mini_formatter::null_atom() {
  const char * s = "null";
  buffer.insert(buffer.end(), s, s + 4);
}
simdjson_really_inline void mini_formatter::one_char(char c) { buffer.push_back(c); }
simdjson_really_inline void mini_formatter::key(std::string_view unescaped) {
  string(unescaped);
  one_char(':');
}
simdjson_really_inline void mini_formatter::string(std::string_view unescaped) {
  one_char('\"');
  size_t i = 0;
  // fast path for the case where we have no control character, no ", and no backslash (fast path)
  for (; (i < unescaped.length()) && (uint8_t(unescaped[i]) > 0x1F) 
        && (unescaped[i] != '\"') && (unescaped[i] != '\\'); i++) {}
  buffer.insert(buffer.end(), unescaped.data(), unescaped.data() + i);
  // We caught a control character if we enter this loop (slow)
  for (; i < unescaped.length(); i++) {
    switch (unescaped[i]) {
    case '\"':
      {
        const char * s = "\\\"";
        buffer.insert(buffer.end(), s, s + 2);
      }
      break;
    case '\\':
      {
        const char * s = "\\\\";
        buffer.insert(buffer.end(), s, s + 2);
      }
      break;
    default:
      if (uint8_t(unescaped[i]) <= 0x1F) {
        const static char *escaped[] = {
            "\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005",
            "\\u0006", "\\u0007", "\\b",     "\\t",     "\\n",     "\\u000b",
            "\\f",     "\\r",     "\\u000e", "\\u000f", "\\u0010", "\\u0011",
            "\\u0012", "\\u0013", "\\u0014", "\\u0015", "\\u0016", "\\u0017",
            "\\u0018", "\\u0019", "\\u001a", "\\u001b", "\\u001c", "\\u001d",
            "\\u001e", "\\u001f"};
        c_str(escaped[uint8_t(unescaped[i])]);
      } else {
        one_char(unescaped[i]);
      }
    } // switch
  }   // for
  one_char('\"');
}

inline void mini_formatter::clear() {
  buffer.clear();
}

simdjson_really_inline std::string_view mini_formatter::str() const {
  return std::string_view(buffer.data(), buffer.size());
}


/***
 * String building code.
 **/

template <class serializer>
inline void string_builder<serializer>::append(dom::element value) {
  using tape_type = internal::tape_type;
  size_t depth = 0;
  constexpr size_t MAX_DEPTH = 16;
  bool is_object[MAX_DEPTH];
  is_object[0] = false;
  bool after_value = false;

  internal::tape_ref iter(value.tape);
  do {
    // print commas after each value
    if (after_value) {
      format.comma();
    }
    // If we are in an object, print the next key and :, and skip to the next
    // value.
    if (is_object[depth]) {
      format.key(iter.get_string_view());
      iter.json_index++;
    }
    switch (iter.tape_ref_type()) {

    // Arrays
    case tape_type::START_ARRAY: {
      // If we're too deep, we need to recurse to go deeper.
      depth++;
      if (simdjson_unlikely(depth >= MAX_DEPTH)) {
        append(dom::array(iter));
        iter.json_index = iter.matching_brace_index() - 1; // Jump to the ]
        depth--;
        break;
      }

      // Output start [
      format.start_array();
      iter.json_index++;

      // Handle empty [] (we don't want to come back around and print commas)
      if (iter.tape_ref_type() == tape_type::END_ARRAY) {
        format.end_array();
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
      if (simdjson_unlikely(depth >= MAX_DEPTH)) {
        append(dom::object(iter));
        iter.json_index = iter.matching_brace_index() - 1; // Jump to the }
        depth--;
        break;
      }

      // Output start {
      format.start_object();
      iter.json_index++;

      // Handle empty {} (we don't want to come back around and print commas)
      if (iter.tape_ref_type() == tape_type::END_OBJECT) {
        format.end_object();
        depth--;
        break;
      }

      is_object[depth] = true;
      after_value = false;
      continue;
    }

    // Scalars
    case tape_type::STRING:
      format.string(iter.get_string_view());
      break;
    case tape_type::INT64:
      format.number(iter.next_tape_value<int64_t>());
      iter.json_index++; // numbers take up 2 spots, so we need to increment
                         // extra
      break;
    case tape_type::UINT64:
      format.number(iter.next_tape_value<uint64_t>());
      iter.json_index++; // numbers take up 2 spots, so we need to increment
                         // extra
      break;
    case tape_type::DOUBLE:
      format.number(iter.next_tape_value<double>());
      iter.json_index++; // numbers take up 2 spots, so we need to increment
                         // extra
      break;
    case tape_type::TRUE_VALUE:
      format.true_atom();
      break;
    case tape_type::FALSE_VALUE:
      format.false_atom();
      break;
    case tape_type::NULL_VALUE:
      format.null_atom();
      break;

    // These are impossible
    case tape_type::END_ARRAY:
    case tape_type::END_OBJECT:
    case tape_type::ROOT:
      SIMDJSON_UNREACHABLE();
    }
    iter.json_index++;
    after_value = true;

    // Handle multiple ends in a row
    while (depth != 0 && (iter.tape_ref_type() == tape_type::END_ARRAY ||
                          iter.tape_ref_type() == tape_type::END_OBJECT)) {
      if (iter.tape_ref_type() == tape_type::END_ARRAY) {
        format.end_array();
      } else {
        format.end_object();
      }
      depth--;
      iter.json_index++;
    }

    // Stop when we're at depth 0
  } while (depth != 0);
}

template <class serializer>
inline void string_builder<serializer>::append(dom::object value) {
  format.start_object();
  auto pair = value.begin();
  auto end = value.end();
  if (pair != end) {
    append(*pair);
    for (++pair; pair != end; ++pair) {
      format.comma();
      append(*pair);
    }
  }
  format.end_object();
}

template <class serializer>
inline void string_builder<serializer>::append(dom::array value) {
  format.start_array();
  auto iter = value.begin();
  auto end = value.end();
  if (iter != end) {
    append(*iter);
    for (++iter; iter != end; ++iter) {
      format.comma();
      append(*iter);
    }
  }
  format.end_array();
}

template <class serializer>
simdjson_really_inline void string_builder<serializer>::append(dom::key_value_pair kv) {
  format.key(kv.key);
  append(kv.value);
}

template <class serializer>
simdjson_really_inline void string_builder<serializer>::clear() {
  format.clear();
}

template <class serializer>
simdjson_really_inline std::string_view string_builder<serializer>::str() const {
  return format.str();
}


} // namespace dom
} // namespace simdjson

#endif