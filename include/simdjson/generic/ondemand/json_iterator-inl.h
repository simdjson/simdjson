#ifndef SIMDJSON_GENERIC_ONDEMAND_JSON_ITERATOR_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_ONDEMAND_JSON_ITERATOR_INL_H
#include "simdjson/internal/dom_parser_implementation.h"
#include "simdjson/generic/ondemand/base.h"
#include "simdjson/generic/ondemand/json_iterator.h"
#include "simdjson/generic/ondemand/parser.h"
#include "simdjson/generic/ondemand/raw_json_string.h"
#include "simdjson/generic/ondemand/logger-inl.h"
#include "simdjson/generic/ondemand/parser-inl.h"
#include "simdjson/generic/ondemand/token_iterator-inl.h"
#include "simdjson/generic/atomparsing.h"
#include "simdjson/generic/numberparsing.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

simdjson_inline json_iterator::json_iterator(json_iterator &&other) noexcept
  : token(std::forward<token_iterator>(other.token)),
    parser{other.parser},
    _string_buf_loc{other._string_buf_loc},
    error{other.error},
    _depth{other._depth},
    _root{other._root},
    _streaming{other._streaming}
#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
    , _allow_incomplete_json{other._allow_incomplete_json}
#endif
{
  other.parser = nullptr;
}
simdjson_inline json_iterator &json_iterator::operator=(json_iterator &&other) noexcept {
  token = other.token;
  parser = other.parser;
  _string_buf_loc = other._string_buf_loc;
  error = other.error;
  _depth = other._depth;
  _root = other._root;
  _streaming = other._streaming;
#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
  _allow_incomplete_json = other._allow_incomplete_json;
#endif
  other.parser = nullptr;
  return *this;
}

simdjson_inline json_iterator::json_iterator(const uint8_t *buf, ondemand::parser *_parser) noexcept
  : token(buf, &_parser->implementation->structural_indexes[0]),
    parser{_parser},
    _string_buf_loc{parser->string_buf.get()},
    _depth{1},
    _root{parser->implementation->structural_indexes.get()},
    _streaming{false}

{
  logger::log_headers();
#if SIMDJSON_CHECK_EOF
  assert_more_tokens();
#endif
}

#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
simdjson_inline json_iterator::json_iterator(const uint8_t *buf, ondemand::parser *_parser, bool streaming) noexcept
    : token(buf, &_parser->implementation->structural_indexes[0]),
      parser{_parser},
      _string_buf_loc{parser->string_buf.get()},
      _depth{1},
      _root{parser->implementation->structural_indexes.get()},
      _streaming{streaming},
      _allow_incomplete_json{true}

{
  logger::log_headers();
#if SIMDJSON_CHECK_EOF
  assert_more_tokens();
#endif
}
#endif // SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON

simdjson_inline error_code json_iterator::rewind() noexcept {
  // abandon() let go of the parser: the iterator cannot be walked again.
  if (!is_alive()) { return error ? error : UNINITIALIZED; }
  token.set_position( root_position() );
  logger::log_headers(); // We start again
  _string_buf_loc = parser->string_buf.get();
  _depth = 1;
  return SUCCESS;
}

inline bool json_iterator::balanced() const noexcept {
  token_iterator ti(token);
  int32_t count{0};
  ti.set_position( root_position() );
  while(ti.peek() <= peek_last()) {
    switch (*ti.return_current_and_advance())
    {
    case '[': case '{':
      count++;
      break;
    case ']': case '}':
      count--;
      break;
    default:
      break;
    }
  }
  return count == 0;
}


// Depth change of each structural character when skipping: +1 for [ and {, -1 for ] and }.
static constexpr int8_t skip_depth_delta[256] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,-1,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,-1,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

// Class of the first token of a value: 0 for a scalar, 1 for an array or an object, 2 for a
// token that cannot start a value.
static constexpr uint8_t value_start_class[256] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,2,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,2,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

simdjson_inline bool json_iterator::out_of_place(uint8_t current) const noexcept {
  // A value or a key follows a colon, a comma or an opening bracket; nothing else does.
  if (_depth <= 1) { return false; }
  const uint8_t before = *peek(-2) | 0x20;
  const bool after_start = before == ':' || before == ',' || before == '{';
  current |= 0x20;
  const bool ends = current == ':' || current == ',' || current == '}';
  return after_start == ends;
}

simdjson_inline error_code json_iterator::out_of_place_error() noexcept {
  return report_error(TAPE_ERROR, "Missing comma, colon or value");
}

simdjson_never_inline bool json_iterator::string_is_key(const uint8_t *json, token_position pos, token_position root, token_position end) noexcept {
  // The string before pos is followed by a colon. After a colon or a bracket, it is a value;
  // after a comma, it is a key if its container is an object.
  const uint8_t before = pos - 2 >= root ? json[pos[-2]] : ':';
  if (before == '{') { return true; }
  if (before == ':' || before == '[') { return false; }
  int32_t depth = 0;
  while (pos < end) {
    const uint8_t c = json[*pos++];
    depth += skip_depth_delta[c];
    if (depth < 0) { return c == '}'; }
  }
  return true;
}

