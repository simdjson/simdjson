// This file contains the common code every implementation uses in stage1
// It is intended to be included multiple times and compiled multiple times
// We assume the file in which it is included already includes
// "simdjson/stage1_find_marks.h" (this simplifies amalgation)

namespace stage1 {

class json_structural_scanner {
public:
  // Whether the first character of the next iteration is escaped.
  uint64_t prev_escaped = 0ULL;
  // Whether the last iteration was still inside a string (all 1's = true, all 0's = false).
  uint64_t prev_in_string = 0ULL;
  // Whether the last character of the previous iteration is a primitive value character
  // (anything except whitespace, braces, comma or colon).
  uint64_t prev_primitive = 0ULL;
  // Errors with unescaped characters in strings (ASCII codepoints < 0x20)
  uint64_t unescaped_chars_error = 0;
  uint64_t *next_structural_block;

  json_structural_scanner(uint64_t *_structural_blocks) : next_structural_block{_structural_blocks} {}

  //
  // Finish the scan and return any errors.
  //
  // This may detect errors as well, such as unclosed string and certain UTF-8 errors.
  //
  really_inline ErrorValues at_eof(ParsedJson& pj);

  //
  // Return a mask of all string characters plus end quotes.
  //
  // prev_escaped is overflow saying whether the next character is escaped.
  // prev_in_string is overflow saying whether we're still in a string.
  //
  // Backslash sequences outside of quotes will be detected in stage 2.
  //
  really_inline uint64_t find_strings(const simd::simd8x64<uint8_t> in);

  //
  // Determine which characters are *structural*:
  // - braces: [] and {}
  // - the start of primitives (123, true, false, null)
  // - the start of invalid non-whitespace (+, &, ture, UTF-8)
  //
  // Also detects value sequence errors:
  // - two values with no separator between ("hello" "world")
  // - separators with no values ([1,] [1,,]and [,2])
  //
  // This method will find all of the above whether it is in a string or not.
  //
  // To reduce dependency on the expensive "what is in a string" computation, this method treats the
  // contents of a string the same as content outside. Errors and structurals inside the string or on
  // the trailing quote will need to be removed later when the correct string information is known.
  //
  really_inline uint64_t find_potential_structurals(const simd::simd8x64<uint8_t> in);

  really_inline void write_structurals(uint64_t structurals);

  //
  // Find the important bits of JSON in a STEP_SIZE-byte chunk, and add them to structural_indexes.
  //
  template<size_t STEP_SIZE>
  really_inline void scan_step(const uint8_t *buf, utf8_checker &utf8_checker);

