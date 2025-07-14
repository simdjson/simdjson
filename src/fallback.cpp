#ifndef SIMDJSON_SRC_FALLBACK_CPP
#define SIMDJSON_SRC_FALLBACK_CPP

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include <base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <simdjson/fallback.h>
#include <simdjson/fallback/implementation.h>

#include <simdjson/fallback/begin.h>
#include <generic/stage1/find_next_document_index.h>
#include <generic/stage2/stringparsing.h>
#include <generic/stage2/logger.h>
#include <generic/stage2/json_iterator.h>
#include <generic/stage2/tape_writer.h>
#include <generic/stage2/tape_builder.h>

//
// Stage 1
//

namespace simdjson {
namespace fallback {

simdjson_warn_unused error_code implementation::create_dom_parser_implementation(
  size_t capacity,
  size_t max_depth,
  std::unique_ptr<internal::dom_parser_implementation>& dst
) const noexcept {
  dst.reset( new (std::nothrow) SIMDJSON_IMPLEMENTATION::dom_parser_implementation() );
  if (!dst) { return MEMALLOC; }
  if (auto err = dst->set_capacity(capacity))
    return err;
  if (auto err = dst->set_max_depth(max_depth))
    return err;
  return SUCCESS;
}

namespace {
namespace stage1 {

class structural_scanner {
public:

simdjson_inline structural_scanner(dom_parser_implementation &_parser, stage1_mode _partial)
  : buf{_parser.buf},
    next_structural_index{_parser.structural_indexes.get()},
    parser{_parser},
    len{static_cast<uint32_t>(_parser.len)},
    partial{_partial} {
}

simdjson_inline void add_structural() {
  *next_structural_index = idx;
  next_structural_index++;
}

simdjson_inline bool is_continuation(uint8_t c) {
  return (c & 0xc0) == 0x80;
}

simdjson_inline void validate_utf8_character() {
  // Continuation
  if (simdjson_unlikely((buf[idx] & 0x40) == 0)) {
    // extra continuation
    error = UTF8_ERROR;
    idx++;
    return;
  }

  // 2-byte
  if ((buf[idx] & 0x20) == 0) {
    // missing continuation
    if (simdjson_unlikely(idx+1 > len || !is_continuation(buf[idx+1]))) {
      if (idx+1 > len && is_streaming(partial)) { idx = len; return; }
      error = UTF8_ERROR;
      idx++;
      return;
    }
    // overlong: 1100000_ 10______
    if (buf[idx] <= 0xc1) { error = UTF8_ERROR; }
    idx += 2;
    return;
  }

  // 3-byte
  if ((buf[idx] & 0x10) == 0) {
    // missing continuation
    if (simdjson_unlikely(idx+2 > len || !is_continuation(buf[idx+1]) || !is_continuation(buf[idx+2]))) {
      if (idx+2 > len && is_streaming(partial)) { idx = len; return; }
      error = UTF8_ERROR;
      idx++;
      return;
    }
    // overlong: 11100000 100_____ ________
    if (buf[idx] == 0xe0 && buf[idx+1] <= 0x9f) { error = UTF8_ERROR; }
    // surrogates: U+D800-U+DFFF 11101101 101_____
    if (buf[idx] == 0xed && buf[idx+1] >= 0xa0) { error = UTF8_ERROR; }
    idx += 3;
    return;
  }

  // 4-byte
  // missing continuation
  if (simdjson_unlikely(idx+3 > len || !is_continuation(buf[idx+1]) || !is_continuation(buf[idx+2]) || !is_continuation(buf[idx+3]))) {
    if (idx+2 > len && is_streaming(partial)) { idx = len; return; }
    error = UTF8_ERROR;
    idx++;
    return;
  }
  // overlong: 11110000 1000____ ________ ________
  if (buf[idx] == 0xf0 && buf[idx+1] <= 0x8f) { error = UTF8_ERROR; }
  // too large: > U+10FFFF:
  // 11110100 (1001|101_)____
  // 1111(1___|011_|0101) 10______
  // also includes 5, 6, 7 and 8 byte characters:
  // 11111___
  if (buf[idx] == 0xf4 && buf[idx+1] >= 0x90) { error = UTF8_ERROR; }
  if (buf[idx] >= 0xf5) { error = UTF8_ERROR; }
  idx += 4;
}

// Returns true if the string is unclosed.
simdjson_inline bool validate_string() {
  idx++; // skip first quote
  while (idx < len && buf[idx] != '"') {
    if (buf[idx] == '\\') {
      idx += 2;
    } else if (simdjson_unlikely(buf[idx] & 0x80)) {
      validate_utf8_character();
    } else {
      if (buf[idx] < 0x20) { error = UNESCAPED_CHARS; }
      idx++;
    }
  }
  if (idx >= len) { return true; }
  return false;
}

simdjson_inline bool is_whitespace_or_operator(uint8_t c) {
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
simdjson_inline error_code scan() {
  bool unclosed_string = false;
  for (;idx<len;idx++) {
    switch (buf[idx]) {
      // String
      case '"':
        add_structural();
        unclosed_string |= validate_string();
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
  // We pad beyond.
  // https://github.com/simdjson/simdjson/issues/906
  // See json_structural_indexer.h for an explanation.
  *next_structural_index = len; // assumed later in partial == stage1_mode::streaming_final
  next_structural_index[1] = len;
  next_structural_index[2] = 0;
  parser.n_structural_indexes = uint32_t(next_structural_index - parser.structural_indexes.get());
  if (simdjson_unlikely(parser.n_structural_indexes == 0)) { return EMPTY; }
  parser.next_structural_index = 0;
  if (partial == stage1_mode::streaming_partial) {
    if(unclosed_string) {
      parser.n_structural_indexes--;
      if (simdjson_unlikely(parser.n_structural_indexes == 0)) { return CAPACITY; }
    }
    // We truncate the input to the end of the last complete document (or zero).
    auto new_structural_indexes = find_next_document_index(parser);
    if (new_structural_indexes == 0 && parser.n_structural_indexes > 0) {
      if(parser.structural_indexes[0] == 0) {
        // If the buffer is partial and we started at index 0 but the document is
        // incomplete, it's too big to parse.
        return CAPACITY;
      } else {
        // It is possible that the document could be parsed, we just had a lot
        // of white space.
        parser.n_structural_indexes = 0;
        return EMPTY;
      }
    }
    parser.n_structural_indexes = new_structural_indexes;
  } else if(partial == stage1_mode::streaming_final) {
    if(unclosed_string) { parser.n_structural_indexes--; }
    // We truncate the input to the end of the last complete document (or zero).
    // Because partial == stage1_mode::streaming_final, it means that we may
    // silently ignore trailing garbage. Though it sounds bad, we do it
    // deliberately because many people who have streams of JSON documents
    // will truncate them for processing. E.g., imagine that you are uncompressing
    // the data from a size file or receiving it in chunks from the network. You
    // may not know where exactly the last document will be. Meanwhile the
    // document_stream instances allow people to know the JSON documents they are
    // parsing (see the iterator.source() method).
    parser.n_structural_indexes = find_next_document_index(parser);
    // We store the initial n_structural_indexes so that the client can see
    // whether we used truncation. If initial_n_structural_indexes == parser.n_structural_indexes,
    // then this will query parser.structural_indexes[parser.n_structural_indexes] which is len,
    // otherwise, it will copy some prior index.
    parser.structural_indexes[parser.n_structural_indexes + 1] = parser.structural_indexes[parser.n_structural_indexes];
    // This next line is critical, do not change it unless you understand what you are
    // doing.
    parser.structural_indexes[parser.n_structural_indexes] = uint32_t(len);
    if (parser.n_structural_indexes == 0) { return EMPTY; }
  } else if(unclosed_string) { error = UNCLOSED_STRING; }
  return error;
}

private:
  const uint8_t *buf;
  uint32_t *next_structural_index;
  dom_parser_implementation &parser;
  uint32_t len;
  uint32_t idx{0};
  error_code error{SUCCESS};
  stage1_mode partial;
}; // structural_scanner

} // namespace stage1
} // unnamed namespace

simdjson_warn_unused error_code dom_parser_implementation::stage1(const uint8_t *_buf, size_t _len, stage1_mode partial) noexcept {
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

simdjson_warn_unused error_code implementation::minify(const uint8_t *buf, size_t len, uint8_t *dst, size_t &dst_len) const noexcept {
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
simdjson_warn_unused bool implementation::validate_utf8(const char *buf, size_t len) const noexcept {
  const uint8_t *data = reinterpret_cast<const uint8_t *>(buf);
  uint64_t pos = 0;
  uint32_t code_point = 0;
  while (pos < len) {
    // check of the next 8 bytes are ascii.
    uint64_t next_pos = pos + 16;
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
    if (byte < 0x80) {
      pos++;
      continue;
    } else if ((byte & 0xe0) == 0xc0) {
      next_pos = pos + 2;
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0xc0) != 0x80) { return false; }
      // range check
      code_point = (byte & 0x1f) << 6 | (data[pos + 1] & 0x3f);
      if (code_point < 0x80 || 0x7ff < code_point) { return false; }
    } else if ((byte & 0xf0) == 0xe0) {
      next_pos = pos + 3;
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0xc0) != 0x80) { return false; }
      if ((data[pos + 2] & 0xc0) != 0x80) { return false; }
      // range check
      code_point = (byte & 0x0f) << 12 |
                   (data[pos + 1] & 0x3f) << 6 |
                   (data[pos + 2] & 0x3f);
      if (code_point < 0x800 || 0xffff < code_point ||
          (0xd7ff < code_point && code_point < 0xe000)) {
        return false;
      }
    } else if ((byte & 0xf8) == 0xf0) { // 0b11110000
      next_pos = pos + 4;
      if (next_pos > len) { return false; }
      if ((data[pos + 1] & 0xc0) != 0x80) { return false; }
      if ((data[pos + 2] & 0xc0) != 0x80) { return false; }
      if ((data[pos + 3] & 0xc0) != 0x80) { return false; }
      // range check
      code_point =
          (byte & 0x07) << 18 | (data[pos + 1] & 0x3f) << 12 |
          (data[pos + 2] & 0x3f) << 6 | (data[pos + 3] & 0x3f);
      if (code_point <= 0xffff || 0x10ffff < code_point) { return false; }
    } else {
      // we may have a continuation
      return false;
    }
    pos = next_pos;
  }
  return true;
}

} // namespace fallback
} // namespace simdjson

//
// Stage 2
//

namespace simdjson {
namespace fallback {

simdjson_warn_unused error_code dom_parser_implementation::stage2(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<false>(*this, _doc);
}

simdjson_warn_unused error_code dom_parser_implementation::stage2_next(dom::document &_doc) noexcept {
  return stage2::tape_builder::parse_document<true>(*this, _doc);
}

SIMDJSON_NO_SANITIZE_MEMORY
simdjson_warn_unused uint8_t *dom_parser_implementation::parse_string(const uint8_t *src, uint8_t *dst, bool replacement_char) const noexcept {
  return fallback::stringparsing::parse_string(src, dst, replacement_char);
}

simdjson_warn_unused uint8_t *dom_parser_implementation::parse_wobbly_string(const uint8_t *src, uint8_t *dst) const noexcept {
  return fallback::stringparsing::parse_wobbly_string(src, dst);
}

simdjson_warn_unused error_code dom_parser_implementation::parse(const uint8_t *_buf, size_t _len, dom::document &_doc) noexcept {
  auto error = stage1(_buf, _len, stage1_mode::regular);
  if (error) { return error; }
  return stage2(_doc);
}

} // namespace fallback
} // namespace simdjson

#include <simdjson/fallback/end.h>

#endif // SIMDJSON_SRC_FALLBACK_CPP
