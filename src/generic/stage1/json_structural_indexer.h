#ifndef SIMDJSON_SRC_GENERIC_STAGE1_JSON_STRUCTURAL_INDEXER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE1_JSON_STRUCTURAL_INDEXER_H
#include <generic/stage1/base.h>
#include <generic/stage1/utf8_lookup4_algorithm.h>
#include <generic/stage1/buf_block_reader.h>
#include <generic/stage1/json_string_scanner.h>
#include <generic/stage1/json_scanner.h>
#include <generic/stage1/json_minifier.h>
#include <generic/stage1/find_next_document_index.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1.h" (this simplifies amalgation)

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace stage1 {

class bit_indexer {
public:
  uint32_t *tail;

  simdjson_inline bit_indexer(uint32_t *index_buf) : tail(index_buf) {}

#if SIMDJSON_PREFER_REVERSE_BITS
  /**
    * ARM lacks a fast trailing zero instruction, but it has a fast
    * bit reversal instruction and a fast leading zero instruction.
    * Thus it may be profitable to reverse the bits (once) and then
    * to rely on a sequence of instructions that call the leading
    * zero instruction.
    *
    * Performance notes:
    * The chosen routine is not optimal in terms of data dependency
    * since zero_leading_bit might require two instructions. However,
    * it tends to minimize the total number of instructions which is
    * beneficial.
    */
  simdjson_inline void write_index(uint32_t idx, uint64_t& rev_bits, int i) {
    int lz = leading_zeroes(rev_bits);
    this->tail[i] = static_cast<uint32_t>(idx) + lz;
    rev_bits = zero_leading_bit(rev_bits, lz);
  }
#else
  /**
    * Under recent x64 systems, we often have both a fast trailing zero
    * instruction and a fast 'clear-lower-bit' instruction so the following
    * algorithm can be competitive.
    */

  simdjson_inline void write_index(uint32_t idx, uint64_t& bits, int i) {
    this->tail[i] = idx + trailing_zeroes(bits);
    bits = clear_lowest_bit(bits);
  }
#endif // SIMDJSON_PREFER_REVERSE_BITS

  template <int START, int N>
  simdjson_inline int write_indexes(uint32_t idx, uint64_t& bits) {
    write_index(idx, bits, START);
    SIMDJSON_IF_CONSTEXPR (N > 1) {
      write_indexes<(N-1>0?START+1:START), (N-1>=0?N-1:1)>(idx, bits);
    }
    return START+N;
  }

  template <int START, int END, int STEP>
  simdjson_inline int write_indexes_stepped(uint32_t idx, uint64_t& bits, int cnt) {
    write_indexes<START, STEP>(idx, bits);
    SIMDJSON_IF_CONSTEXPR ((START+STEP)  < END) {
      if (simdjson_unlikely((START+STEP) < cnt)) {
        write_indexes_stepped<(START+STEP<END?START+STEP:END), END, STEP>(idx, bits, cnt);
      }
    }
    return ((END-START) % STEP) == 0 ? END : (END-START) - ((END-START) % STEP) + STEP;
  }

  // flatten out values in 'bits' assuming that they are are to have values of idx
  // plus their position in the bitvector, and store these indexes at
  // base_ptr[base] incrementing base as we go
  // will potentially store extra values beyond end of valid bits, so base_ptr
  // needs to be large enough to handle this
  //
  // If the kernel sets SIMDJSON_GENERIC_JSON_STRUCTURAL_INDEXER_CUSTOM_BIT_INDEXER, then it
  // will provide its own version of the code.
#ifdef SIMDJSON_GENERIC_JSON_STRUCTURAL_INDEXER_CUSTOM_BIT_INDEXER
  simdjson_inline void write(uint32_t idx, uint64_t bits);
#else
  simdjson_inline void write(uint32_t idx, uint64_t bits) {
    // In some instances, the next branch is expensive because it is mispredicted.
    // Unfortunately, in other cases,
    // it helps tremendously.
    if (bits == 0)
        return;

    int cnt = static_cast<int>(count_ones(bits));

#if SIMDJSON_PREFER_REVERSE_BITS
    bits = reverse_bits(bits);
#endif
#ifdef SIMDJSON_STRUCTURAL_INDEXER_STEP
    static constexpr const int STEP = SIMDJSON_STRUCTURAL_INDEXER_STEP;
#else
    static constexpr const int STEP = 4;
#endif
    static constexpr const int STEP_UNTIL = 24;

    write_indexes_stepped<0, STEP_UNTIL, STEP>(idx, bits, cnt);
    SIMDJSON_IF_CONSTEXPR (STEP_UNTIL < 64) {
      if (simdjson_unlikely(STEP_UNTIL < cnt)) {
        for (int i=STEP_UNTIL; i<cnt; i++) {
          write_index(idx, bits, i);
        }
      }
    }

    this->tail += cnt;
  }
#endif // SIMDJSON_GENERIC_JSON_STRUCTURAL_INDEXER_CUSTOM_BIT_INDEXER

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
  static error_code index(const uint8_t *buf, size_t len, dom_parser_implementation &parser, stage1_mode partial) noexcept;

private:
  simdjson_inline json_structural_indexer(uint32_t *structural_indexes);
  template<size_t STEP_SIZE>
  simdjson_inline void step(const uint8_t *block, buf_block_reader<STEP_SIZE> &reader) noexcept;
  simdjson_inline void next(const simd::simd8x64<uint8_t>& in, const json_block& block, size_t idx);
  simdjson_inline error_code finish(dom_parser_implementation &parser, size_t idx, size_t len, stage1_mode partial);

  json_scanner scanner{};
  utf8_checker checker{};
  bit_indexer indexer;
  uint64_t prev_structurals = 0;
  uint64_t unescaped_chars_error = 0;
};

simdjson_inline json_structural_indexer::json_structural_indexer(uint32_t *structural_indexes) : indexer{structural_indexes} {}

// Skip the last character if it is partial
simdjson_inline size_t trim_partial_utf8(const uint8_t *buf, size_t len) {
  if (simdjson_unlikely(len < 3)) {
    switch (len) {
      case 2:
        if (buf[len-1] >= 0xc0) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
        if (buf[len-2] >= 0xe0) { return len-2; } // 3- and 4-byte characters with only 2 bytes left
        return len;
      case 1:
        if (buf[len-1] >= 0xc0) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
        return len;
      case 0:
        return len;
    }
  }
  if (buf[len-1] >= 0xc0) { return len-1; } // 2-, 3- and 4-byte characters with only 1 byte left
  if (buf[len-2] >= 0xe0) { return len-2; } // 3- and 4-byte characters with only 1 byte left
  if (buf[len-3] >= 0xf0) { return len-3; } // 4-byte characters with only 3 bytes left
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
error_code json_structural_indexer::index(const uint8_t *buf, size_t len, dom_parser_implementation &parser, stage1_mode partial) noexcept {
  if (simdjson_unlikely(len > parser.capacity())) { return CAPACITY; }
  // We guard the rest of the code so that we can assume that len > 0 throughout.
  if (len == 0) { return EMPTY; }
  if (is_streaming(partial)) {
    len = trim_partial_utf8(buf, len);
    // If you end up with an empty window after trimming
    // the partial UTF-8 bytes, then chances are good that you
    // have an UTF-8 formatting error.
    if(len == 0) { return UTF8_ERROR; }
  }
  buf_block_reader<STEP_SIZE> reader(buf, len);
  json_structural_indexer indexer(parser.structural_indexes.get());

  // Read all but the last block
  while (reader.has_full_block()) {
    indexer.step<STEP_SIZE>(reader.full_block(), reader);
  }
  // Take care of the last block (will always be there unless file is empty which is
  // not supposed to happen.)
  uint8_t block[STEP_SIZE];
  if (simdjson_unlikely(reader.get_remainder(block) == 0)) { return UNEXPECTED_ERROR; }
  indexer.step<STEP_SIZE>(block, reader);
  return indexer.finish(parser, reader.block_index(), len, partial);
}

template<>
simdjson_inline void json_structural_indexer::step<128>(const uint8_t *block, buf_block_reader<128> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block);
  simd::simd8x64<uint8_t> in_2(block+64);
  json_block block_1 = scanner.next(in_1);
  json_block block_2 = scanner.next(in_2);
  this->next(in_1, block_1, reader.block_index());
  this->next(in_2, block_2, reader.block_index()+64);
  reader.advance();
}

template<>
simdjson_inline void json_structural_indexer::step<64>(const uint8_t *block, buf_block_reader<64> &reader) noexcept {
  simd::simd8x64<uint8_t> in_1(block);
  json_block block_1 = scanner.next(in_1);
  this->next(in_1, block_1, reader.block_index());
  reader.advance();
}

simdjson_inline void json_structural_indexer::next(const simd::simd8x64<uint8_t>& in, const json_block& block, size_t idx) {
  uint64_t unescaped = in.lteq(0x1F);
#if SIMDJSON_UTF8VALIDATION
  checker.check_next_input(in);
#endif
  indexer.write(uint32_t(idx-64), prev_structurals); // Output *last* iteration's structurals to the parser
  prev_structurals = block.structural_start();
  unescaped_chars_error |= block.non_quote_inside_string(unescaped);
}

simdjson_inline error_code json_structural_indexer::finish(dom_parser_implementation &parser, size_t idx, size_t len, stage1_mode partial) {
  // Write out the final iteration's structurals
  indexer.write(uint32_t(idx-64), prev_structurals);
  error_code error = scanner.finish();
  // We deliberately break down the next expression so that it is
  // human readable.
  const bool should_we_exit = is_streaming(partial) ?
    ((error != SUCCESS) && (error != UNCLOSED_STRING)) // when partial we tolerate UNCLOSED_STRING
    : (error != SUCCESS); // if partial is false, we must have SUCCESS
  const bool have_unclosed_string = (error == UNCLOSED_STRING);
  if (simdjson_unlikely(should_we_exit)) { return error; }

  if (unescaped_chars_error) {
    return UNESCAPED_CHARS;
  }
  parser.n_structural_indexes = uint32_t(indexer.tail - parser.structural_indexes.get());
  /***
   * The On Demand API requires special padding.
   *
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
   *
   * This is illustrated with the test array_iterate_unclosed_error() on the following input:
   * R"({ "a": [,,)"
   **/
  parser.structural_indexes[parser.n_structural_indexes] = uint32_t(len); // used later in partial == stage1_mode::streaming_final
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
  if (partial == stage1_mode::streaming_partial) {
    // If we have an unclosed string, then the last structural
    // will be the quote and we want to make sure to omit it.
    if(have_unclosed_string) {
      parser.n_structural_indexes--;
      // a valid JSON file cannot have zero structural indexes - we should have found something
      if (simdjson_unlikely(parser.n_structural_indexes == 0u)) { return CAPACITY; }
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
  } else if (partial == stage1_mode::streaming_final) {
    if(have_unclosed_string) { parser.n_structural_indexes--; }
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
    if (simdjson_unlikely(parser.n_structural_indexes == 0u)) {
        // We tolerate an unclosed string at the very end of the stream. Indeed, users
        // often load their data in bulk without being careful and they want us to ignore
        // the trailing garbage.
        return EMPTY;
    }
  }
  checker.check_eof();
  return checker.errors();
}

} // namespace stage1
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

// Clear CUSTOM_BIT_INDEXER so other implementations can set it if they need to.
#undef SIMDJSON_GENERIC_JSON_STRUCTURAL_INDEXER_CUSTOM_BIT_INDEXER

#endif // SIMDJSON_SRC_GENERIC_STAGE1_JSON_STRUCTURAL_INDEXER_H
