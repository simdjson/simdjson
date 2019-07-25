#include "simdjson/stage1_find_marks.h"

namespace simdjson {
// We need to compile that code for multiple architectures. However, target attributes can be used
// only once by function definition. Huge macro seemed better than huge code duplication.
// FIND_STRUCTURAL_BITS(architecture T, const uint8_t *buf, size_t len, ParsedJson &pj)
#define FIND_STRUCTURAL_BITS(T, buf, len, pj) {                                                     \
  if (len > pj.bytecapacity) {                                                                      \
    std::cerr << "Your ParsedJson object only supports documents up to "                            \
         << pj.bytecapacity << " bytes but you are trying to process " << len                       \
         << " bytes" << std::endl;                                                                  \
    return simdjson::CAPACITY;                                                                      \
  }                                                                                                 \
  uint32_t *base_ptr = pj.structural_indexes;                                                       \
  uint32_t base = 0;                                                                                \
  utf8_checking_state<T> state;                                                                     \
                                                                                                    \
  /* we have padded the input out to 64 byte multiple with the remainder being                   */ \
  /* zeros                                                                                       */ \
                                                                                                    \
  /* persistent state across loop                                                                */ \
  /* does the last iteration end with an odd-length sequence of backslashes?                     */ \
  /* either 0 or 1, but a 64-bit value                                                           */ \
  uint64_t prev_iter_ends_odd_backslash = 0ULL;                                                     \
  /* does the previous iteration end inside a double-quote pair?                                 */ \
  uint64_t prev_iter_inside_quote = 0ULL;  /* either all zeros or all ones                       */ \
  /* does the previous iteration end on something that is a predecessor of a                     */ \
  /* pseudo-structural character - i.e. whitespace or a structural character                     */ \
  /* effectively the very first char is considered to follow "whitespace" for                    */ \
  /* the                                                                                         */ \
  /* purposes of pseudo-structural character detection so we initialize to 1                     */ \
  uint64_t prev_iter_ends_pseudo_pred = 1ULL;                                                       \
                                                                                                    \
  /* structurals are persistent state across loop as we flatten them on the                      */ \
  /* subsequent iteration into our array pointed to be base_ptr.                                 */ \
  /* This is harmless on the first iteration as structurals==0                                   */ \
  /* and is done for performance reasons; we can hide some of the latency of the                 */ \
  /* expensive carryless multiply in the previous step with this work                            */ \
  uint64_t structurals = 0;                                                                         \
                                                                                                    \
  size_t lenminus64 = len < 64 ? 0 : len - 64;                                                      \
  size_t idx = 0;                                                                                   \
  uint64_t error_mask = 0; /* for unescaped characters within strings (ASCII code points < 0x20) */ \
                                                                                                    \
  for (; idx < lenminus64; idx += 64) {                                                             \
                                                                                                    \
    simd_input<T> in = fill_input<T>(buf+idx);                                                      \
    check_utf8<T>(in, state);                                                                       \
    /* detect odd sequences of backslashes                                                       */ \
    uint64_t odd_ends = find_odd_backslash_sequences<T>(                                            \
        in, prev_iter_ends_odd_backslash);                                                          \
                                                                                                    \
    /* detect insides of quote pairs ("quote_mask") and also our quote_bits                      */ \
    /* themselves                                                                                */ \
    uint64_t quote_bits;                                                                            \
    uint64_t quote_mask = find_quote_mask_and_bits<T>(                                              \
        in, odd_ends, prev_iter_inside_quote, quote_bits, error_mask);                              \
                                                                                                    \
    /* take the previous iterations structural bits, not our current iteration,                  */ \
    /* and flatten                                                                               */ \
    flatten_bits(base_ptr, base, idx, structurals);                                                 \
                                                                                                    \
    uint64_t whitespace;                                                                            \
    find_whitespace_and_structurals<T>(in, whitespace, structurals);                                \
                                                                                                    \
    /* fixup structurals to reflect quotes and add pseudo-structural characters                  */ \
    structurals = finalize_structurals(structurals, whitespace, quote_mask,                         \
                                       quote_bits, prev_iter_ends_pseudo_pred);                     \
  }                                                                                                 \
                                                                                                    \
  /*//////////////                                                                               */ \
  /*/ we use a giant copy-paste which is ugly.                                                   */ \
  /*/ but otherwise the string needs to be properly padded or else we                            */ \
  /*/ risk invalidating the UTF-8 checks.                                                        */ \
  /*//////////                                                                                   */ \
  if (idx < len) {                                                                                  \
    uint8_t tmpbuf[64];                                                                             \
    memset(tmpbuf, 0x20, 64);                                                                       \
    memcpy(tmpbuf, buf + idx, len - idx);                                                           \
    simd_input<T> in = fill_input<T>(tmpbuf);                                                       \
    check_utf8<T>(in, state);                                                                       \
                                                                                                    \
    /* detect odd sequences of backslashes                                                       */ \
    uint64_t odd_ends = find_odd_backslash_sequences<T>(                                            \
        in, prev_iter_ends_odd_backslash);                                                          \
                                                                                                    \
    /* detect insides of quote pairs ("quote_mask") and also our quote_bits                      */ \
    /* themselves                                                                                */ \
    uint64_t quote_bits;                                                                            \
    uint64_t quote_mask = find_quote_mask_and_bits<T>(                                              \
        in, odd_ends, prev_iter_inside_quote, quote_bits, error_mask);                              \
                                                                                                    \
    /* take the previous iterations structural bits, not our current iteration,                  */ \
    /* and flatten                                                                               */ \
    flatten_bits(base_ptr, base, idx, structurals);                                                 \
                                                                                                    \
    uint64_t whitespace;                                                                            \
    find_whitespace_and_structurals<T>(in, whitespace, structurals);                                \
                                                                                                    \
    /* fixup structurals to reflect quotes and add pseudo-structural characters                  */ \
    structurals = finalize_structurals(structurals, whitespace, quote_mask,                         \
                                       quote_bits, prev_iter_ends_pseudo_pred);                     \
    idx += 64;                                                                                      \
  }                                                                                                 \
                                                                                                    \
  /* is last string quote closed?                                                                */ \
  if (prev_iter_inside_quote) {                                                                     \
      return simdjson::UNCLOSED_STRING;                                                             \
  }                                                                                                 \
                                                                                                    \
  /* finally, flatten out the remaining structurals from the last iteration                      */ \
  flatten_bits(base_ptr, base, idx, structurals);                                                   \
                                                                                                    \
  pj.n_structural_indexes = base;                                                                   \
  /* a valid JSON file cannot have zero structural indexes - we should have                      */ \
  /* found something                                                                             */ \
  if (pj.n_structural_indexes == 0u) {                                                              \
    return simdjson::EMPTY;                                                                         \
  }                                                                                                 \
  if (base_ptr[pj.n_structural_indexes - 1] > len) {                                                \
    return simdjson::UNEXPECTED_ERROR;                                                              \
  }                                                                                                 \
  if (len != base_ptr[pj.n_structural_indexes - 1]) {                                               \
    /* the string might not be NULL terminated, but we add a virtual NULL ending                 */ \
    /* character.                                                                                */ \
    base_ptr[pj.n_structural_indexes++] = len;                                                      \
  }                                                                                                 \
  /* make it safe to dereference one beyond this array                                           */ \
  base_ptr[pj.n_structural_indexes] = 0;                                                            \
  if (error_mask) {                                                                                 \
    return simdjson::UNESCAPED_CHARS;                                                               \
  }                                                                                                 \
  return check_utf8_errors<T>(state);                                                               \
}                                                                                                   \



#ifdef IS_X86_64
TARGET_HASWELL();
template<>
int find_structural_bits<architecture::haswell>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  FIND_STRUCTURAL_BITS(architecture::haswell, buf, len, pj);
}
UNTARGET_REGION();

TARGET_WESTMERE();
template<>
int find_structural_bits<architecture::westmere>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  FIND_STRUCTURAL_BITS(architecture::westmere, buf, len, pj);
}
UNTARGET_REGION();
#endif

#ifdef IS_ARM64
template<>
int find_structural_bits<architecture::arm64>(const uint8_t *buf, size_t len, ParsedJson &pj) {
  FIND_STRUCTURAL_BITS(architecture::arm64, buf, len, pj);
}
#endif
//#undef FIND_STRUCTURAL_BITS
}