// GCC 7 warns when the first line of this function is inlined away into oblivion due to the caller
// relating depth and parent_depth, which is a desired effect. The warning does not show up if the
// skip_child() function is not marked inline).
SIMDJSON_PUSH_DISABLE_WARNINGS
SIMDJSON_DISABLE_STRICT_OVERFLOW_WARNING
simdjson_warn_unused simdjson_inline error_code json_iterator::skip_rest(depth_t parent_depth) noexcept {
  // Now that we've considered the first value, we only increment/decrement for arrays/objects.
  //
  // Without logging, look up the depth change of each token in a table instead of branching
  // on it: the next byte is hard to predict, so a switch mispredicts often. The loop also keeps
  // the position and depth in locals, because the byte loads are char loads that may alias
  // token._position and _depth, which would force a store of both on every token.
  if (!logger::LOG_ENABLED) {
    token_position pos = token.position();
    const token_position end = end_position();
    const uint8_t *const json = token.buf;
    depth_t depth = _depth;
    while (pos < end) {
      depth += skip_depth_delta[json[*pos++]];
      if (depth <= parent_depth) {
        token.set_position(pos);
        _depth = depth;
        return SUCCESS;
      }
    }
    token.set_position(pos);
    _depth = depth;
    return report_error(INCOMPLETE_ARRAY_OR_OBJECT, "not enough close braces");
  }

  while (position() < end_position()) {
    switch (*return_current_and_advance()) {
      case '[': case '{':
        logger::log_start_value(*this, "skip");
        _depth++;
        break;
      // TODO consider whether matching braces is a requirement: if non-matching braces indicates
      // *missing* braces, then future lookups are not in the object/arrays they think they are,
      // violating the rule "validate enough structure that the user can be confident they are
      // looking at the right values."
      // PERF TODO we can eliminate the switch here with a lookup of how much to add to depth
      case ']': case '}':
        logger::log_end_value(*this, "skip");
        _depth--;
        if (depth() <= parent_depth) { return SUCCESS; }
        break;
      default:
        logger::log_value(*this, "skip", "");
        break;
    }
  }

  return report_error(INCOMPLETE_ARRAY_OR_OBJECT, "not enough close braces");
}

simdjson_warn_unused simdjson_inline error_code json_iterator::skip_child(depth_t parent_depth) noexcept {
  if (depth() <= parent_depth) { return SUCCESS; }
  const uint8_t current = *return_current_and_advance();
  if (simdjson_unlikely(out_of_place(current))) { return out_of_place_error(); }
  switch (current) {
    // TODO consider whether matching braces is a requirement: if non-matching braces indicates
    // *missing* braces, then future lookups are not in the object/arrays they think they are,
    // violating the rule "validate enough structure that the user can be confident they are
    // looking at the right values."
    // PERF TODO we can eliminate the switch here with a lookup of how much to add to depth

    // For the first open array/object in a value, we've already incremented depth, so keep it the same
    // We never stop at colon, but if we did, it wouldn't affect depth
    case '[': case '{': case ':':
      logger::log_start_value(*this, "skip");
      break;
    // If there is a comma, we have just finished a value in an array/object, and need to get back in
    case ',':
      logger::log_value(*this, "skip");
      break;
    // ] or } means we just finished a value and need to jump out of the array/object
    case ']': case '}':
      logger::log_end_value(*this, "skip");
      _depth--;
      if (depth() <= parent_depth) { return SUCCESS; }
#if SIMDJSON_CHECK_EOF
      // If there are no more tokens, the parent is incomplete.
      if (at_end()) { return report_error(INCOMPLETE_ARRAY_OR_OBJECT, "Missing [ or { at start"); }
#endif // SIMDJSON_CHECK_EOF
      break;
    case '"':
      // At the end, peek() would read the sentinel, which points into the padding.
      if(!at_end() && *peek() == ':' && string_is_key(token.buf, token.position(), _root, end_position())) {
        // We are at a key!!!
        // This might happen if you just started an object and you skip it immediately.
        // Performance note: it would be nice to get rid of this check as it is somewhat
        // expensive.
        // https://github.com/simdjson/simdjson/issues/1742
        logger::log_value(*this, "key");
        return_current_and_advance(); // eat up the ':'
        break; // important!!!
      }
      if (_depth > 1 && *peek(-2) == '{') {
#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
        if (allow_incomplete_json() && at_end()) {
          return report_error(INCOMPLETE_ARRAY_OR_OBJECT, "Object key cut before its colon");
        }
#endif // SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
        return out_of_place_error();
      }
      simdjson_fallthrough;
    // Anything else must be a scalar value
    default:
      // For the first scalar, we will have incremented depth already, so we decrement it here.
      logger::log_value(*this, "skip");
      _depth--;
      if (depth() <= parent_depth) { return SUCCESS; }
      break;
  }

  return skip_rest(parent_depth);
}

simdjson_warn_unused simdjson_inline error_code json_iterator::skip_value(depth_t parent_depth) noexcept {
  const uint8_t value_class = value_start_class[*return_current_and_advance()];
  if (value_class == 0) {
    logger::log_value(*this, "skip");
    _depth--;
    return SUCCESS;
  }
  if (value_class == 2) { return out_of_place_error(); }
  logger::log_start_value(*this, "skip");
  return skip_rest(parent_depth);
}

simdjson_warn_unused simdjson_inline error_code json_iterator::skip_partial(depth_t parent_depth) noexcept {
  if (depth() <= parent_depth) { return SUCCESS; }
  if (depth() != parent_depth + 1) { return skip_child(parent_depth); }
  // We are in the child container, past its start: at a key, a colon, a comma or the closing
  // bracket. Another value can only follow a missing comma.
  switch (*return_current_and_advance()) {
    case ',': case ':':
      logger::log_value(*this, "skip");
      return skip_rest(parent_depth);
    case ']': case '}':
      logger::log_end_value(*this, "skip");
      _depth--;
      return SUCCESS;
    case '"':
      if (!at_end() && *peek() == ':') {
        logger::log_value(*this, "key");
        return_current_and_advance();
        return skip_rest(parent_depth);
      }
#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
      if (allow_incomplete_json() && at_end()) {
        return report_error(INCOMPLETE_ARRAY_OR_OBJECT, "Object key cut before its colon");
      }
#endif // SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
      simdjson_fallthrough;
    default:
#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
      if (simdjson_unlikely(allow_incomplete_json() && position() > end_position())) {
        return report_error(INCOMPLETE_ARRAY_OR_OBJECT, "Value cut at the end of an incomplete document");
      }
#endif // SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
      return out_of_place_error();
  }
}

SIMDJSON_POP_DISABLE_WARNINGS

