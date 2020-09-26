// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1.h" (this simplifies amalgation)

#include "generic/stage1/buf_block_reader.h"
#include "generic/stage1/json_string_scanner.h"
#include "generic/stage1/json_scanner.h"
#include "generic/stage1/json_minifier.h"
#include "generic/stage1/find_next_document_index.h"

namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage1 {

class bit_indexer {
public:
  uint32_t *tail;

  simdjson_really_inline bit_indexer(uint32_t *index_buf) : tail(index_buf) {}

  // flatten out values in 'bits' assuming that they are are to have values of idx
  // plus their position in the bitvector, and store these indexes at
  // base_ptr[base] incrementing base as we go
  // will potentially store extra values beyond end of valid bits, so base_ptr
  // needs to be large enough to handle this
  simdjson_really_inline void write(uint32_t idx, uint64_t bits) {
    // In some instances, the next branch is expensive because it is mispredicted.
    // Unfortunately, in other cases,
    // it helps tremendously.
    if (bits == 0)
        return;
    int cnt = static_cast<int>(count_ones(bits));

    // Do the first 8 all together
    for (int i=0; i<8; i++) {
      this->tail[i] = idx + trailing_zeroes(bits);
      bits = clear_lowest_bit(bits);
    }

    // Do the next 8 all together (we hope in most cases it won't happen at all
    // and the branch is easily predicted).
    if (simdjson_unlikely(cnt > 8)) {
      for (int i=8; i<16; i++) {
        this->tail[i] = idx + trailing_zeroes(bits);
        bits = clear_lowest_bit(bits);
      }

      // Most files don't have 16+ structurals per block, so we take several basically guaranteed
      // branch mispredictions here. 16+ structurals per block means either punctuation ({} [] , :)
      // or the start of a value ("abc" true 123) every four characters.
      if (simdjson_unlikely(cnt > 16)) {
        int i = 16;
        do {
          this->tail[i] = idx + trailing_zeroes(bits);
          bits = clear_lowest_bit(bits);
          i++;
        } while (i < cnt);
      }
    }

    this->tail += cnt;
  }
};

class json_structural_indexer {
public:
  /**
   * Find the important bits of JSON in a 128-byte chunk, and add them to structural_indexes.
   *
   * @param partial Setting the partial parameter to true allows the find_structural_bits to
   *   tolerate unclosed strings. The caller should still ensure that the input is valid UTF-8. If
   *   you are processing substrings, you may want to call on a function like trimmed_length_safe_utf8.
   */
  template<size_t STEP_SIZE>
  static error_code index(const uint8_t *buf, size_t len, dom_parser_implementation &parser, bool partial) noexcept;

private:
  simdjson_really_inline json_structural_indexer(uint32_t *structural_indexes);
  template<size_t STEP_SIZE>
  simdjson_really_inline void step(const uint8_t *block, buf_block_reader<STEP_SIZE> &reader) noexcept;
  simdjson_really_inline void next(const simd::simd8x64<uint8_t>& in, json_block block, size_t idx);
  simdjson_really_inline error_code finish(dom_parser_implementation &parser, size_t idx, size_t len, bool partial);

  json_scanner scanner{};
  utf8_checker checker{};
  bit_indexer indexer;
  uint64_t prev_structurals = 0;
  uint64_t unescaped_chars_error = 0;
};

simdjson_really_inline json_structural_indexer::json_structural_indexer(uint32_t *structural_indexes) : indexer{structural_indexes} {}

// Skip the last character if it is partial
simdjson_really_inline size_t trim_partial_utf8(const uint8_t *buf, size_t len) {
  if (simdjson_unlikely(len < 3)) {
    switch (len) {
      case 2:
        if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
        if (buf[len-2] >= 0b11100000) { return len-2; } // 3- and 4-byte characters with only 2 bytes left
        return len;
      case 1:
        if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
        return len;
      case 0:
        return len;
    }
  }
  if (buf[len-1] >= 0b11000000) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
  if (buf[len-2] >= 0b11100000) { return len-2; } // 3- and 4-byte characters with only 1 byte left
  if (buf[len-3] >= 0b11110000) { return len-3; } // 4-byte characters with only 3 bytes left
  return len;
}

//
// PERF NOTES:
// We pipe 2 inputs through these stages:
// 1. Load JSON into registers. This takes a long time and is highly parallelizable, so we load
//    2 inputs' worth at once so that by the time step 2 is looking for them input, it's available.
// 2. Scan the JSON for critical data: strings, scalars and operators. This is the critical path.
//    The output of step 1 depends entirely on this information. These functions don't quite use
//    up enough CPU: the second half of the functions is highly serial, only using 1 execution core
//    at a time. The second input's scans has some dependency on the first ones finishing it, but
//    they can make a lot of progress before they need that information.
// 3. Step 1 doesn't use enough capacity, so we run some extra stuff while we're waiting for that
//    to finish: utf-8 checks and generating the output from the last iteration.
//
// The reason we run 2 inputs at a time, is steps 2 and 3 are *still* not enough to soak up all
// available capacity with just one input. Running 2 at a time seems to give the CPU a good enough
// workout.
//
template<size_t STEP_SIZE>
error_code json_structural_indexer::index(const uint8_t *buf, size_t len, dom_parser_implementation &parser, bool partial) noexcept {
  if (simdjson_unlikely(len > parser.capacity())) { return CAPACITY; }
  if (partial) { len = trim_partial_utf8(buf, len); }

  buf_block_reader<STEP_SIZE> reader(buf, len);
  json_structural_indexer indexer(parser.structural_indexes.get());

  // Read all but the last block
  while (reader.has_full_block()) {
    indexer.step<STEP_SIZE>(reader.full_block(), reader);
  }

  // Take care of the last block (will always be there unless file is empty)
  uint8_t block[STEP_SIZE];
  if (simdjson_unlikely(reader.get_remainder(block) == 0)) { return EMPTY; }
  indexer.step<STEP_SIZE>(block, reader);

  return indexer.finish(parser, reader.block_index(), len, partial);
}

template<>
simdjson_really_inline void json_structural_indexer::step<128>(const uint8_t *block, buf_block_reader<128> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block);
  simd::simd8x64<uint8_t> in_2(block+64);
  json_block block_1 = scanner.next(in_1);
  json_block block_2 = scanner.next(in_2);
  this->next(in_1, block_1, reader.block_index());
  this->next(in_2, block_2, reader.block_index()+64);
  reader.advance();
}

