
#ifndef SIMDJSON_SERIALIZATION_INL_H
#define SIMDJSON_SERIALIZATION_INL_H

#include "simdjson/dom/base.h"
#include "simdjson/dom/parser.h"
#include "simdjson/dom/serialization.h"
#include "simdjson/internal/jeaiii.h"
#include "simdjson/internal/json_escape.h"
#include "simdjson/internal/tape_type.h"

#include "simdjson/dom/array-inl.h"
#include "simdjson/dom/object-inl.h"
#include "simdjson/internal/tape_ref-inl.h"

#include <cmath>
#include <cstring>

namespace simdjson {
namespace dom {
inline bool parser::print_json(std::ostream &os) const noexcept {
  if (!valid) {
    return false;
  }
  simdjson::internal::string_builder<> sb;
  sb.append(doc.root());
  std::string_view answer = sb.str();
  os << answer;
  return true;
}

inline std::ostream &operator<<(std::ostream &out,
                                simdjson::dom::element value) {
  simdjson::internal::string_builder<> sb;
  sb.append(value);
  return (out << sb.str());
}
#if SIMDJSON_EXCEPTIONS
inline std::ostream &
operator<<(std::ostream &out,
           simdjson::simdjson_result<simdjson::dom::element> x) {
  if (x.error()) {
    throw simdjson::simdjson_error(x.error());
  }
  return (out << x.value());
}
#endif
inline std::ostream &operator<<(std::ostream &out, simdjson::dom::array value) {
  simdjson::internal::string_builder<> sb;
  sb.append(value);
  return (out << sb.str());
}
#if SIMDJSON_EXCEPTIONS
inline std::ostream &
operator<<(std::ostream &out,
           simdjson::simdjson_result<simdjson::dom::array> x) {
  if (x.error()) {
    throw simdjson::simdjson_error(x.error());
  }
  return (out << x.value());
}
#endif
inline std::ostream &operator<<(std::ostream &out,
                                simdjson::dom::object value) {
  simdjson::internal::string_builder<> sb;
  sb.append(value);
  return (out << sb.str());
}
#if SIMDJSON_EXCEPTIONS
inline std::ostream &
operator<<(std::ostream &out,
           simdjson::simdjson_result<simdjson::dom::object> x) {
  if (x.error()) {
    throw simdjson::simdjson_error(x.error());
  }
  return (out << x.value());
}
#endif

} // namespace dom

namespace internal {

/***
 * Minifier/formatter code.
 **/

template <class formatter>
simdjson_inline void base_formatter<formatter>::number(uint64_t x) {
  // write_uint_jeaiii may store one byte before its output pointer.
  char number_buffer[24];
  chars(number_buffer + 1, write_uint_jeaiii(number_buffer + 1, x));
}

template <class formatter>
simdjson_inline void base_formatter<formatter>::number(int64_t x) {
  char number_buffer[24];
  // 0 - uint64_t(x) is the magnitude, also for INT64_MIN.
  char *end = write_uint_jeaiii(number_buffer + 1, x < 0 ? 0 - uint64_t(x) : uint64_t(x));
  number_buffer[0] = '-';
  chars(number_buffer + (x >= 0), end);
}

template <class formatter>
simdjson_inline void base_formatter<formatter>::number(double x) {
#if SIMDJSON_ENABLE_NAN_INF
  if (simdjson_unlikely(!std::isfinite(x))) {
    if (std::isnan(x)) {
      char const *s = "NaN";
      chars(s, s + 3);
    } else {
      if (x < 0) {
        one_char('-');
      }
      char const *s = "Infinity";
      chars(s, s + 8);
    }
    return;
  }
#endif

  // Must be to_chars_buffer_size (40): only ~24 chars are emitted, but
  // to_chars over-writes with fixed-size 16/17-byte copies for inlining.
  char number_buffer[simdjson::internal::to_chars_buffer_size];
  // Currently, passing the nullptr to the second argument is
  // safe because our implementation does not check the second
  // argument.
  char *newp = simdjson::internal::to_chars(number_buffer, nullptr, x);
  chars(number_buffer, newp);
}

template <class formatter>
simdjson_inline void base_formatter<formatter>::start_array() {
  one_char('[');
}

template <class formatter>
simdjson_inline void base_formatter<formatter>::end_array() {
  one_char(']');
}

template <class formatter>
simdjson_inline void base_formatter<formatter>::start_object() {
  one_char('{');
}

template <class formatter>
simdjson_inline void base_formatter<formatter>::end_object() {
  one_char('}');
}

template <class formatter>
simdjson_inline void base_formatter<formatter>::comma() {
  one_char(',');
}

template <class formatter>
simdjson_inline void base_formatter<formatter>::true_atom() {
  const char *s = "true";
  chars(s, s + 4);
}

template <class formatter>
simdjson_inline void base_formatter<formatter>::false_atom() {
  const char *s = "false";
  chars(s, s + 5);
}

template <class formatter>
simdjson_inline void base_formatter<formatter>::null_atom() {
  const char *s = "null";
  chars(s, s + 4);
}

template <class formatter>
simdjson_inline void base_formatter<formatter>::one_char(char c) {
  buffer.push_back(c);
}

template <class formatter>
simdjson_inline void base_formatter<formatter>::chars(const char *begin,
                                                      const char *end) {
  buffer.append(begin, end);
}

template <class formatter>
simdjson_inline void
base_formatter<formatter>::key(std::string_view unescaped) {
  string(unescaped);
  one_char(':');
}

template <class formatter>
simdjson_inline void
base_formatter<formatter>::string(std::string_view unescaped) {
  one_char('\"');
  // Each input byte becomes at most 6 output bytes. Escaping the string in
  // pieces bounds the room reserved ahead of a long string.
  constexpr size_t piece = 4096;
  for (size_t i = 0; i < unescaped.size(); i += piece) {
    const std::string_view part = unescaped.substr(i, piece);
    char *out = buffer.reserve(6 * part.size());
    buffer.set_end(out + escape_best::write_string_escaped(part, out));
  }
  one_char('\"');
}

template <class formatter> inline void base_formatter<formatter>::clear() {
  buffer.clear();
}

template <class formatter>
simdjson_inline std::string_view base_formatter<formatter>::str() const {
  return buffer.str();
}

simdjson_inline void mini_formatter::print_newline() { return; }

simdjson_inline void mini_formatter::print_indents(size_t depth) {
  (void)depth;
  return;
}

simdjson_inline void mini_formatter::print_space() { return; }

simdjson_inline void pretty_formatter::print_newline() { one_char('\n'); }

simdjson_inline void pretty_formatter::print_indents(size_t depth) {
  if (indent_step <= 0) {
    return;
  }
  buffer.append_n(' ', depth * static_cast<size_t>(indent_step));
}

simdjson_inline void pretty_formatter::print_space() { one_char(' '); }

/***
 * String building code.
 **/

template <class serializer>
inline void string_builder<serializer>::append(simdjson::dom::element value) {
  append(value, 0);
  format.print_newline();
}

template <class serializer>
inline void string_builder<serializer>::append(simdjson::dom::element value, size_t base_depth) {
  // using tape_type = simdjson::internal::tape_type;
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
      format.print_newline();
    }