// Tells whether a backslash occurs in the len bytes at src. The kernels with vector
// registers look at a whole register at a time and may read up to a register past the
// string, inside the padding.
template <typename BQ>
simdjson_inline bool json_iterator::has_backslash(const uint8_t *src, size_t len, std::true_type) noexcept {
  constexpr size_t STEP = BQ::BYTES_PROCESSED;
  uint8_t scratch[STEP];
  for (; len > STEP; src += STEP, len -= STEP) {
    if (BQ{}.copy_and_find(src, scratch).bs_bits) { return true; }
  }
  const uint64_t bs_bits = uint64_t(BQ{}.copy_and_find(src, scratch).bs_bits);
  return (len < 64 ? bs_bits & ((uint64_t(1) << len) - 1) : bs_bits) != 0;
}

// Without vector registers, the bytes are read sixteen at a time, up to fifteen bytes past
// the string. Most strings are short, so they take no loop iteration, whatever their length.
template <typename BQ>
simdjson_inline bool json_iterator::has_backslash(const uint8_t *src, size_t len, std::false_type) noexcept {
  constexpr uint64_t ONES = 0x0101010101010101;
  constexpr uint64_t HIGHS = 0x8080808080808080;
  constexpr uint64_t BACKSLASHES = 0x5C5C5C5C5C5C5C5C;
  // The masks are copied from this table, so that their first len bytes are set whatever
  // the endianness.
  static constexpr uint8_t KEEP[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint64_t lo, hi;
  for (; len > 16; src += 16, len -= 16) {
    std::memcpy(&lo, src, sizeof(lo));
    std::memcpy(&hi, src + 8, sizeof(hi));
    lo ^= BACKSLASHES;
    hi ^= BACKSLASHES;
    if ((((lo - ONES) & ~lo) | ((hi - ONES) & ~hi)) & HIGHS) { return true; }
  }
  uint64_t keep_lo, keep_hi;
  std::memcpy(&keep_lo, KEEP + 16 - len, sizeof(keep_lo));
  std::memcpy(&keep_hi, KEEP + 24 - len, sizeof(keep_hi));
  std::memcpy(&lo, src, sizeof(lo));
  std::memcpy(&hi, src + 8, sizeof(hi));
  lo = (lo ^ BACKSLASHES) | ~keep_lo;
  hi = (hi ^ BACKSLASHES) | ~keep_hi;
  return ((((lo - ONES) & ~lo) | ((hi - ONES) & ~hi)) & HIGHS) != 0;
}

simdjson_inline bool json_iterator::has_backslash(const uint8_t *src, size_t len) noexcept {
  constexpr size_t STEP = backslash_and_quote::BYTES_PROCESSED;
  return has_backslash<backslash_and_quote>(src, len, std::integral_constant<bool, STEP >= 16 && STEP <= 64>());
}

// Counts the decimal digits at p. It may read up to seven bytes past them.
simdjson_inline size_t json_iterator::count_digits(const uint8_t *p) noexcept {
  size_t n = 0;
#if SIMDJSON_SWAR_NUMBER_PARSING
  for (;; n += 8) {
    uint64_t val;
    std::memcpy(&val, p + n, sizeof(val));
    const uint64_t non_digits = numberparsing::mark_non_digits(val);
    if (non_digits) { return n + size_t(trailing_zeroes(non_digits)) / 8; }
  }
#else
  while (numberparsing::is_digit(p[n])) { n++; }
  return n;
#endif
}

// Checks a number as get_double() does, without computing its value when the value is
// certainly finite: at most 300 integer digits without an exponent, or at most 200 with an
// exponent of at most two digits, stay below 1e300. Anything else goes to the full parser.
simdjson_inline error_code json_iterator::validate_number(const uint8_t *src) noexcept {
  const uint8_t *p = src + (*src == '-');
  const size_t integer_digits = count_digits(p);
  if (integer_digits == 0 || integer_digits > 300 || (*p == '0' && integer_digits > 1)) { goto full; }
  p += integer_digits;
  if (*p == '.') {
    p++;
    const size_t fraction_digits = count_digits(p);
    if (fraction_digits == 0) { return NUMBER_ERROR; }
    p += fraction_digits;
  }
  if ((*p | 0x20) == 'e') {
    p++;
    p += (*p == '-' || *p == '+');
    const size_t exponent_digits = count_digits(p);
    if (exponent_digits == 0) { return NUMBER_ERROR; }
    if (exponent_digits > 2 || integer_digits > 200) { goto full; }
    p += exponent_digits;
  }
  return jsoncharutils::is_not_structural_or_whitespace(*p) ? NUMBER_ERROR : SUCCESS;
full:
  return numberparsing::parse_double(src).error() ? NUMBER_ERROR : SUCCESS;
}

// Checks a value that is neither a container nor a string. When it is the last token, the
// document was cut short.
simdjson_inline error_code json_iterator::validate_scalar(const uint8_t *src, bool last) noexcept {
  switch (*src) {
    case 't': case 'f': case 'n': case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
#if SIMDJSON_ENABLE_NAN_INF
    case 'N': case 'i': case 'I':
#endif
      break;
    default:
      return TAPE_ERROR;
  }
  // The scalar is not the last token, so the parsers read inside the document.
  if (last) { return INCOMPLETE_ARRAY_OR_OBJECT; }
  switch (*src) {
    case 't': return atomparsing::is_valid_true_atom(src) ? SUCCESS : T_ATOM_ERROR;
    case 'f': return atomparsing::is_valid_false_atom(src) ? SUCCESS : F_ATOM_ERROR;
    case 'n':
#if SIMDJSON_ENABLE_NAN_INF
      return atomparsing::is_valid_null_atom(src) || atomparsing::is_valid_nan_atom(src) ? SUCCESS : N_ATOM_ERROR;
#else
      return atomparsing::is_valid_null_atom(src) ? SUCCESS : N_ATOM_ERROR;
#endif
#if SIMDJSON_ENABLE_NAN_INF
    case 'N': return atomparsing::is_valid_nan_atom(src) ? SUCCESS : TAPE_ERROR;
    case 'i': case 'I': return atomparsing::is_valid_inf_atom(src) ? SUCCESS : TAPE_ERROR;
#endif
    default:
      return validate_number(src);
  }
}

// The walk follows the DOM tape builder: values inside objects and inside arrays are
// dispatched from separate places, and the kind of the enclosing container is read only
// when a container closes.
simdjson_warn_unused simdjson_never_inline error_code json_iterator::validate_container() noexcept {
  const uint8_t *const json = token.buf;
  token_position pos = token.position();
  const token_position end = end_position();
  const size_t max_depth = parser->max_depth();
  // One bit per depth tells whether the container at that depth is an array. Deeper than
  // the fixed stack allows, the bits move to the heap.
  uint64_t fixed_stack[16]{};
  std::unique_ptr<uint64_t[]> heap_stack;
  uint64_t *is_array = fixed_stack;
  size_t depth = 0;
  uint32_t index;
  uint8_t c;

// Reads the next token into index and c; a document cut short has no more tokens.
#define SIMDJSON_VALIDATE_NEXT() \
  do { \
    if (pos == end) { return INCOMPLETE_ARRAY_OR_OBJECT; } \
    index = *pos++; \
    c = json[index]; \
  } while (0)

// A string is valid once stage 1 has accepted the document, unless it holds an escape. The
// next token bounds it.
#define SIMDJSON_VALIDATE_STRING() \
  do { \
    if (pos == end) { return INCOMPLETE_ARRAY_OR_OBJECT; } \
    if (has_backslash(json + index + 1, *pos - index - 1)) { \
      uint8_t *dst = _string_buf_loc; \
      if (!parser->implementation->parse_string(json + index + 1, dst, false)) { return STRING_ERROR; } \
    } \
  } while (0)

// As in DOM, an empty container does not count toward the maximal depth.
#define SIMDJSON_VALIDATE_VALUE() \
  do { \
    SIMDJSON_VALIDATE_NEXT(); \
    switch (c) { \
      case '{': \
        if (pos != end && json[*pos] == '}') { pos++; break; } \
        goto object_begin; \
      case '[': \
        if (pos != end && json[*pos] == ']') { pos++; break; } \
        goto array_begin; \
      case '"': \
        SIMDJSON_VALIDATE_STRING(); \
        break; \
      default: \
        SIMDJSON_TRY(validate_scalar(json + index, pos == end)); \
        break; \
    } \
  } while (0)

  index = *pos++;
  c = json[index];
  if (pos != end && json[*pos] == c + 2) {
    pos++;
    goto document_end;
  }
  if (c == '[') { goto array_begin; }

object_begin:
  if (++depth >= max_depth) { return DEPTH_ERROR; }
  // The bits move once: the next time this depth is reached, the heap holds the current
  // bits and the fixed stack holds stale ones.
  if (depth == sizeof(fixed_stack) * 8 && !heap_stack) {
    heap_stack.reset(new (std::nothrow) uint64_t[max_depth / 64 + 1]());
    if (!heap_stack) { return MEMALLOC; }
    std::memcpy(heap_stack.get(), fixed_stack, sizeof(fixed_stack));
    is_array = heap_stack.get();
  }
  is_array[depth / 64] &= ~(uint64_t(1) << (depth % 64));
  SIMDJSON_VALIDATE_NEXT();
  if (c != '"') { return TAPE_ERROR; }
  SIMDJSON_VALIDATE_STRING();

object_field:
  SIMDJSON_VALIDATE_NEXT();
  if (c != ':') { return TAPE_ERROR; }
  SIMDJSON_VALIDATE_VALUE();

object_continue:
  SIMDJSON_VALIDATE_NEXT();
  if (c == ',') {
    SIMDJSON_VALIDATE_NEXT();
    if (c != '"') { return TAPE_ERROR; }
    SIMDJSON_VALIDATE_STRING();
    goto object_field;
  }
  if (c != '}') { return TAPE_ERROR; }

scope_end:
  if (--depth == 0) { goto document_end; }
  if (is_array[depth / 64] & (uint64_t(1) << (depth % 64))) { goto array_continue; }
  goto object_continue;

array_begin:
  if (++depth >= max_depth) { return DEPTH_ERROR; }
  if (depth == sizeof(fixed_stack) * 8 && !heap_stack) {
    heap_stack.reset(new (std::nothrow) uint64_t[max_depth / 64 + 1]());
    if (!heap_stack) { return MEMALLOC; }
    std::memcpy(heap_stack.get(), fixed_stack, sizeof(fixed_stack));
    is_array = heap_stack.get();
  }
  is_array[depth / 64] |= uint64_t(1) << (depth % 64);

array_value:
  SIMDJSON_VALIDATE_VALUE();

array_continue:
  SIMDJSON_VALIDATE_NEXT();
  if (c == ',') { goto array_value; }
  if (c != ']') { return TAPE_ERROR; }
  goto scope_end;

document_end:
#undef SIMDJSON_VALIDATE_VALUE
#undef SIMDJSON_VALIDATE_STRING
#undef SIMDJSON_VALIDATE_NEXT
  if (!streaming() && pos != end) { return TRAILING_CONTENT; }
  return SUCCESS;
}

template <typename V>
simdjson_warn_unused simdjson_inline error_code json_iterator::visit_number(const number &value, const char *raw, V &visitor) {
  switch (value.get_number_type()) {
    case number_type::signed_integer: return visitor.on_int64(value.get_int64(), raw);
    case number_type::unsigned_integer: return visitor.on_uint64(value.get_uint64(), raw);
    case number_type::floating_point_number: return visitor.on_double(value.get_double(), raw);
    default: return BIGINT_ERROR;
  }
}

template <typename V>
simdjson_warn_unused simdjson_inline error_code json_iterator::visit_string(const uint8_t *quote, bool key, V &visitor) {
  const uint8_t *const start = quote + 1;
  // A string without escapes is passed as it is in the document.
  uint8_t scratch[backslash_and_quote::BYTES_PROCESSED];
  for (const uint8_t *src = start;; src += backslash_and_quote::BYTES_PROCESSED) {
    auto bs_quote = backslash_and_quote{}.copy_and_find(src, scratch);
    if (bs_quote.has_quote_first()) {
      std::string_view text(reinterpret_cast<const char *>(start), size_t(src - start) + size_t(bs_quote.quote_index()));
      return key ? visitor.on_key(text) : visitor.on_string(text);
    }
    if (bs_quote.has_backslash()) { break; }
  }
  uint8_t *const end = parser->implementation->parse_string(start, _string_buf_loc, false);
  if (!end) { return STRING_ERROR; }
  std::string_view text(reinterpret_cast<const char *>(_string_buf_loc), size_t(end - _string_buf_loc));
  _string_buf_loc = end;
  return key ? visitor.on_key(text) : visitor.on_string(text);
}

template <typename V>
simdjson_warn_unused simdjson_inline error_code json_iterator::visit_scalar(const uint8_t *src, V &visitor) {
  switch (*src) {
    case 't': return atomparsing::is_valid_true_atom(src) ? visitor.on_bool(true) : T_ATOM_ERROR;
    case 'f': return atomparsing::is_valid_false_atom(src) ? visitor.on_bool(false) : F_ATOM_ERROR;
    case 'n':
      if (atomparsing::is_valid_null_atom(src)) { return visitor.on_null(); }
#if SIMDJSON_ENABLE_NAN_INF
      if (atomparsing::is_valid_nan_atom(src)) { return visitor.on_double(std::numeric_limits<double>::quiet_NaN(), reinterpret_cast<const char *>(src)); }
#endif
      return N_ATOM_ERROR;
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      number value;
      SIMDJSON_TRY(numberparsing::parse_number(src, value));
      return visit_number(value, reinterpret_cast<const char *>(src), visitor);
    }
#if SIMDJSON_ENABLE_NAN_INF
    case 'N':
      if (atomparsing::is_valid_nan_atom(src)) { return visitor.on_double(std::numeric_limits<double>::quiet_NaN(), reinterpret_cast<const char *>(src)); }
      return TAPE_ERROR;
    case 'i': case 'I':
      if (atomparsing::is_valid_inf_atom(src)) { return visitor.on_double(std::numeric_limits<double>::infinity(), reinterpret_cast<const char *>(src)); }
      return TAPE_ERROR;
#endif
    default:
      return TAPE_ERROR;
  }
}