  //
  // Parse the entire input in STEP_SIZE-byte chunks.
  //
  template<size_t STEP_SIZE>
  really_inline void scan(const uint8_t *buf, const size_t len, utf8_checker &utf8_checker);
};

// return a bitvector indicating where we have characters that end an odd-length
// sequence of backslashes (and thus change the behavior of the next character
// to follow). A even-length sequence of backslashes, and, for that matter, the
// largest even-length prefix of our odd-length sequence of backslashes, simply
// modify the behavior of the backslashes themselves.
// We also update the prev_iter_ends_odd_backslash reference parameter to
// indicate whether we end an iteration on an odd-length sequence of
// backslashes, which modifies our subsequent search for odd-length
// sequences of backslashes in an obvious way.
really_inline uint64_t follows_odd_sequence_of(const uint64_t match, uint64_t &overflow) {
  const uint64_t even_bits = 0x5555555555555555ULL;
  const uint64_t odd_bits = ~even_bits;
  uint64_t start_edges = match & ~(match << 1);
  /* flip lowest if we have an odd-length run at the end of the prior
  * iteration */
  uint64_t even_start_mask = even_bits ^ overflow;
  uint64_t even_starts = start_edges & even_start_mask;
  uint64_t odd_starts = start_edges & ~even_start_mask;
  uint64_t even_carries = match + even_starts;

  uint64_t odd_carries;
  /* must record the carry-out of our odd-carries out of bit 63; this
  * indicates whether the sense of any edge going to the next iteration
  * should be flipped */
  bool new_overflow = add_overflow(match, odd_starts, &odd_carries);

  odd_carries |= overflow; /* push in bit zero as a
                              * potential end if we had an
                              * odd-numbered run at the
                              * end of the previous
                              * iteration */
  overflow = new_overflow ? 0x1ULL : 0x0ULL;
  uint64_t even_carry_ends = even_carries & ~match;
  uint64_t odd_carry_ends = odd_carries & ~match;
  uint64_t even_start_odd_end = even_carry_ends & odd_bits;
  uint64_t odd_start_even_end = odd_carry_ends & even_bits;
  uint64_t odd_ends = even_start_odd_end | odd_start_even_end;
  return odd_ends;
}

//
// Check if the current character immediately follows a matching character.
//
// For example, this checks for quotes with backslashes in front of them:
//
//     const uint64_t backslashed_quote = in.eq('"') & immediately_follows(in.eq('\'), prev_backslash);
//
really_inline uint64_t follows(const uint64_t match, uint64_t &overflow) {
  const uint64_t result = match << 1 | overflow;
  overflow = match >> 63;
  return result;
}

//
// Check if the current character follows a matching character, with possible "filler" between.
// For example, this checks for empty curly braces, e.g. 
//
//     in.eq('}') & follows(in.eq('['), in.eq(' '), prev_empty_array) // { <whitespace>* }
//
really_inline uint64_t follows(const uint64_t match, const uint64_t filler, uint64_t &overflow) {
  uint64_t follows_match = follows(match, overflow);
  uint64_t result;
  overflow |= add_overflow(follows_match, filler, &result);
  return result;
}

really_inline ErrorValues json_structural_scanner::at_eof(ParsedJson& pj) {
  // Cap off structural_blocks with a 0 so we can safely go off the end and only pay
  // a branch penalty on the very last block
  *this->next_structural_block = 0;
  pj.num_structural_blocks = this->next_structural_block - pj.structural_blocks;
  if (prev_in_string) {
    return UNCLOSED_STRING;
  }
  if (unescaped_chars_error) {
    return UNESCAPED_CHARS;
  }
  if (pj.num_structural_blocks == 0) {
    return EMPTY;
  }
  return SUCCESS;
}

//
// Return a mask of all string characters plus end quotes.
//
// prev_escaped is overflow saying whether the next character is escaped.
// prev_in_string is overflow saying whether we're still in a string.
//
// Backslash sequences outside of quotes will be detected in stage 2.
//
really_inline uint64_t json_structural_scanner::find_strings(const simd::simd8x64<uint8_t> in) {
  const uint64_t backslash = in.eq('\\');
  const uint64_t escaped = follows_odd_sequence_of(backslash, prev_escaped);
  const uint64_t quote = in.eq('"') & ~escaped;
  // prefix_xor flips on bits inside the string (and flips off the end quote).
  const uint64_t in_string = prefix_xor(quote) ^ prev_in_string;
  /* right shift of a signed value expected to be well-defined and standard
  * compliant as of C++20,
  * John Regher from Utah U. says this is fine code */
  prev_in_string = static_cast<uint64_t>(static_cast<int64_t>(in_string) >> 63);
  // Use ^ to turn the beginning quote off, and the end quote on.
  return in_string ^ quote;
}

//
// Determine which characters are *structural*:
// - braces: [] and {}
// - the start of primitives (123, true, false, null)
// - the start of invalid non-whitespace (+, &, ture, UTF-8)
//
// Also detects value sequence errors:
// - two values with no separator between ("hello" "world")
// - separators with no values ([1,] [1,,]and [,2])
//
// This method will find all of the above whether it is in a string or not.
//
// To reduce dependency on the expensive "what is in a string" computation, this method treats the
// contents of a string the same as content outside. Errors and structurals inside the string or on
// the trailing quote will need to be removed later when the correct string information is known.
//
really_inline uint64_t json_structural_scanner::find_potential_structurals(const simd::simd8x64<uint8_t> in) {
  // These use SIMD so let's kick them off before running the regular 64-bit stuff ...
  uint64_t whitespace, op;
  find_whitespace_and_operators(in, whitespace, op);

  // Detect the start of a run of primitive characters. Includes numbers, booleans, and strings (").
  // Everything except whitespace, braces, colon and comma.
  const uint64_t primitive = ~(op | whitespace);
  const uint64_t follows_primitive = follows(primitive, prev_primitive);
  const uint64_t start_primitive = primitive & ~follows_primitive;

  // Return final structurals
  return op | start_primitive;
}

really_inline void json_structural_scanner::write_structurals(uint64_t structurals) {
  *this->next_structural_block = structurals;
  this->next_structural_block += (structurals != 0);
}

void print_input(simd8x64<uint8_t> in) {
  char buf[64];
  in.store((uint8_t*)buf);
  for (int n=0;n<64;n++) {
    switch (buf[n]) {
      case '\n':
      case '\t':
        printf(" ");
        break;
      default:
        printf("%c", buf[n]);
    }
  }
  printf("\n");
}
void print_mask(uint64_t mask) {
  for (int n=0;n<64;n++) {
    if (mask & (uint64_t(1)<<n)) {
      printf("X");
    } else {
      printf(" ");
    }
  }
  printf("\n");
}

//
// Find the important bits of JSON in a 128-byte chunk, and add them to structural_indexes.
//
// PERF NOTES:
// We pipe 2 inputs through these stages:
// 1. Load JSON into registers. This takes a long time and is highly parallelizable, so we load
//    2 inputs' worth at once so that by the time step 2 is looking for them input, it's available.
// 2. Scan the JSON for critical data: strings, primitives and operators. This is the critical path.
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
template<>
really_inline void json_structural_scanner::scan_step<128>(const uint8_t *buf, utf8_checker &utf8_checker) {
  //
  // Load up all 128 bytes into SIMD registers
  //
  simd::simd8x64<uint8_t> in_1(buf);
  simd::simd8x64<uint8_t> in_2(buf+64);

  //
  // Find the strings and potential structurals (operators / primitives).
  //
  // This will include false structurals that are *inside* strings--we'll filter strings out
  // before we return.
  //
  uint64_t string_1 = this->find_strings(in_1);
  uint64_t structurals_1 = this->find_potential_structurals(in_1);
  uint64_t string_2 = this->find_strings(in_2);
  uint64_t structurals_2 = this->find_potential_structurals(in_2);

  //
  // Do miscellaneous work while the processor is busy calculating strings and structurals.
  //
  // After that, weed out structurals that are inside strings and find invalid string characters.
  //
  uint64_t unescaped_1 = in_1.lteq(0x1F);
  utf8_checker.check_next_input(in_1);
  // print_input(in_1);
  // print_mask(structurals_1 & ~string_1);
  this->write_structurals(structurals_1 & ~string_1);
  this->unescaped_chars_error |= unescaped_1 & string_1;

  uint64_t unescaped_2 = in_2.lteq(0x1F);
  utf8_checker.check_next_input(in_2);
  // print_input(in_2);
  // print_mask(structurals_2 & ~string_2);
  this->write_structurals(structurals_2 & ~string_2);
  this->unescaped_chars_error |= unescaped_2 & string_2;
}

//
// Find the important bits of JSON in a 64-byte chunk, and add them to structural_indexes.
//
template<>
really_inline void json_structural_scanner::scan_step<64>(const uint8_t *buf, utf8_checker &utf8_checker) {
  //
  // Load up bytes into SIMD registers
  //
  simd::simd8x64<uint8_t> in_1(buf);

  //
  // Find the strings and potential structurals (operators / primitives).
  //
  // This will include false structurals that are *inside* strings--we'll filter strings out
  // before we return.
  //
  uint64_t string_1 = this->find_strings(in_1);
  uint64_t structurals_1 = this->find_potential_structurals(in_1);

  //
  // Do miscellaneous work while the processor is busy calculating strings and structurals.
  //
  // After that, weed out structurals that are inside strings and find invalid string characters.
  //
  uint64_t unescaped_1 = in_1.lteq(0x1F);
  utf8_checker.check_next_input(in_1);
  this->write_structurals(structurals_1 & ~string_1);
  this->unescaped_chars_error |= unescaped_1 & string_1;
}

template<size_t STEP_SIZE>
really_inline void json_structural_scanner::scan(const uint8_t *buf, const size_t len, utf8_checker &utf8_checker) {
  size_t lenminusstep = len < STEP_SIZE ? 0 : len - STEP_SIZE;
  size_t idx = 0;

  for (; idx < lenminusstep; idx += STEP_SIZE) {
    this->scan_step<STEP_SIZE>(&buf[idx], utf8_checker);
  }

  /* If we have a final chunk of less than 64 bytes, pad it to 64 with
  * spaces  before processing it (otherwise, we risk invalidating the UTF-8
  * checks). */
  if (likely(idx < len)) {
    uint8_t tmp_buf[STEP_SIZE];
    memset(tmp_buf, 0x20, STEP_SIZE);
    memcpy(tmp_buf, buf + idx, len - idx);
    this->scan_step<STEP_SIZE>(&tmp_buf[0], utf8_checker);
    idx += STEP_SIZE;
  }
}

template<size_t STEP_SIZE>
int find_structural_bits(const uint8_t *buf, size_t len, simdjson::ParsedJson &pj, bool streaming) {
  // printf("\n");
  if (unlikely(len > pj.byte_capacity)) {
    std::cerr << "Your ParsedJson object only supports documents up to "
              << pj.byte_capacity << " bytes but you are trying to process "
              << len << " bytes" << std::endl;
    return CAPACITY;
  }
  utf8_checker utf8_checker{};
  json_structural_scanner scanner(pj.structural_blocks);
  scanner.scan<STEP_SIZE>(buf, len, utf8_checker);

  ErrorValues error = scanner.at_eof(pj);
  if (!streaming && unlikely(error != SUCCESS)) {
    return error;
  }

  // if (unlikely(uint64_t(scanner.out_structurals - pj.structurals) != (ROUNDUP_N(len, 64)/64))) {
  //   return simdjson::UNEXPECTED_ERROR;
  // }
  return utf8_checker.errors();
}

} // namespace stage1