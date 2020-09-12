#include "fallback/begin_implementation.h"
#include "fallback/dom_parser_implementation.h"
#include "generic/stage2/jsoncharutils.h"

//
// Stage 1
//
#include "generic/stage1/find_next_document_index.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

class structural_scanner {
public:

simdjson_really_inline structural_scanner(dom_parser_implementation &_parser, bool _partial)
  : buf{_parser.buf},
    next_structural_index{_parser.structural_indexes.get()},
    parser{_parser},
    len{static_cast<uint32_t>(_parser.len)},
    partial{_partial} {
}

simdjson_really_inline void add_structural() {
  *next_structural_index = idx;
  next_structural_index++;
}

simdjson_really_inline bool is_continuation(uint8_t c) {
  return (c & 0b11000000) == 0b10000000;
}

simdjson_really_inline void validate_utf8_character() {
  // Continuation
  if (simdjson_unlikely((buf[idx] & 0b01000000) == 0)) {
    // extra continuation
    error = UTF8_ERROR;
    idx++;
    return;
  }

  // 2-byte
  if ((buf[idx] & 0b00100000) == 0) {
    // missing continuation
    if (simdjson_unlikely(idx+1 > len || !is_continuation(buf[idx+1]))) {
      if (idx+1 > len && partial) { idx = len; return; }
      error = UTF8_ERROR;
      idx++;
      return;
    }
    // overlong: 1100000_ 10______
    if (buf[idx] <= 0b11000001) { error = UTF8_ERROR; }
    idx += 2;
    return;
  }

  // 3-byte
  if ((buf[idx] & 0b00010000) == 0) {
    // missing continuation
    if (simdjson_unlikely(idx+2 > len || !is_continuation(buf[idx+1]) || !is_continuation(buf[idx+2]))) {
      if (idx+2 > len && partial) { idx = len; return; }
      error = UTF8_ERROR;
      idx++;
      return;
    }
    // overlong: 11100000 100_____ ________
    if (buf[idx] == 0b11100000 && buf[idx+1] <= 0b10011111) { error = UTF8_ERROR; }
    // surrogates: U+D800-U+DFFF 11101101 101_____
    if (buf[idx] == 0b11101101 && buf[idx+1] >= 0b10100000) { error = UTF8_ERROR; }
    idx += 3;
    return;
  }

  // 4-byte
  // missing continuation
  if (simdjson_unlikely(idx+3 > len || !is_continuation(buf[idx+1]) || !is_continuation(buf[idx+2]) || !is_continuation(buf[idx+3]))) {
    if (idx+2 > len && partial) { idx = len; return; }
    error = UTF8_ERROR;
    idx++;
    return;
  }
  // overlong: 11110000 1000____ ________ ________
  if (buf[idx] == 0b11110000 && buf[idx+1] <= 0b10001111) { error = UTF8_ERROR; }
  // too large: > U+10FFFF:
  // 11110100 (1001|101_)____
  // 1111(1___|011_|0101) 10______
  // also includes 5, 6, 7 and 8 byte characters:
  // 11111___
  if (buf[idx] == 0b11110100 && buf[idx+1] >= 0b10010000) { error = UTF8_ERROR; }
  if (buf[idx] >= 0b11110101) { error = UTF8_ERROR; }
  idx += 4;
}

simdjson_really_inline void validate_string() {
  idx++; // skip first quote
  while (idx < len && buf[idx] != '"') {
    if (buf[idx] == '\\') {
      idx += 2;
    } else if (simdjson_unlikely(buf[idx] & 0b10000000)) {
      validate_utf8_character();
    } else {
      if (buf[idx] < 0x20) { error = UNESCAPED_CHARS; }
      idx++;
    }
  }
  if (idx >= len && !partial) { error = UNCLOSED_STRING; }
}

simdjson_really_inline bool is_whitespace_or_operator(uint8_t c) {
  switch (c) {
    case '{': case '}': case '[': case ']': case ',': case ':':
    case ' ': case '\r': case '\n': case '\t':
      return true;
    default:
      return false;
  }
}

//
// Parse the entire input in STEP_SIZE-byte chunks.
//
simdjson_really_inline error_code scan() {
  for (;idx<len;idx++) {
    switch (buf[idx]) {
      // String
      case '"':
        add_structural();
        validate_string();
        break;
      // Operator
      case '{': case '}': case '[': case ']': case ',': case ':':
        add_structural();
        break;
      // Whitespace
      case ' ': case '\r': case '\n': case '\t':
        break;
      // Primitive or invalid character (invalid characters will be checked in stage 2)
      default:
        // Anything else, add the structural and go until we find the next one
        add_structural();
        while (idx+1<len && !is_whitespace_or_operator(buf[idx+1])) {
          idx++;
        };
        break;
    }
  }
  *next_structural_index = len;
  // We pad beyond.
  // https://github.com/simdjson/simdjson/issues/906
  next_structural_index[1] = len;
  next_structural_index[2] = 0;
  parser.n_structural_indexes = uint32_t(next_structural_index - parser.structural_indexes.get());
  parser.next_structural_index = 0;

  if (simdjson_unlikely(parser.n_structural_indexes == 0)) {
    return EMPTY;
  }

  if (partial) {
    auto new_structural_indexes = find_next_document_index(parser);
    if (new_structural_indexes == 0 && parser.n_structural_indexes > 0) {
      return CAPACITY; // If the buffer is partial but the document is incomplete, it's too big to parse.
    }
    parser.n_structural_indexes = new_structural_indexes;
  }

  return error;
}

private:
  const uint8_t *buf;
  uint32_t *next_structural_index;
  dom_parser_implementation &parser;
  uint32_t len;
  uint32_t idx{0};
  error_code error{SUCCESS};
  bool partial;
}; // structural_scanner

} // namespace stage1

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, bool partial) noexcept {
  this->buf = _buf;
  this->len = _len;
  stage1::structural_scanner scanner(*this, partial);
  return scanner.scan();
}