template <typename V>
simdjson_warn_unused error_code json_iterator::visit_container(V &visitor, token_position start, size_t parent_depth) {
  const uint8_t *const json = token.buf;
  token_position pos = start;
  const token_position end = end_position();
  const size_t max_depth = parser->max_depth();
  // For each open container, the number of its elements so far, with the top bit set for an
  // array. Deeper than the fixed stack allows, the stack moves to the heap.
  constexpr uint32_t ARRAY_BIT = uint32_t(1) << 31;
  constexpr size_t FIXED_DEPTH = 64;
  uint32_t fixed_stack[FIXED_DEPTH];
  std::unique_ptr<uint32_t[]> heap_stack;
  uint32_t *stack = fixed_stack;
  size_t depth = parent_depth;
  uint32_t index;
  uint8_t c;

// Reads the next token into index and c; a document cut short has no more tokens.
#define SIMDJSON_VISIT_NEXT() \
  do { \
    if (pos == end) { return INCOMPLETE_ARRAY_OR_OBJECT; } \
    index = *pos++; \
    c = json[index]; \
  } while (0)

  SIMDJSON_VISIT_NEXT();
  goto container_start;

value:
  switch (c) {
    case '{': case '[': goto container_start;
    case '"':
      // A string is not the last token, so that its closing quote is inside the document.
      if (pos == end) { return INCOMPLETE_ARRAY_OR_OBJECT; }
      SIMDJSON_TRY(visit_string(json + index, false, visitor));
      goto value_end;
    default:
      // The scalar is not the last token, so the parsers read inside the document.
      if (simdjson_unlikely(pos == end)) {
        switch (c) {
          case 't': case 'f': case 'n': case '-': case '0': case '1': case '2': case '3': case '4':
          case '5': case '6': case '7': case '8': case '9':
#if SIMDJSON_ENABLE_NAN_INF
          case 'N': case 'i': case 'I':
#endif
            return INCOMPLETE_ARRAY_OR_OBJECT;
          default:
            return TAPE_ERROR;
        }
      }
      SIMDJSON_TRY(visit_scalar(json + index, visitor));
      goto value_end;
  }

container_start:
  // As in DOM, an empty container does not count toward the maximal depth.
  if (pos != end && json[*pos] == c + 2) {
    pos++;
    if (c == '[') {
      SIMDJSON_TRY(visitor.on_array_start());
      SIMDJSON_TRY(visitor.on_array_end(0));
    } else {
      SIMDJSON_TRY(visitor.on_object_start());
      SIMDJSON_TRY(visitor.on_object_end(0));
    }
    if (depth == parent_depth) { goto document_end; }
    goto value_end;
  }
  if (++depth >= max_depth) { return DEPTH_ERROR; }
  // Copy the fixed stack once. A later container at this depth must keep the
  // counts already stored on the heap.
  if (depth >= FIXED_DEPTH && !heap_stack) {
    heap_stack.reset(new (std::nothrow) uint32_t[max_depth]);
    if (!heap_stack) { return MEMALLOC; }
    // Only the slots past the parent were written.
    if (parent_depth + 1 < FIXED_DEPTH) {
      std::memcpy(heap_stack.get() + parent_depth + 1, fixed_stack + parent_depth + 1, (FIXED_DEPTH - parent_depth - 1) * sizeof(fixed_stack[0]));
    }
    stack = heap_stack.get();
  }
  stack[depth] = 1;
  if (c == '[') {
    stack[depth] |= ARRAY_BIT;
    SIMDJSON_TRY(visitor.on_array_start());
    SIMDJSON_VISIT_NEXT();
    goto value;
  }
  SIMDJSON_TRY(visitor.on_object_start());
  SIMDJSON_VISIT_NEXT();

key:
  if (c != '"') { return TAPE_ERROR; }
  if (pos == end) { return INCOMPLETE_ARRAY_OR_OBJECT; }
  SIMDJSON_TRY(visit_string(json + index, true, visitor));
  SIMDJSON_VISIT_NEXT();
  if (c != ':') { return TAPE_ERROR; }
  SIMDJSON_VISIT_NEXT();
  goto value;

value_end:
  SIMDJSON_VISIT_NEXT();
  if (c == ',') {
    stack[depth]++;
    SIMDJSON_VISIT_NEXT();
    if (stack[depth] & ARRAY_BIT) { goto value; }
    goto key;
  }
  if (stack[depth] & ARRAY_BIT) {
    if (c != ']') { return TAPE_ERROR; }
    SIMDJSON_TRY(visitor.on_array_end(stack[depth] & ~ARRAY_BIT));
  } else {
    if (c != '}') { return TAPE_ERROR; }
    SIMDJSON_TRY(visitor.on_object_end(stack[depth]));
  }
  if (--depth > parent_depth) { goto value_end; }

document_end:
#undef SIMDJSON_VISIT_NEXT
  if (parent_depth == 0 && !streaming() && pos != end) { return TRAILING_CONTENT; }
  return SUCCESS;
}