template<>
simdjson_really_inline void json_structural_indexer::step<64>(const uint8_t *block, buf_block_reader<64> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block);
  json_block block_1 = scanner.next(in_1);
  this->next(in_1, block_1, reader.block_index());
  reader.advance();
}

simdjson_really_inline void json_structural_indexer::next(const simd::simd8x64<uint8_t>& in, json_block block, size_t idx) {
  uint64_t unescaped = in.lteq(0x1F);
  checker.check_next_input(in);
  indexer.write(uint32_t(idx-64), prev_structurals); // Output *last* iteration's structurals to the parser
  prev_structurals = block.structural_start();
  unescaped_chars_error |= block.non_quote_inside_string(unescaped);
}

simdjson_really_inline error_code json_structural_indexer::finish(dom_parser_implementation &parser, size_t idx, size_t len, bool partial) {
  // Write out the final iteration's structurals
  indexer.write(uint32_t(idx-64), prev_structurals);

  error_code error = scanner.finish(partial);
  if (simdjson_unlikely(error != SUCCESS)) { return error; }

  if (unescaped_chars_error) {
    return UNESCAPED_CHARS;
  }

  parser.n_structural_indexes = uint32_t(indexer.tail - parser.structural_indexes.get());
  /***
   * This is related to https://github.com/simdjson/simdjson/issues/906
   * Basically, we want to make sure that if the parsing continues beyond the last (valid)
   * structural character, it quickly stops.
   * Only three structural characters can be repeated without triggering an error in JSON:  [,] and }.
   * We repeat the padding character (at 'len'). We don't know what it is, but if the parsing
   * continues, then it must be [,] or }.
   * Suppose it is ] or }. We backtrack to the first character, what could it be that would
   * not trigger an error? It could be ] or } but no, because you can't start a document that way.
   * It can't be a comma, a colon or any simple value. So the only way we could continue is
   * if the repeated character is [. But if so, the document must start with [. But if the document
   * starts with [, it should end with ]. If we enforce that rule, then we would get
   * ][[ which is invalid.
   **/
  parser.structural_indexes[parser.n_structural_indexes] = uint32_t(len);
  parser.structural_indexes[parser.n_structural_indexes + 1] = uint32_t(len);
  parser.structural_indexes[parser.n_structural_indexes + 2] = 0;
  parser.next_structural_index = 0;
  // a valid JSON file cannot have zero structural indexes - we should have found something
  if (simdjson_unlikely(parser.n_structural_indexes == 0u)) {
    return EMPTY;
  }
  if (simdjson_unlikely(parser.structural_indexes[parser.n_structural_indexes - 1] > len)) {
    return UNEXPECTED_ERROR;
  }
  if (partial) {
    auto new_structural_indexes = find_next_document_index(parser);
    if (new_structural_indexes == 0 && parser.n_structural_indexes > 0) {
      return CAPACITY; // If the buffer is partial but the document is incomplete, it's too big to parse.
    }
    parser.n_structural_indexes = new_structural_indexes;
  }
  checker.check_eof();
  return checker.errors();
}

} // namespace stage1
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
