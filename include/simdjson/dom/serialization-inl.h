
#ifndef SIMDJSON_SERIALIZATION_INL_H
#define SIMDJSON_SERIALIZATION_INL_H

#include "simdjson/portability.h"
#include "simdjson/common_defs.h" 
#include <cinttypes>
#include <type_traits>

namespace simdjson {
namespace dom {

namespace {
// defined in src/to_chars
char* to_chars(char* first, const char* last, double value);

template <typename T>
char *fast_itoa(char *output, T value) {
    // This is a standard implementation of itoa.
    // We first write in reverse order and then reverse.
    int64_t original_value;
    char * write_pointer = output;
    do {
        original_value = value;
        value /= 10;
        *write_pointer++ = '0' + (original_value - value * 10);
    } while (value > 0);
    if(std::is_signed<T>::value) {
      if (original_value < 0) { *write_pointer++ = '-'; }
    }
    // then we reverse the result
    char * const answer = write_pointer;
    char * second_write_pointer = output;
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
}// anonymous namespace


inline void mini_formatter::number(uint64_t x) {
    size_t current_size = buffer.size();
    char * p = buffer.data() + current_size;
    size_t buffer_size = 64;
    buffer.resize(current_size + buffer_size); // not good
    char * newp = fast_itoa(p, x);
    buffer.resize(newp - p);
}

inline void mini_formatter::number(int64_t x) {
    size_t current_size = buffer.size();
    char * p = buffer.data() + current_size;
    size_t buffer_size = 64;
    buffer.resize(current_size + buffer_size); // not good
    char * newp = fast_itoa(p, x);
    buffer.resize(newp - p);
}

inline void mini_formatter::number(double x) {
    size_t current_size = buffer.size();
    char * p = buffer.data() + current_size;
    size_t buffer_size = 64;
    buffer.resize(current_size + buffer_size); // not good
    char * newp = to_chars(p, nullptr, x);
    buffer.resize(newp - p);
}


inline void mini_formatter::start_array() { one_char('['); }
inline void mini_formatter::end_array() { one_char(']'); }
inline void mini_formatter::start_object() { one_char('{'); }
inline void mini_formatter::end_object() { one_char('}'); }
inline void mini_formatter::comma() { one_char(','); }
inline void mini_formatter::key(std::string_view v) { one_char(','); }
inline void mini_formatter::c_str(const char *c) {
    for (; *c != '\0'; c++) { one_char(*c); }
}

inline void mini_formatter::true_atom() { c_str("true"); }
inline void mini_formatter::false_atom() { c_str("false"); }
inline void mini_formatter::null_atom() { c_str("null"); }
inline void mini_formatter::one_char(char c) { buffer.push_back(c); }
inline void mini_formatter::key(std::string_view unescaped) {
      string(unescaped);
      one_char(':');
}
inline void mini_formatter::string(std::string_view unescaped) {
    one_char('\"');
    size_t i = 0;
    // fast path for the case where we have no control character
    for (; (i < unescaped.length()) && (uint8_t(unescaped[i]) > 0x1F); i++) {
      buffer.push_back(unescaped[i]);
    }
    // We caught a control character if we enter this loop (slow)
    for (; i < unescaped.length(); i++) {
      switch (unescaped[i]) {
      case '\"':
        c_str("\\\"");
        break;
      case '\\':
        c_str("\\\\");
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
    } //for
    one_char('\"');
}


template <class serializer>
inline void string_buffer<serializer>::append(dom::element value) {
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
inline string_buffer<serializer>::append(dom::object value) {
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
inline string_buffer<serializer>::append(dom::array value) {
  format.start_array();
  auto iter = value.begin();
  auto end = value.end();
  if (iter != end) {
    format.append(*iter);
    for (++iter; iter != end; ++iter) {
      format.comma();
      format.append(*iter);
    }
  }
  format.end_array();
}


template <class serializer>
inline string_buffer<serializer>::append(dom::key_value_pair kv) {
  format.key(kv.key);
  format.append(kv.value);
}

} // dom
} // namespace simdjson 

#endif