#ifndef SIMDJSON_SRC_GENERIC_STAGE2_TAPE_BUILDER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE2_TAPE_BUILDER_H
#include <generic/stage2/base.h>
#include <generic/stage2/json_iterator.h>
#include <generic/stage2/stringparsing.h>
#include <generic/stage2/tape_writer.h>
#include <simdjson/dom/document.h>
#include <simdjson/generic/atomparsing.h>
#include <simdjson/generic/dom_parser_implementation.h>
#include <simdjson/generic/numberparsing.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE


namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage2 {

template <bool UNPADDED>
struct tape_builder_impl {
  /** Called when a non-empty document starts. */
  simdjson_warn_unused simdjson_inline error_code visit_document_start(json_iterator &iter) noexcept;
  /** Called when a non-empty document ends without error. */
  simdjson_warn_unused simdjson_inline error_code visit_document_end(json_iterator &iter) noexcept;

  /** Called when a non-empty array starts. */
  simdjson_warn_unused simdjson_inline error_code visit_array_start(json_iterator &iter) noexcept;
  /** Called when a non-empty array ends. */
  simdjson_warn_unused simdjson_inline error_code visit_array_end(json_iterator &iter) noexcept;
  /** Called when an empty array is found. */
  simdjson_warn_unused simdjson_inline error_code visit_empty_array(json_iterator &iter) noexcept;

  /** Called when a non-empty object starts. */
  simdjson_warn_unused simdjson_inline error_code visit_object_start(json_iterator &iter) noexcept;
  /**
   * Called when a key in a field is encountered.
   *
   * primitive, visit_object_start, visit_empty_object, visit_array_start, or visit_empty_array
   * will be called after this with the field value.
   */
  simdjson_warn_unused simdjson_inline error_code visit_key(json_iterator &iter, const uint8_t *key) noexcept;
  /** Called when a non-empty object ends. */
  simdjson_warn_unused simdjson_inline error_code visit_object_end(json_iterator &iter) noexcept;
  /** Called when an empty object is found. */
  simdjson_warn_unused simdjson_inline error_code visit_empty_object(json_iterator &iter) noexcept;

  /**
   * Called when a string, number, boolean or null is found.
   */
  simdjson_warn_unused simdjson_inline error_code visit_primitive(json_iterator &iter, const uint8_t *value) noexcept;
  /**
   * Called when a string, number, boolean or null is found at the top level of a document (i.e.
   * when there is no array or object and the entire document is a single string, number, boolean or
   * null.
   *
   * This is separate from primitive() because simdjson's normal primitive parsing routines assume
   * there is at least one more token after the value, which is only true in an array or object.
   */
  simdjson_warn_unused simdjson_inline error_code visit_root_primitive(json_iterator &iter, const uint8_t *value) noexcept;

  simdjson_warn_unused simdjson_inline error_code visit_string(json_iterator &iter, const uint8_t *value, bool key = false) noexcept;
  simdjson_warn_unused simdjson_inline error_code visit_number(json_iterator &iter, const uint8_t *value) noexcept;
  simdjson_warn_unused simdjson_inline error_code visit_true_atom(json_iterator &iter, const uint8_t *value) noexcept;
  simdjson_warn_unused simdjson_inline error_code visit_false_atom(json_iterator &iter, const uint8_t *value) noexcept;
  simdjson_warn_unused simdjson_inline error_code visit_null_atom(json_iterator &iter, const uint8_t *value) noexcept;

  simdjson_warn_unused simdjson_inline error_code visit_root_string(json_iterator &iter, const uint8_t *value) noexcept;
  simdjson_warn_unused simdjson_inline error_code visit_root_number(json_iterator &iter, const uint8_t *value) noexcept;
  simdjson_warn_unused simdjson_inline error_code visit_root_true_atom(json_iterator &iter, const uint8_t *value) noexcept;
  simdjson_warn_unused simdjson_inline error_code visit_root_false_atom(json_iterator &iter, const uint8_t *value) noexcept;
  simdjson_warn_unused simdjson_inline error_code visit_root_null_atom(json_iterator &iter, const uint8_t *value) noexcept;

#if SIMDJSON_ENABLE_NAN_INF
  simdjson_warn_unused simdjson_inline error_code visit_nan_atom(json_iterator &iter, const uint8_t *value, error_code errc) noexcept;
  simdjson_warn_unused simdjson_inline error_code visit_root_nan_atom(json_iterator &iter, const uint8_t *value, error_code errc) noexcept;
  // Attempts to parse 'inf' or 'infinity' (case insensitive). Because neither are canonical atoms,
  // this returns a tape error on failure.
  simdjson_warn_unused simdjson_inline error_code visit_inf_atom(json_iterator &iter, const uint8_t *value) noexcept;
  simdjson_warn_unused simdjson_inline error_code visit_root_inf_atom(json_iterator &iter, const uint8_t *value) noexcept;
#endif

  /** Called each time a new field or element in an array or object is found. */
  simdjson_warn_unused simdjson_inline error_code increment_count(json_iterator &iter) noexcept;

  /** Next location to write to tape */
  tape_writer tape;
public:
  simdjson_inline tape_builder_impl(dom::document &doc) noexcept;
private:
  /** Next write location in the string buf for stage 2 parsing */
  uint8_t *current_string_buf_loc;

  simdjson_inline uint32_t next_tape_index(json_iterator &iter) const noexcept;
  simdjson_inline void start_container(json_iterator &iter) noexcept;
  simdjson_warn_unused simdjson_inline error_code end_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept;
  simdjson_warn_unused simdjson_inline error_code empty_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept;
  simdjson_inline uint8_t *on_start_string(json_iterator &iter) noexcept;
  simdjson_inline void on_end_string(uint8_t *dst) noexcept;
}; // struct tape_builder_impl

// Thin, non-templated entry so each architecture's stage2() keeps calling
// tape_builder::parse_document<STREAMING> unchanged. It chooses the bounds-safe
// (unpadded) or the regular (padded) tape_builder_impl ONCE per document, so the
// choice is a compile-time constant inside the walk: the padded path carries no
// extra branch or load (see tape_builder_impl::visit_string).
struct tape_builder {
  template<bool STREAMING>
  simdjson_warn_unused static simdjson_inline error_code parse_document(
      dom_parser_implementation &dom_parser, dom::document &doc) noexcept {
    dom_parser.doc = &doc;
    json_iterator iter(dom_parser, STREAMING ? dom_parser.next_structural_index : 0);
    if (dom_parser._unpadded) {
      tape_builder_impl<true> builder(doc);
      return iter.walk_document<STREAMING>(builder);
    } else {
      tape_builder_impl<false> builder(doc);
      return iter.walk_document<STREAMING>(builder);
    }
  }
};

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_root_primitive(json_iterator &iter, const uint8_t *value) noexcept {
  return iter.visit_root_primitive(*this, value);
}
template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_primitive(json_iterator &iter, const uint8_t *value) noexcept {
  return iter.visit_primitive(*this, value);
}
template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_empty_object(json_iterator &iter) noexcept {
  return empty_container(iter, internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
}
template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_empty_array(json_iterator &iter) noexcept {
  return empty_container(iter, internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_document_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}
template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_object_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}
template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_array_start(json_iterator &iter) noexcept {
  start_container(iter);
  return SUCCESS;
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_object_end(json_iterator &iter) noexcept {
  return end_container(iter, internal::tape_type::START_OBJECT, internal::tape_type::END_OBJECT);
}
template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_array_end(json_iterator &iter) noexcept {
  return end_container(iter, internal::tape_type::START_ARRAY, internal::tape_type::END_ARRAY);
}
template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_document_end(json_iterator &iter) noexcept {
  constexpr uint32_t start_tape_index = 0;
  tape.append(start_tape_index, internal::tape_type::ROOT);
  tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index(iter), internal::tape_type::ROOT);
  return SUCCESS;
}
template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_key(json_iterator &iter, const uint8_t *key) noexcept {
  return visit_string(iter, key, true);
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::increment_count(json_iterator &iter) noexcept {
  iter.dom_parser.open_containers[iter.depth].count++; // we have a key value pair in the object at parser.dom_parser.depth - 1
  return SUCCESS;
}

template <bool UNPADDED>
simdjson_inline tape_builder_impl<UNPADDED>::tape_builder_impl(dom::document &doc) noexcept : tape{doc.tape.get()}, current_string_buf_loc{doc.string_buf.get()} {}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_string(json_iterator &iter, const uint8_t *value, bool key) noexcept {
  iter.log_value(key ? "key" : "string");
  uint8_t *dst = on_start_string(iter);
  // We do not allow replacement when the escape characters are invalid.
  // UNPADDED is a compile-time constant chosen once per document by
  // tape_builder::parse_document, so the padded build instantiates only the
  // plain parse_string call below -- no runtime branch and no flag load.
  SIMDJSON_IF_CONSTEXPR (UNPADDED) {
    dst = stringparsing::parse_string_safe(value+1, dst, false, iter.buf + iter.dom_parser.len);
  } else {
    dst = stringparsing::parse_string(value+1, dst, false);
  }
  if (dst == nullptr) {
    iter.log_error("Invalid escape in string");
    return STRING_ERROR;
  }
  on_end_string(dst);
  return SUCCESS;
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_root_string(json_iterator &iter, const uint8_t *value) noexcept {
  return visit_string(iter, value);
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_number(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("number");
  const uint8_t *num = value;
  std::unique_ptr<uint8_t[]> copy{}; // keeps a padded copy of the tail alive when used
  SIMDJSON_IF_CONSTEXPR (UNPADDED) {
    // numberparsing reads ahead in 8-byte blocks for floats
    // (is_made_of_eight_digits_fast reads up to 7 bytes past the digits), so a
    // number whose digits reach the final bytes of an unpadded buffer would read
    // past it. *(next_structural) is the offset of the token following this
    // number, hence an upper bound on where the digits end; when that is within
    // SIMDJSON_PADDING of the end we parse from a space-padded copy of the tail
    // (mirroring visit_root_number). This fires only for numbers near the end.
    if (simdjson_unlikely(*(iter.next_structural) + SIMDJSON_PADDING > iter.dom_parser.len)) {
      const size_t rl = iter.remaining_len(); // bytes from `value` to the end of the document
      copy.reset(new (std::nothrow) uint8_t[rl + SIMDJSON_PADDING]);
      if (copy.get() == nullptr) { return MEMALLOC; }
      std::memcpy(copy.get(), value, rl);
      std::memset(copy.get() + rl, ' ', SIMDJSON_PADDING);
      num = copy.get();
    }
  }
  error_code err = numberparsing::parse_number(num, tape);
  if (simdjson_unlikely(err == BIGINT_ERROR &&
      iter.dom_parser._number_as_string)) {
    // Write big integer to string buffer using the same format as strings.
    // Scan digits the same way parse_number does (skip optional '-', then digits).
    const uint8_t *p = num;
    if (*p == '-') p++;
    while (numberparsing::is_digit(*p)) p++;
    size_t len = size_t(p - num);
    tape.append(current_string_buf_loc - iter.dom_parser.doc->string_buf.get(), internal::tape_type::BIGINT);
    uint8_t *dst = current_string_buf_loc + sizeof(uint32_t);
    memcpy(dst, num, len);
    dst += len;
    on_end_string(dst);
    return SUCCESS;
  }
  return err;
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_root_number(json_iterator &iter, const uint8_t *value) noexcept {
  //
  // We need to make a copy to make sure that the string is space terminated.
  // This is not about padding the input, which should already padded up
  // to len + SIMDJSON_PADDING. However, we have no control at this stage
  // on how the padding was done. What if the input string was padded with nulls?
  // It is quite common for an input string to have an extra null character (C string).
  // We do not want to allow 9\0 (where \0 is the null character) inside a JSON
  // document, but the string "9\0" by itself is fine. So we make a copy and
  // pad the input with spaces when we know that there is just one input element.
  // This copy is relatively expensive, but it will almost never be called in
  // practice unless you are in the strange scenario where you have many JSON
  // documents made of single atoms.
  //
  std::unique_ptr<uint8_t[]>copy(new (std::nothrow) uint8_t[iter.remaining_len() + SIMDJSON_PADDING]);
  if (copy.get() == nullptr) { return MEMALLOC; }
  std::memcpy(copy.get(), value, iter.remaining_len());
  std::memset(copy.get() + iter.remaining_len(), ' ', SIMDJSON_PADDING);
  error_code error = visit_number(iter, copy.get());
  return error;
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_true_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("true");
  // The non-length-aware validator reads a fixed 5 bytes; a malformed/truncated
  // token at the very end of an unpadded buffer would over-read. Use the
  // length-aware form there (the root variant already does this).
  const bool ok = UNPADDED ? atomparsing::is_valid_true_atom(value, iter.remaining_len())
                           : atomparsing::is_valid_true_atom(value);
  if (!ok) { return T_ATOM_ERROR; }
  tape.append(0, internal::tape_type::TRUE_VALUE);
  return SUCCESS;
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_root_true_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("true");
  if (!atomparsing::is_valid_true_atom(value, iter.remaining_len())) { return T_ATOM_ERROR; }
  tape.append(0, internal::tape_type::TRUE_VALUE);
  return SUCCESS;
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_false_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("false");
  const bool ok = UNPADDED ? atomparsing::is_valid_false_atom(value, iter.remaining_len())
                           : atomparsing::is_valid_false_atom(value);
  if (!ok) { return F_ATOM_ERROR; }
  tape.append(0, internal::tape_type::FALSE_VALUE);
  return SUCCESS;
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_root_false_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("false");
  if (!atomparsing::is_valid_false_atom(value, iter.remaining_len())) { return F_ATOM_ERROR; }
  tape.append(0, internal::tape_type::FALSE_VALUE);
  return SUCCESS;
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_null_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("null");
  const bool ok = UNPADDED ? atomparsing::is_valid_null_atom(value, iter.remaining_len())
                           : atomparsing::is_valid_null_atom(value);
  if (!ok) { return N_ATOM_ERROR; }
  tape.append(0, internal::tape_type::NULL_VALUE);
  return SUCCESS;
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_root_null_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("null");
  if (!atomparsing::is_valid_null_atom(value, iter.remaining_len())) { return N_ATOM_ERROR; }
  tape.append(0, internal::tape_type::NULL_VALUE);
  return SUCCESS;
}

#if SIMDJSON_ENABLE_NAN_INF
template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_nan_atom(json_iterator &iter, const uint8_t *value, error_code errc) noexcept {
  iter.log_value("nan");
  // For unpadded input use the length-aware validator so the 'infinity'-style
  // 8-byte compare cannot read past the buffer on a malformed token at the end.
  const bool ok = UNPADDED ? atomparsing::is_valid_nan_atom(value, iter.remaining_len())
                           : atomparsing::is_valid_nan_atom(value);
  if (!ok) { return errc; }
  tape.append_double(std::numeric_limits<double>::quiet_NaN());
  return SUCCESS;
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_root_nan_atom(json_iterator &iter, const uint8_t *value, error_code errc) noexcept {
  iter.log_value("nan");
  if (!atomparsing::is_valid_nan_atom(value, iter.remaining_len())) { return errc; }
  tape.append_double(std::numeric_limits<double>::quiet_NaN());
  return SUCCESS;
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_inf_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("inf");
  // Because 'inf' is an extension, non a canonical atom, a tape error should be returned on failure.
  // For unpadded input use the length-aware validator so the 'infinity' 8-byte
  // compare cannot read past the buffer on a malformed token at the end.
  const bool ok = UNPADDED ? atomparsing::is_valid_inf_atom(value, iter.remaining_len())
                           : atomparsing::is_valid_inf_atom(value);
  if (!ok) { return TAPE_ERROR; }
  tape.append_double(std::numeric_limits<double>::infinity());
  return SUCCESS;
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::visit_root_inf_atom(json_iterator &iter, const uint8_t *value) noexcept {
  iter.log_value("inf");
  // Because 'inf' is an extension, non a canonical atom, a tape error should be returned on failure
  if (!atomparsing::is_valid_inf_atom(value, iter.remaining_len())) { return TAPE_ERROR; }
  tape.append_double(std::numeric_limits<double>::infinity());
  return SUCCESS;
}
#endif // SIMDJSON_ENABLE_NAN_INF

// private:

template <bool UNPADDED>
simdjson_inline uint32_t tape_builder_impl<UNPADDED>::next_tape_index(json_iterator &iter) const noexcept {
  return uint32_t(tape.next_tape_loc - iter.dom_parser.doc->tape.get());
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::empty_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept {
  auto start_index = next_tape_index(iter);
  tape.append(start_index+2, start);
  tape.append(start_index, end);
  return SUCCESS;
}

template <bool UNPADDED>
simdjson_inline void tape_builder_impl<UNPADDED>::start_container(json_iterator &iter) noexcept {
  iter.dom_parser.open_containers[iter.depth].tape_index = next_tape_index(iter);
  iter.dom_parser.open_containers[iter.depth].count = 0;
  tape.skip(); // We don't actually *write* the start element until the end.
}

template <bool UNPADDED>
simdjson_warn_unused simdjson_inline error_code tape_builder_impl<UNPADDED>::end_container(json_iterator &iter, internal::tape_type start, internal::tape_type end) noexcept {
  // Write the ending tape element, pointing at the start location
  const uint32_t start_tape_index = iter.dom_parser.open_containers[iter.depth].tape_index;
  tape.append(start_tape_index, end);
  // Write the start tape element, pointing at the end location (and including count)
  // count can overflow if it exceeds 24 bits... so we saturate
  // the convention being that a cnt of 0xffffff or more is undetermined in value (>=  0xffffff).
  const uint32_t count = iter.dom_parser.open_containers[iter.depth].count;
  const uint32_t cntsat = count > 0xFFFFFF ? 0xFFFFFF : count;
  tape_writer::write(iter.dom_parser.doc->tape[start_tape_index], next_tape_index(iter) | (uint64_t(cntsat) << 32), start);
  return SUCCESS;
}

template <bool UNPADDED>
simdjson_inline uint8_t *tape_builder_impl<UNPADDED>::on_start_string(json_iterator &iter) noexcept {
  // we advance the point, accounting for the fact that we have a NULL termination
  tape.append(current_string_buf_loc - iter.dom_parser.doc->string_buf.get(), internal::tape_type::STRING);
  return current_string_buf_loc + sizeof(uint32_t);
}

template <bool UNPADDED>
simdjson_inline void tape_builder_impl<UNPADDED>::on_end_string(uint8_t *dst) noexcept {
  uint32_t str_length = uint32_t(dst - (current_string_buf_loc + sizeof(uint32_t)));
  // TODO check for overflow in case someone has a crazy string (>=4GB?)
  // But only add the overflow check when the document itself exceeds 4GB
  // Currently unneeded because we refuse to parse docs larger or equal to 4GB.
  memcpy(current_string_buf_loc, &str_length, sizeof(uint32_t));
  // NULL termination is still handy if you expect all your strings to
  // be NULL terminated? It comes at a small cost
  *dst = 0;
  current_string_buf_loc = dst + 1;
}

} // namespace stage2
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE2_TAPE_BUILDER_H
