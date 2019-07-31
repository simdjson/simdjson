#ifndef SIMDJSON_STAGE1_FIND_MARKS_H
#define SIMDJSON_STAGE1_FIND_MARKS_H

#include "simdjson/common_defs.h"
#include "simdjson/parsedjson.h"
#include "simdjson/portability.h"
#include "simdjson/simdjson.h"
#include <cassert>

namespace simdjson {

template <Architecture> struct simd_input;

template <Architecture> uint64_t compute_quote_mask(uint64_t quote_bits);

namespace {
// for when clmul is unavailable
[[maybe_unused]] uint64_t portable_compute_quote_mask(uint64_t quote_bits) {
  uint64_t quote_mask = quote_bits ^ (quote_bits << 1);
  quote_mask = quote_mask ^ (quote_mask << 2);
  quote_mask = quote_mask ^ (quote_mask << 4);
  quote_mask = quote_mask ^ (quote_mask << 8);
  quote_mask = quote_mask ^ (quote_mask << 16);
  quote_mask = quote_mask ^ (quote_mask << 32);
  return quote_mask;
}
} // namespace

// Holds the state required to perform check_utf8().
template <Architecture> struct utf8_checking_state;

template <Architecture T>
void check_utf8(simd_input<T> in, utf8_checking_state<T> &state);

// Checks if the utf8 validation has found any error.
template <Architecture T>
ErrorValues check_utf8_errors(utf8_checking_state<T> &state);

// a straightforward comparison of a mask against input.
template <Architecture T>
uint64_t cmp_mask_against_input(simd_input<T> in, uint8_t m);

template <Architecture T> simd_input<T> fill_input(const uint8_t *ptr);

// find all values less than or equal than the content of maxval (using unsigned
// arithmetic)
template <Architecture T>
uint64_t unsigned_lteq_against_input(simd_input<T> in, uint8_t m);

template <Architecture T>
really_inline uint64_t find_odd_backslash_sequences(
    simd_input<T> in, uint64_t &prev_iter_ends_odd_backslash);

template <Architecture T>
really_inline uint64_t find_quote_mask_and_bits(
    simd_input<T> in, uint64_t odd_ends, uint64_t &prev_iter_inside_quote,
    uint64_t &quote_bits, uint64_t &error_mask);

// do a 'shufti' to detect structural JSON characters
// they are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c
// these go into the first 3 buckets of the comparison (1/2/4)

// we are also interested in the four whitespace characters
// space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d
// these go into the next 2 buckets of the comparison (8/16)
template <Architecture T>
void find_whitespace_and_structurals(simd_input<T> in, uint64_t &whitespace,
                                     uint64_t &structurals);

// return a updated structural bit vector with quoted contents cleared out and
// pseudo-structural characters added to the mask
// updates prev_iter_ends_pseudo_pred which tells us whether the previous
// iteration ended on a whitespace or a structural character (which means that
// the next iteration
// will have a pseudo-structural character at its start)
really_inline uint64_t finalize_structurals(
    uint64_t structurals, uint64_t whitespace, uint64_t quote_mask,
    uint64_t quote_bits, uint64_t &prev_iter_ends_pseudo_pred) {
  // mask off anything inside quotes
  structurals &= ~quote_mask;
  // add the real quote bits back into our bit_mask as well, so we can
  // quickly traverse the strings we've spent all this trouble gathering
  structurals |= quote_bits;
  // Now, establish "pseudo-structural characters". These are non-whitespace
  // characters that are (a) outside quotes and (b) have a predecessor that's
  // either whitespace or a structural character. This means that subsequent
  // passes will get a chance to encounter the first character of every string
  // of non-whitespace and, if we're parsing an atom like true/false/null or a
  // number we can stop at the first whitespace or structural character
  // following it.

  // a qualified predecessor is something that can happen 1 position before an
  // pseudo-structural character
  uint64_t pseudo_pred = structurals | whitespace;

  uint64_t shifted_pseudo_pred =
      (pseudo_pred << 1) | prev_iter_ends_pseudo_pred;
  prev_iter_ends_pseudo_pred = pseudo_pred >> 63;
  uint64_t pseudo_structurals =
      shifted_pseudo_pred & (~whitespace) & (~quote_mask);
  structurals |= pseudo_structurals;

  // now, we've used our close quotes all we need to. So let's switch them off
  // they will be off in the quote mask and on in quote bits.
  structurals &= ~(quote_bits & ~quote_mask);
  return structurals;
}

template <Architecture T = Architecture::NATIVE>
int find_structural_bits(const uint8_t *buf, size_t len,
                         simdjson::ParsedJson &pj);

template <Architecture T = Architecture::NATIVE>
int find_structural_bits(const char *buf, size_t len,
                         simdjson::ParsedJson &pj) {
  return find_structural_bits((const uint8_t *)buf, len, pj);
}

} // namespace simdjson

#endif