// big table for the minifier
static uint8_t jump_table[256 * 3] = {
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0,
    1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
    1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1,
    1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
    0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1,
};

SIMDJSON_WARN_UNUSED error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
  size_t i = 0, pos = 0;
  uint8_t quote = 0;
  uint8_t nonescape = 1;

  while (i < len) {
    unsigned char c = buf[i];
    uint8_t *meta = jump_table + 3 * c;

    quote = quote ^ (meta[0] & nonescape);
    dst[pos] = c;
    pos += meta[2] | quote;

    i += 1;
    nonescape = uint8_t(~nonescape) | (meta[1]);
  }
  dst_len = pos; // we intentionally do not work with a reference
  // for fear of aliasing
  return quote ? UNCLOSED_STRING : SUCCESS;
}

// credit: based on code from Google Fuchsia (Apache Licensed)
SIMDJSON_WARN_UNUSED bool implementation::validate_utf8(const char *buf, size_t len) const noexcept { 
  const uint8_t *data = (const uint8_t *)buf;
  uint64_t pos = 0;
  uint64_t next_pos = 0;
  uint32_t code_point = 0;
  while (pos < len) {
    // check of the next 8 bytes are ascii.
    next_pos = pos + 16;
    if (next_pos <= len) { // if it is safe to read 8 more bytes, check that they are ascii
      uint64_t v1;
      memcpy(&v1, data + pos, sizeof(uint64_t));
      uint64_t v2;
      memcpy(&v2, data + pos + sizeof(uint64_t), sizeof(uint64_t));
      uint64_t v{v1 | v2};
      if ((v & 0x8080808080808080) == 0) {
        pos = next_pos;
        continue;
      }
    }
    unsigned char byte = data[pos];
    if (byte < 0b10000000) {
      pos++;
      continue;
    } else if ((byte & 0b11100000) == 0b11000000) {
      next_pos = pos + 2;
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
      // range check
      code_point = (byte & 0b00011111) << 6 | (data[pos + 1] & 0b00111111);
      if (code_point < 0x80 || 0x7ff < code_point) { return false; }
    } else if ((byte & 0b11110000) == 0b11100000) {
      next_pos = pos + 3;
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) { return false; }
      // range check
      code_point = (byte & 0b00001111) << 12 |
                   (data[pos + 1] & 0b00111111) << 6 |
                   (data[pos + 2] & 0b00111111);
      if (code_point < 0x800 || 0xffff < code_point ||
          (0xd7ff < code_point && code_point < 0xe000)) {
        return false;
      }
    } else if ((byte & 0b11111000) == 0b11110000) { // 0b11110000
      next_pos = pos + 4;
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0b11000000) != 0b10000000) { return false; }
      if ((data[pos + 2] & 0b11000000) != 0b10000000) { return false; }
      if ((data[pos + 3] & 0b11000000) != 0b10000000) { return false; }
      // range check
      code_point =
          (byte & 0b00000111) << 18 | (data[pos + 1] & 0b00111111) << 12 |
          (data[pos + 2] & 0b00111111) << 6 | (data[pos + 3] & 0b00111111);
      if (code_point < 0xffff || 0x10ffff < code_point) { return false; }
    } else {
      // we may have a continuation
      return false;
    }
    pos = next_pos;
  }
  return true;
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

//
// Stage 2
//
#include "fallback/stringparsing.h"
#include "fallback/numberparsing.h"
#include "generic/stage2/tape_builder.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

SIMDJSON_WARN_UNUSED error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, false);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace

#include "fallback/end_implementation.h"