simdjson_inline bool json_iterator::at_root() const noexcept {
  return position() == root_position();
}

simdjson_inline bool json_iterator::is_single_token() const noexcept {
  return parser->implementation->n_structural_indexes == 1;
}

simdjson_inline bool json_iterator::streaming() const noexcept {
  return _streaming;
}

#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
simdjson_inline bool json_iterator::allow_incomplete_json() const noexcept {
  return _allow_incomplete_json;
}

simdjson_inline size_t json_iterator::remaining_input_length(const uint8_t *json) const noexcept {
  const uint8_t *end = token.buf + parser->_document_len;
  return json < end ? size_t(end - json) : 0;
}

simdjson_inline const uint8_t *json_iterator::scalar_within_input(const uint8_t *json) noexcept {
  // Only the last token can run to the end of the input.
  const uint32_t n = parser->implementation->n_structural_indexes;
  if (simdjson_likely(json != token.buf + parser->implementation->structural_indexes[n - 1])) { return json; }
  uint8_t *tmpbuf = parser->incomplete_scalar_buf;
  constexpr size_t capacity = sizeof(parser->incomplete_scalar_buf) - SIMDJSON_PADDING;
  const size_t length = remaining_input_length(json);
  // A scalar too long for the buffer is left out entirely, so that it does not parse.
  const size_t copied = length <= capacity ? length : 0;
  std::memcpy(tmpbuf, json, copied);
  std::memset(tmpbuf + copied, ' ', sizeof(parser->incomplete_scalar_buf) - copied);
  return tmpbuf;
}
#endif // SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON

simdjson_inline token_position json_iterator::root_position() const noexcept {
  return _root;
}

simdjson_inline void json_iterator::assert_at_document_depth() const noexcept {
  SIMDJSON_ASSUME( _depth == 1 );
}

simdjson_inline void json_iterator::assert_at_root() const noexcept {
  SIMDJSON_ASSUME( _depth == 1 );
#ifndef SIMDJSON_CLANG_VISUAL_STUDIO
  // Under Visual Studio, the next SIMDJSON_ASSUME fails with: the argument
  // has side effects that will be discarded.
  SIMDJSON_ASSUME( token.position() == _root );
#endif
}

simdjson_inline void json_iterator::assert_more_tokens(uint32_t required_tokens) const noexcept {
#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
  if (allow_incomplete_json()) {
    SIMDJSON_ASSUME( token._position + required_tokens - 1 <= end_position() );
    return;
  }
#endif // SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
  assert_valid_position(token._position + required_tokens - 1);
}

simdjson_inline void json_iterator::assert_valid_position(token_position position) const noexcept {
  (void)position; // Suppress unused parameter warning
#ifndef SIMDJSON_CLANG_VISUAL_STUDIO
  SIMDJSON_ASSUME( position >= &parser->implementation->structural_indexes[0] );
  SIMDJSON_ASSUME( position < &parser->implementation->structural_indexes[parser->implementation->n_structural_indexes] );
#endif
}

simdjson_inline bool json_iterator::at_end() const noexcept {
  return position() == end_position();
}
simdjson_inline token_position json_iterator::end_position() const noexcept {
  uint32_t n_structural_indexes{parser->implementation->n_structural_indexes};
  return &parser->implementation->structural_indexes[n_structural_indexes];
}

inline std::string json_iterator::to_string() const noexcept {
  if( !is_alive() ) { return "dead json_iterator instance"; }
  const char * current_structural = reinterpret_cast<const char *>(token.peek());
  return std::string("json_iterator [ depth : ") + std::to_string(_depth)
          + std::string(", structural : '") + std::string(current_structural,1)
          + std::string("', offset : ") + std::to_string(token.current_offset())
          + std::string("', error : ") + error_message(error)
          + std::string(" ]");
}

inline simdjson_result<const char *> json_iterator::current_location() const noexcept {
  if (!is_alive()) {    // Unrecoverable error
    if (!at_root()) {
      return reinterpret_cast<const char *>(token.peek(-1));
    } else {
      return reinterpret_cast<const char *>(token.peek());
    }
  }
  if (at_end()) {
    return OUT_OF_BOUNDS;
  }
  return reinterpret_cast<const char *>(token.peek());
}

simdjson_inline bool json_iterator::is_alive() const noexcept {
  return parser;
}