    // At depth 0 the caller has already indented the value.
    if (depth != 0) {
      format.print_indents(base_depth + depth);
    }

    // If we are in an object, print the next key and :, and skip to the next
    // value.
    if (is_object[depth]) {
      format.key(iter.get_string_view());
      format.print_space();
      iter.json_index++;
    }
    switch (iter.tape_ref_type()) {

    // Arrays
    case tape_type::START_ARRAY: {
      // If we're too deep, we need to recurse to go deeper.
      depth++;
      if (simdjson_unlikely(depth >= MAX_DEPTH)) {
        append(simdjson::dom::element(iter), base_depth + depth - 1);
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
      format.print_newline();
      continue;
    }

    // Objects
    case tape_type::START_OBJECT: {
      // If we're too deep, we need to recurse to go deeper.
      depth++;
      if (simdjson_unlikely(depth >= MAX_DEPTH)) {
        append(simdjson::dom::element(iter), base_depth + depth - 1);
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
      format.print_newline();
      continue;
    }

    // Scalars
    case tape_type::STRING:
      format.string(iter.get_string_view());
      break;
    case tape_type::BIGINT: {
      // Big integer stored as string -- output raw digits (no quotes)
      auto sv = iter.get_string_view();
      format.chars(sv.data(), sv.data() + sv.size());
      break;
    }
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
      format.print_newline();
      depth--;
      format.print_indents(base_depth + depth);
      if (iter.tape_ref_type() == tape_type::END_ARRAY) {
        format.end_array();
      } else {
        format.end_object();
      }
      iter.json_index++;
    }

    // Stop when we're at depth 0
  } while (depth != 0);
}

template <class serializer>
inline void string_builder<serializer>::append(simdjson::dom::object value) {
  append(simdjson::dom::element(value));
}

template <class serializer>
inline void string_builder<serializer>::append(simdjson::dom::array value) {
  append(simdjson::dom::element(value));
}

template <class serializer>
simdjson_inline void
string_builder<serializer>::append(simdjson::dom::key_value_pair kv) {
  format.key(kv.key);
  append(kv.value);
}

template <class serializer>
simdjson_inline void string_builder<serializer>::clear() {
  format.clear();
}

template <class serializer>
simdjson_inline std::string_view string_builder<serializer>::str() const {
  return format.str();
}

} // namespace internal
} // namespace simdjson

#endif