simdjson_inline void json_iterator::abandon() noexcept {
  parser = nullptr;
  _depth = 0;
}

simdjson_inline const uint8_t *json_iterator::return_current_and_advance() noexcept {
#if SIMDJSON_CHECK_EOF
  assert_more_tokens();
#endif // SIMDJSON_CHECK_EOF
  return token.return_current_and_advance();
}

simdjson_inline const uint8_t *json_iterator::unsafe_pointer() const noexcept {
  // deliberately done without safety guard:
  return token.peek();
}

simdjson_inline const uint8_t *json_iterator::peek(int32_t delta) const noexcept {
#if SIMDJSON_CHECK_EOF
  assert_more_tokens(delta+1);
#endif // SIMDJSON_CHECK_EOF
  return token.peek(delta);
}

simdjson_inline uint32_t json_iterator::peek_length(int32_t delta) const noexcept {
#if SIMDJSON_CHECK_EOF
  assert_more_tokens(delta+1);
#endif // #if SIMDJSON_CHECK_EOF
  return token.peek_length(delta);
}

simdjson_inline const uint8_t *json_iterator::peek(token_position position) const noexcept {
  // todo: currently we require end-of-string buffering, but the following
  // assert_valid_position should be turned on if/when we lift that condition.
  // assert_valid_position(position);
  // This is almost surely related to SIMDJSON_CHECK_EOF but given that SIMDJSON_CHECK_EOF
  // is ON by default, we have no choice but to disable it for real with a comment.
  return token.peek(position);
}

simdjson_inline uint32_t json_iterator::peek_length(token_position position) const noexcept {
#if SIMDJSON_CHECK_EOF
  assert_valid_position(position);
#endif // SIMDJSON_CHECK_EOF
  return token.peek_length(position);
}
simdjson_inline uint32_t json_iterator::peek_root_length(token_position position) const noexcept {
#if SIMDJSON_CHECK_EOF
  assert_valid_position(position);
#endif // SIMDJSON_CHECK_EOF
  return token.peek_root_length(position);
}

simdjson_inline token_position json_iterator::last_position() const noexcept {
  // The following line fails under some compilers...
  // SIMDJSON_ASSUME(parser->implementation->n_structural_indexes > 0);
  // since it has side-effects.
  uint32_t n_structural_indexes{parser->implementation->n_structural_indexes};
  SIMDJSON_ASSUME(n_structural_indexes > 0);
  return &parser->implementation->structural_indexes[n_structural_indexes - 1];
}
simdjson_inline const uint8_t *json_iterator::peek_last() const noexcept {
  return token.peek(last_position());
}

simdjson_inline void json_iterator::ascend_to(depth_t parent_depth) noexcept {
  SIMDJSON_ASSUME(parent_depth >= 0 && parent_depth < INT32_MAX - 1);
  SIMDJSON_ASSUME(_depth == parent_depth + 1);
  _depth = parent_depth;
}

simdjson_inline void json_iterator::descend_to(depth_t child_depth) noexcept {
  SIMDJSON_ASSUME(child_depth >= 1 && child_depth < INT32_MAX);
  SIMDJSON_ASSUME(_depth == child_depth - 1);
  _depth = child_depth;
}

simdjson_inline depth_t json_iterator::depth() const noexcept {
  return _depth;
}

simdjson_inline uint8_t *&json_iterator::string_buf_loc() noexcept {
  return _string_buf_loc;
}

simdjson_warn_unused simdjson_inline error_code json_iterator::report_error(error_code _error, const char *message) noexcept {
  SIMDJSON_ASSUME(_error != SUCCESS && _error != UNINITIALIZED && _error != INCORRECT_TYPE && _error != NO_SUCH_FIELD);
  logger::log_error(*this, message);
  error = _error;
  return error;
}

simdjson_inline token_position json_iterator::position() const noexcept {
  return token.position();
}

simdjson_inline simdjson_result<std::string_view> json_iterator::unescape(raw_json_string in, bool allow_replacement) noexcept {
#if SIMDJSON_DEVELOPMENT_CHECKS
  auto result = parser->unescape(in, _string_buf_loc, allow_replacement);
#if !defined(SIMDJSON_VISUAL_STUDIO) && !defined(SIMDJSON_CLANG_VISUAL_STUDIO)
  // Under Visual Studio, the next SIMDJSON_ASSUME fails with: the argument
  // has side effects that will be discarded.
  SIMDJSON_ASSUME(!parser->string_buffer_overflow(_string_buf_loc));
#endif // !defined(SIMDJSON_VISUAL_STUDIO) && !defined(SIMDJSON_CLANG_VISUAL_STUDIO)
  return result;
#else
  return parser->unescape(in, _string_buf_loc, allow_replacement);
#endif
}

template <typename string_type>
simdjson_warn_unused simdjson_inline error_code json_iterator::unescape(raw_json_string in, string_type& receiver, bool allow_replacement) noexcept {
// The inline capacity of std::string must be known at compile time, hence constexpr std::string.
#if SIMDJSON_SUPPORTS_RESIZE_AND_OVERWRITE && SIMDJSON_SUPPORTS_CONSTEXPR_STRING
  if constexpr (std::is_same_v<string_type, std::string>) {
    // Short strings without escapes go into the string's inline (SSO) buffer with two overlapping
    // copies of fixed size. A memcpy of the exact length is much slower here, because its branches
    // on the length mispredict. Like parse_string, this reads past the closing quote into the padding.
    constexpr size_t sso_capacity = std::string().capacity(); // 22 with libc++, 15 with libstdc++ and MSVC
    constexpr size_t chunk = sso_capacity >= 16 ? 16 : 8;
    if constexpr (sso_capacity >= chunk && sso_capacity <= 2 * chunk) {
      const uint8_t *const start = reinterpret_cast<const uint8_t *>(in.raw());
      uint8_t scratch[backslash_and_quote::BYTES_PROCESSED];
      for (size_t offset = 0; offset <= sso_capacity; offset += backslash_and_quote::BYTES_PROCESSED) {
        auto bs_quote = backslash_and_quote{}.copy_and_find(start + offset, scratch);
        if (bs_quote.has_quote_first()) {
          const size_t length = offset + size_t(bs_quote.quote_index());
          if (length > sso_capacity) { break; }
          receiver.resize_and_overwrite(sso_capacity, [&](char *dst, size_t) noexcept {
            std::memcpy(dst, start, chunk);
            std::memcpy(dst + sso_capacity - chunk, start + sso_capacity - chunk, chunk);
            return length;
          });
          return SUCCESS;
        }
        if (bs_quote.has_backslash()) { break; }
      }
    }
  }
#endif // SIMDJSON_SUPPORTS_RESIZE_AND_OVERWRITE && SIMDJSON_SUPPORTS_CONSTEXPR_STRING

  // Anything else: unescape into the string buffer, copy it out, then give the buffer space back.
  auto saved_string_buf_loc = _string_buf_loc;
  std::string_view content;
  SIMDJSON_TRY( unescape(in, allow_replacement).get(content) );
  internal::assign_utf8(receiver, content);
  _string_buf_loc = saved_string_buf_loc;
  return SUCCESS;
}

simdjson_inline simdjson_result<std::string_view> json_iterator::unescape_wobbly(raw_json_string in) noexcept {
#if SIMDJSON_DEVELOPMENT_CHECKS
  auto result = parser->unescape_wobbly(in, _string_buf_loc);
#if !defined(SIMDJSON_VISUAL_STUDIO) && !defined(SIMDJSON_CLANG_VISUAL_STUDIO)
  // Under Visual Studio, the next SIMDJSON_ASSUME fails with: the argument
  // has side effects that will be discarded.
  SIMDJSON_ASSUME(!parser->string_buffer_overflow(_string_buf_loc));
#endif // !defined(SIMDJSON_VISUAL_STUDIO) && !defined(SIMDJSON_CLANG_VISUAL_STUDIO)
  return result;
#else
  return parser->unescape_wobbly(in, _string_buf_loc);
#endif
}

simdjson_inline void json_iterator::reenter_child(token_position position, depth_t child_depth) noexcept {
  SIMDJSON_ASSUME(child_depth >= 1 && child_depth < INT32_MAX);
  SIMDJSON_ASSUME(_depth == child_depth - 1);
#if SIMDJSON_DEVELOPMENT_CHECKS
#ifndef SIMDJSON_CLANG_VISUAL_STUDIO
  SIMDJSON_ASSUME(size_t(child_depth) < parser->max_depth());
  SIMDJSON_ASSUME(position >= parser->start_positions[child_depth]);
#endif
#endif
  token.set_position(position);
  _depth = child_depth;
}

simdjson_warn_unused simdjson_inline error_code json_iterator::consume_character(char c) noexcept {
  if (*peek() == c) {
    return_current_and_advance();
    return SUCCESS;
  }
  return TAPE_ERROR;
}

#if SIMDJSON_DEVELOPMENT_CHECKS

simdjson_inline token_position json_iterator::start_position(depth_t depth) const noexcept {
  SIMDJSON_ASSUME(size_t(depth) < parser->max_depth());
  return size_t(depth) < parser->max_depth() ? parser->start_positions[depth] : 0;
}

simdjson_inline void json_iterator::set_start_position(depth_t depth, token_position position) noexcept {
  SIMDJSON_ASSUME(size_t(depth) < parser->max_depth());
  if(size_t(depth) < parser->max_depth()) { parser->start_positions[depth] = position; }
}

#endif


simdjson_warn_unused simdjson_inline error_code json_iterator::optional_error(error_code _error, const char *message) noexcept {
  SIMDJSON_ASSUME(_error == INCORRECT_TYPE || _error == NO_SUCH_FIELD);
  logger::log_error(*this, message);
  return _error;
}


simdjson_warn_unused simdjson_inline bool json_iterator::copy_to_buffer(const uint8_t *json, uint32_t max_len, uint8_t *tmpbuf, size_t N) noexcept {
  // This function is not expected to be called in performance-sensitive settings.
  // peek_root_length() stops at the next structural index. Stage 1 omits the
  // comma or RS (0x1E) that separates stream documents, so that byte and the
  // whitespace around it remain in the span. RS is not a valid number finisher.
  // Outside of a stream, the comma and RS are not separators: some kernels
  // keep RS in the scalar token, and dropping it would accept "1\x1e".
  while (max_len > 0) {
    const uint8_t last = json[max_len - 1];
    // iterate_allow_incomplete_json() reuses _streaming, so a root scalar
    // there is not a document_stream.
#ifdef SIMDJSON_EXPERIMENTAL_ALLOW_INCOMPLETE_JSON
    const bool separator = _streaming && !_allow_incomplete_json &&
                           (last == ',' || last == 0x1E);
#else
    const bool separator = _streaming && (last == ',' || last == 0x1E);
#endif
    if (last != ' ' && last != '\t' && last != '\n' && last != '\r' &&
        !separator) {
      break;
    }
    max_len--;
  }
  // Let us guard against silly cases:
  if((N < max_len) || (N == 0)) { return false; }
  // Copy to the buffer.
  std::memcpy(tmpbuf, json, max_len);
  if(N > max_len) { // We pad whatever remains with ' '.
    std::memset(tmpbuf + max_len, ' ', N - max_len);
  }
  return true;
}

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

namespace simdjson {

simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>::simdjson_result(SIMDJSON_IMPLEMENTATION::ondemand::json_iterator &&value) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(std::forward<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(value)) {}
simdjson_inline simdjson_result<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>::simdjson_result(error_code error) noexcept
    : implementation_simdjson_result_base<SIMDJSON_IMPLEMENTATION::ondemand::json_iterator>(error) {}

} // namespace simdjson

#endif // SIMDJSON_GENERIC_ONDEMAND_JSON_ITERATOR_INL_H
