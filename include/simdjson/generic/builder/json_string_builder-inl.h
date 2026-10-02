#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>
#ifndef SIMDJSON_GENERIC_STRING_BUILDER_INL_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_GENERIC_STRING_BUILDER_INL_H
#include "simdjson/generic/builder/json_string_builder.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

/*
 * Empirically, we have found that an inlined optimization is important for
 * performance. The following macros are not ideal. We should find a better
 * way to inline the code.
 */

#if defined(__SSE2__) || defined(__x86_64__) || defined(__x86_64) ||           \
    (defined(_M_AMD64) || defined(_M_X64) ||                                   \
     (defined(_M_IX86_FP) && _M_IX86_FP == 2))
#ifndef SIMDJSON_EXPERIMENTAL_HAS_SSE2
#define SIMDJSON_EXPERIMENTAL_HAS_SSE2 1
#endif
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#ifndef SIMDJSON_EXPERIMENTAL_HAS_NEON
#define SIMDJSON_EXPERIMENTAL_HAS_NEON 1
#endif
#endif
#if defined(__loongarch_sx)
#ifndef SIMDJSON_EXPERIMENTAL_HAS_LSX
#define SIMDJSON_EXPERIMENTAL_HAS_LSX 1
#endif
#endif
#if defined(__loongarch_asx)
#ifndef SIMDJSON_EXPERIMENTAL_HAS_LASX
#define SIMDJSON_EXPERIMENTAL_HAS_LASX 1
#endif
#endif
#if defined(__riscv_v_intrinsic) && __riscv_v_intrinsic >= 11000 &&            \
    defined(__riscv_vector)
#ifndef SIMDJSON_EXPERIMENTAL_HAS_RVV
#define SIMDJSON_EXPERIMENTAL_HAS_RVV 1
#endif
#endif
#if (defined(__PPC64__) || defined(_M_PPC64)) && defined(__ALTIVEC__) && defined(__POWER8_VECTOR__)
#ifndef SIMDJSON_EXPERIMENTAL_HAS_PPC64
#define SIMDJSON_EXPERIMENTAL_HAS_PPC64 1
#endif
#endif
#if SIMDJSON_EXPERIMENTAL_HAS_NEON
#include <arm_neon.h>
#ifdef _MSC_VER
#include <intrin.h>
#endif
#endif
#if SIMDJSON_EXPERIMENTAL_HAS_SSE2
#include <emmintrin.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif
#ifdef _MSC_VER
#include <intrin.h>
#endif
#endif
#if SIMDJSON_EXPERIMENTAL_HAS_LSX
#include <lsxintrin.h>
#endif
#if SIMDJSON_EXPERIMENTAL_HAS_LASX
#include <lasxintrin.h>
#endif
#if SIMDJSON_EXPERIMENTAL_HAS_RVV
#include <riscv_vector.h>
#endif
#if SIMDJSON_EXPERIMENTAL_HAS_PPC64
#include <altivec.h>
#ifdef bool
#undef bool
#endif
#ifdef vector
#undef vector
#endif
#endif


namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace builder {

static SIMDJSON_CONSTEXPR_LAMBDA std::array<uint8_t, 256>
    json_quotable_character = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/**

A possible SWAR implementation of has_json_escapable_byte. It is not used
because it is slower than the current implementation. It is kept here for
reference (to show that we tried it).

inline bool has_json_escapable_byte(uint64_t x) {
  uint64_t is_ascii = 0x8080808080808080ULL & ~x;
  uint64_t xor2 = x ^ 0x0202020202020202ULL;
  uint64_t lt32_or_eq34 = xor2 - 0x2121212121212121ULL;
  uint64_t sub92 = x ^ 0x5C5C5C5C5C5C5C5CULL;
  uint64_t eq92 = (sub92 - 0x0101010101010101ULL);
  return ((lt32_or_eq34 | eq92) & is_ascii) != 0;
}

**/

// Scalar fallback for finding next quotable character
SIMDJSON_CONSTEXPR_LAMBDA simdjson_inline size_t
find_next_json_quotable_character_scalar(const std::string_view view,
                                         size_t location) noexcept {
  for (auto pos = view.begin() + location; pos != view.end(); ++pos) {
    if (json_quotable_character[static_cast<uint8_t>(*pos)]) {
      return pos - view.begin();
    }
  }
  return size_t(view.size());
}

// SIMD-accelerated position finding that directly locates the first quotable
// character, combining detection and position extraction in a single pass to
// minimize redundant work.
#if SIMDJSON_EXPERIMENTAL_HAS_NEON
simdjson_inline size_t
find_next_json_quotable_character(const std::string_view view,
                                  size_t location) noexcept {
  const size_t len = view.size();
  const uint8_t *ptr =
      reinterpret_cast<const uint8_t *>(view.data()) + location;
  size_t remaining = len - location;

  // SIMD constants for characters requiring escape
  uint8x16_t v34 = vdupq_n_u8(34);  // '"'
  uint8x16_t v92 = vdupq_n_u8(92);  // '\\'
  uint8x16_t v32 = vdupq_n_u8(32);  // control char threshold

  while (remaining >= 16) {
    uint8x16_t word = vld1q_u8(ptr);

    // Check for quotable characters: '"', '\\', or control chars (< 32)
    uint8x16_t needs_escape = vceqq_u8(word, v34);
    needs_escape = vorrq_u8(needs_escape, vceqq_u8(word, v92));
    needs_escape = vorrq_u8(needs_escape, vcltq_u8(word, v32));

    const uint8x8_t res = vshrn_n_u16(vreinterpretq_u16_u8(needs_escape), 4);
    const uint64_t mask = vget_lane_u64(vreinterpret_u64_u8(res), 0);
    if(mask != 0) {
      size_t offset = ptr - reinterpret_cast<const uint8_t *>(view.data());
      auto trailing_zero = trailing_zeroes(mask);
      return offset + (trailing_zero >> 2);
    }
    ptr += 16;
    remaining -= 16;
  }

  // Scalar fallback for remaining bytes
  size_t current = len - remaining;
  return find_next_json_quotable_character_scalar(view, current);
}
#elif SIMDJSON_EXPERIMENTAL_HAS_SSE2
simdjson_inline size_t
find_next_json_quotable_character(const std::string_view view,
                                  size_t location) noexcept {
  const size_t len = view.size();
  const uint8_t *ptr =
      reinterpret_cast<const uint8_t *>(view.data()) + location;
  size_t remaining = len - location;

  // SIMD constants
  __m128i v34 = _mm_set1_epi8(34);  // '"'
  __m128i v92 = _mm_set1_epi8(92);  // '\\'
  __m128i v31 = _mm_set1_epi8(31);  // for control char detection

  while (remaining >= 16) {
    __m128i word = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr));

    // Check for quotable characters
    __m128i needs_escape = _mm_cmpeq_epi8(word, v34);
    needs_escape = _mm_or_si128(needs_escape, _mm_cmpeq_epi8(word, v92));
    needs_escape = _mm_or_si128(
        needs_escape,
        _mm_cmpeq_epi8(_mm_subs_epu8(word, v31), _mm_setzero_si128()));

    int mask = _mm_movemask_epi8(needs_escape);
    if (mask != 0) {
      // Found quotable character - use trailing zero count to find position
      size_t offset = ptr - reinterpret_cast<const uint8_t *>(view.data());
      return offset + trailing_zeroes(mask);
    }
    ptr += 16;
    remaining -= 16;
  }

  // Scalar fallback for remaining bytes
  size_t current = len - remaining;
  return find_next_json_quotable_character_scalar(view, current);
}
#elif SIMDJSON_EXPERIMENTAL_HAS_LASX
simdjson_inline size_t
find_next_json_quotable_character(const std::string_view view,
                                  size_t location) noexcept {
  const size_t len = view.size();
  const uint8_t *ptr =
      reinterpret_cast<const uint8_t *>(view.data()) + location;
  size_t remaining = len - location;

  // SIMD constants for characters requiring escape
  __m256i v34 = __lasx_xvreplgr2vr_b(34);  // '"'
  __m256i v92 = __lasx_xvreplgr2vr_b(92);  // '\\'
  __m256i v32 = __lasx_xvreplgr2vr_b(32);  // control char threshold

  while (remaining >= 32) {
    __m256i word = __lasx_xvld(ptr, 0);

    // Check for quotable characters: '"', '\\', or control chars (< 32)
    __m256i needs_escape = __lasx_xvseq_b(word, v34);
    needs_escape = __lasx_xvor_v(needs_escape, __lasx_xvseq_b(word, v92));
    needs_escape = __lasx_xvor_v(needs_escape, __lasx_xvslt_bu(word, v32));

    if (!__lasx_xbz_v(needs_escape)) {
      // Found a quotable character - locate it via the four 64-bit lanes
      uint64_t lane0 = __lasx_xvpickve2gr_du(needs_escape, 0);
      uint64_t lane1 = __lasx_xvpickve2gr_du(needs_escape, 1);
      uint64_t lane2 = __lasx_xvpickve2gr_du(needs_escape, 2);
      uint64_t lane3 = __lasx_xvpickve2gr_du(needs_escape, 3);
      size_t offset = ptr - reinterpret_cast<const uint8_t *>(view.data());
      if (lane0 != 0) {
        return offset + trailing_zeroes(lane0) / 8;
      } else if (lane1 != 0) {
        return offset + 8 + trailing_zeroes(lane1) / 8;
      } else if (lane2 != 0) {
        return offset + 16 + trailing_zeroes(lane2) / 8;
      } else {
        return offset + 24 + trailing_zeroes(lane3) / 8;
      }
    }
    ptr += 32;
    remaining -= 32;
  }
  size_t current = len - remaining;
  return find_next_json_quotable_character_scalar(view, current);
}
#elif SIMDJSON_EXPERIMENTAL_HAS_LSX
simdjson_inline size_t
find_next_json_quotable_character(const std::string_view view,
                                  size_t location) noexcept {
  const size_t len = view.size();
  const uint8_t *ptr =
      reinterpret_cast<const uint8_t *>(view.data()) + location;
  size_t remaining = len - location;

  //SIMD constants for characters requiring escape
  __m128i v34 = __lsx_vreplgr2vr_b(34);  // '"'
  __m128i v92 = __lsx_vreplgr2vr_b(92);  // '\\'
  __m128i v32 = __lsx_vreplgr2vr_b(32);  // control char threshold

  while (remaining >= 16){
    __m128i word = __lsx_vld(ptr, 0);

    //Check for the quotable characters: '"', '\\', or control char (<32)
    __m128i needs_escape = __lsx_vseq_b(word, v34);
    needs_escape = __lsx_vor_v(needs_escape, __lsx_vseq_b(word, v92));
    needs_escape = __lsx_vor_v(needs_escape, __lsx_vslt_bu(word, v32));

    if (!__lsx_bz_v(needs_escape)){

      //Found quotable character - extract exact byte position
      uint64_t lo = __lsx_vpickve2gr_du(needs_escape,0);
      uint64_t hi = __lsx_vpickve2gr_du(needs_escape,1);
      size_t offset = ptr - reinterpret_cast<const uint8_t *>(view.data());
      if ( lo != 0) {
        return offset + trailing_zeroes(lo) / 8;
      } else {
        return offset + 8 + trailing_zeroes(hi) / 8;
      }
    }
    ptr += 16;
    remaining -= 16;
  }
  size_t current = len - remaining;
  return find_next_json_quotable_character_scalar(view, current);
}
#elif SIMDJSON_EXPERIMENTAL_HAS_RVV
simdjson_inline size_t
find_next_json_quotable_character(const std::string_view view,
                                  size_t location) noexcept {
  const size_t len = view.size();
  const uint8_t *ptr =
      reinterpret_cast<const uint8_t *>(view.data()) + location;
  size_t remaining = len - location;

  while (remaining > 0) {
    size_t vl = __riscv_vsetvl_e8m1(remaining);
    vuint8m1_t word = __riscv_vle8_v_u8m1(ptr, vl);

    // Check for quotable characters: '"', '\\', or control chars (< 32)
    vbool8_t needs_escape = __riscv_vmseq(word, (uint8_t)34, vl);
    needs_escape = __riscv_vmor(needs_escape,
        __riscv_vmseq(word, (uint8_t)92, vl), vl);
    needs_escape = __riscv_vmor(needs_escape,
        __riscv_vmsltu(word, (uint8_t)32, vl), vl);

    long first = __riscv_vfirst(needs_escape, vl);
    if (first >= 0) {
      size_t offset = ptr - reinterpret_cast<const uint8_t *>(view.data());
      return offset + first;
    }
    ptr += vl;
    remaining -= vl;
  }

  return len;
}
#elif SIMDJSON_EXPERIMENTAL_HAS_PPC64
simdjson_inline size_t
find_next_json_quotable_character(const std::string_view view,
                                  size_t location) noexcept {
  const size_t len = view.size();
  const uint8_t *ptr =
      reinterpret_cast<const uint8_t *>(view.data()) + location;
  size_t remaining = len - location;

  // SIMD constants for characters requiring escape
  __vector unsigned char v34 = vec_splats((unsigned char)34);  // '"'
  __vector unsigned char v92 = vec_splats((unsigned char)92);  // '\\'
  __vector unsigned char v32 = vec_splats((unsigned char)32);  // control char threshold

  // Bitmask for vec_vbpermq to extract one bit per byte
  const __vector unsigned char perm_mask = {0x78, 0x70, 0x68, 0x60, 0x58, 0x50,
                                            0x48, 0x40, 0x38, 0x30, 0x28, 0x20,
                                            0x18, 0x10, 0x08, 0x00};

  while (remaining >= 16) {
    __vector unsigned char word =
        vec_vsx_ld(0, reinterpret_cast<const unsigned char *>(ptr));

    // Check for quotable characters: '"', '\\', or control chars (< 32)
    __vector unsigned char needs_escape =
        (__vector unsigned char)vec_cmpeq(word, v34);
    needs_escape = vec_or(needs_escape,
        (__vector unsigned char)vec_cmpeq(word, v92));
    needs_escape = vec_or(needs_escape,
        (__vector unsigned char)vec_cmplt(word, v32));

    __vector unsigned long long result =
        (__vector unsigned long long)vec_vbpermq(needs_escape, perm_mask);
#ifdef __LITTLE_ENDIAN__
    unsigned int mask = static_cast<unsigned int>(result[1]);
#else
    unsigned int mask = static_cast<unsigned int>(result[0]);
#endif
    if (mask != 0) {
      size_t offset = ptr - reinterpret_cast<const uint8_t *>(view.data());
      return offset + __builtin_ctz(mask);
    }
    ptr += 16;
    remaining -= 16;
  }

  // Scalar fallback for remaining bytes
  size_t current = len - remaining;
  return find_next_json_quotable_character_scalar(view, current);
}
#else
SIMDJSON_CONSTEXPR_LAMBDA simdjson_inline size_t
find_next_json_quotable_character(const std::string_view view,
                                  size_t location) noexcept {
  return find_next_json_quotable_character_scalar(view, location);
}
#endif

SIMDJSON_CONSTEXPR_LAMBDA static std::string_view control_chars[] = {
    "\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005", "\\u0006",
    "\\u0007", "\\b",     "\\t",     "\\n",     "\\u000b", "\\f",     "\\r",
    "\\u000e", "\\u000f", "\\u0010", "\\u0011", "\\u0012", "\\u0013", "\\u0014",
    "\\u0015", "\\u0016", "\\u0017", "\\u0018", "\\u0019", "\\u001a", "\\u001b",
    "\\u001c", "\\u001d", "\\u001e", "\\u001f"};

// All Unicode characters may be placed within the quotation marks, except for
// the characters that MUST be escaped: quotation mark, reverse solidus, and the
// control characters (U+0000 through U+001F). There are two-character sequence
// escape representations of some popular characters:
// \", \\, \b, \f, \n, \r, \t.
SIMDJSON_CONSTEXPR_LAMBDA simdjson_inline void escape_json_char(char c, char *&out) {
  if (c == '"') {
    memcpy(out, "\\\"", 2);
    out += 2;
  } else if (c == '\\') {
    memcpy(out, "\\\\", 2);
    out += 2;
  } else {
    std::string_view v = control_chars[uint8_t(c)];
    memcpy(out, v.data(), v.size());
    out += v.size();
  }
}

#if SIMDJSON_EXPERIMENTAL_HAS_SSE2 || SIMDJSON_EXPERIMENTAL_HAS_NEON
#define SIMDJSON_BUILDER_HAS_BLOCK_ESCAPE 1
#endif

#if SIMDJSON_BUILDER_HAS_BLOCK_ESCAPE

#if SIMDJSON_EXPERIMENTAL_HAS_SSE2

using escape_vector = __m128i;
// Mask bits that each input byte contributes to escape_bitmask().
static constexpr unsigned escape_mask_bits = 1;

simdjson_inline escape_vector escape_load16(const uint8_t *p) noexcept {
  return _mm_loadu_si128(reinterpret_cast<const __m128i *>(p));
}

simdjson_inline void escape_store16(char *out, escape_vector v) noexcept {
  _mm_storeu_si128(reinterpret_cast<__m128i *>(out), v);
}

// Builds a vector whose bytes 0..7 come from a and bytes 8..15 from b.
simdjson_inline escape_vector escape_load8x2(const uint8_t *a,
                                             const uint8_t *b) noexcept {
  return _mm_unpacklo_epi64(
      _mm_loadl_epi64(reinterpret_cast<const __m128i *>(a)),
      _mm_loadl_epi64(reinterpret_cast<const __m128i *>(b)));
}

// Builds a vector whose bytes 0..3 come from a and bytes 4..7 from b. The
// upper half is unspecified; callers only look at the low eight bits.
simdjson_inline escape_vector escape_load4x2(const uint8_t *a,
                                             const uint8_t *b) noexcept {
  int32_t a32, b32;
  memcpy(&a32, a, 4);
  memcpy(&b32, b, 4);
  return _mm_unpacklo_epi32(_mm_cvtsi32_si128(a32), _mm_cvtsi32_si128(b32));
}

// Sets every bit of byte k when byte k of v is a quotable character ('"',
// '\\' or a control character).
simdjson_inline escape_vector escape_flags(escape_vector v) noexcept {
  const __m128i v34 = _mm_set1_epi8(34); // '"'
  const __m128i v92 = _mm_set1_epi8(92); // '\\'
  const __m128i v31 = _mm_set1_epi8(31); // for control char detection
  __m128i needs_escape = _mm_cmpeq_epi8(v, v34);
  needs_escape = _mm_or_si128(needs_escape, _mm_cmpeq_epi8(v, v92));
  return _mm_or_si128(
      needs_escape, _mm_cmpeq_epi8(_mm_subs_epu8(v, v31), _mm_setzero_si128()));
}

// True when any byte needs escaping. Kept separate from escape_bitmask
// because some instruction sets can answer it without leaving the vector
// register file; on SSE2 the compiler folds the two together.
simdjson_inline bool escape_any(escape_vector flags) noexcept {
  return _mm_movemask_epi8(flags) != 0;
}

// A 16-bit mask with bit k set when byte k needed escaping.
simdjson_inline uint64_t escape_bitmask(escape_vector flags) noexcept {
  return uint64_t(uint32_t(_mm_movemask_epi8(flags)));
}

#else // SIMDJSON_EXPERIMENTAL_HAS_NEON

using escape_vector = uint8x16_t;
static constexpr unsigned escape_mask_bits = 4;

simdjson_inline escape_vector escape_load16(const uint8_t *p) noexcept {
  return vld1q_u8(p);
}

simdjson_inline void escape_store16(char *out, escape_vector v) noexcept {
  vst1q_u8(reinterpret_cast<uint8_t *>(out), v);
}

simdjson_inline escape_vector escape_load8x2(const uint8_t *a,
                                             const uint8_t *b) noexcept {
  uint64_t a64, b64;
  memcpy(&a64, a, 8);
  memcpy(&b64, b, 8);
  return vreinterpretq_u8_u64(vsetq_lane_u64(b64, vdupq_n_u64(a64), 1));
}

simdjson_inline escape_vector escape_load4x2(const uint8_t *a,
                                             const uint8_t *b) noexcept {
  uint32_t a32, b32;
  memcpy(&a32, a, 4);
  memcpy(&b32, b, 4);
  return vreinterpretq_u8_u32(vsetq_lane_u32(b32, vdupq_n_u32(a32), 1));
}

simdjson_inline escape_vector escape_flags(escape_vector v) noexcept {
  uint8x16_t needs_escape = vceqq_u8(v, vdupq_n_u8(34));              // '"'
  needs_escape = vorrq_u8(needs_escape, vceqq_u8(v, vdupq_n_u8(92))); // '\\'
  return vorrq_u8(needs_escape, vcltq_u8(v, vdupq_n_u8(32)));
}

simdjson_inline bool escape_any(escape_vector flags) noexcept {
  return vmaxvq_u32(vreinterpretq_u32_u8(flags)) != 0;
}

// Four bits per byte rather than one: escape_block and the tail paths scale
// their shifts by escape_mask_bits to match.
simdjson_inline uint64_t escape_bitmask(escape_vector flags) noexcept {
  uint8x8_t narrowed = vshrn_n_u16(vreinterpretq_u16_u8(flags), 4);
  return vget_lane_u64(vreinterpret_u64_u8(narrowed), 0);
}

#endif // instruction set selection

// The escape bitmask of a 16-byte block.
simdjson_inline uint64_t escape_mask(escape_vector v) noexcept {
  return escape_bitmask(escape_flags(v));
}

// Copies n bytes with n < 16, using overlapping loads and stores. It never
// reads more than n bytes from src, nor writes more than n bytes to dst.
simdjson_inline void copy_lt16(char *dst, const uint8_t *src,
                               size_t n) noexcept {
  if (n >= 8) {
    memcpy(dst, src, 8);
    memcpy(dst + n - 8, src + n - 8, 8);
  } else if (n >= 4) {
    memcpy(dst, src, 4);
    memcpy(dst + n - 4, src + n - 4, 4);
  } else if (n > 0) {
    dst[0] = char(src[0]);
    dst[n >> 1] = char(src[n >> 1]);
    dst[n - 1] = char(src[n - 1]);
  }
}

// Escapes the bytes of src in the range [i, blockend), given that m is the
// (non-zero) escape mask for that range: the escape_mask_bits-wide lane of m
// at byte k is non-zero when src[i + k] requires escaping. Returns the updated
// output pointer.
simdjson_never_inline char *escape_block(const uint8_t *src, char *out,
                                         size_t i, size_t blockend,
                                         uint64_t m) noexcept {
  constexpr uint64_t lane = (uint64_t(1) << escape_mask_bits) - 1;

  size_t pos = i; // first byte not yet copied
  while (m) {
    const size_t tz = trailing_zeroes(m);
    const size_t next = i + tz / escape_mask_bits;
    // Copy the run of safe bytes that precedes this escape.
    copy_lt16(out, src + pos, next - pos);
    out += next - pos;
    escape_json_char(char(src[next]), out);
    pos = next + 1;
    m &= ~(lane << tz);
  }
  // Copy whatever follows the last escape.
  copy_lt16(out, src + pos, blockend - pos);
  return out + (blockend - pos);
}

// Writes the escaped version of input to out, returning the number of bytes
// written.
simdjson_really_inline size_t write_string_escaped(const std::string_view input, char *out) {
  const size_t len = input.size();
  const uint8_t *src = reinterpret_cast<const uint8_t *>(input.data());
  const char *const initout = out;

  size_t i = 0;
#if SIMDJSON_EXPERIMENTAL_HAS_SSE2 && defined(__AVX2__)
  while (i + 32 <= len) {
    const __m256i word = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + i));
    const __m256i flags = _mm256_or_si256(
        _mm256_or_si256(_mm256_cmpeq_epi8(word, _mm256_set1_epi8(34)),   // '"'
                        _mm256_cmpeq_epi8(word, _mm256_set1_epi8(92))),  // '\\'
        _mm256_cmpeq_epi8(_mm256_subs_epu8(word, _mm256_set1_epi8(31)),
                          _mm256_setzero_si256()));                      // control
    const uint32_t mask = uint32_t(_mm256_movemask_epi8(flags));
    if (simdjson_likely(mask == 0)) {
      _mm256_storeu_si256(reinterpret_cast<__m256i *>(out), word);
      out += 32;
    } else {
      for (size_t half = 0; half < 32; half += 16) {
        const uint64_t m = (mask >> half) & 0xFFFF;
        if (m == 0) {
          escape_store16(out, escape_load16(src + i + half));
          out += 16;
        } else {
          out = escape_block(src, out, i + half, i + half + 16, m);
        }
      }
    }
    i += 32;
  }
#endif
  while (i + 16 <= len) {
    escape_vector word = escape_load16(src + i);
    escape_vector flags = escape_flags(word);
    if (simdjson_likely(!escape_any(flags))) {
      escape_store16(out, word);
      out += 16;
    } else {
      out = escape_block(src, out, i, i + 16, escape_bitmask(flags));
    }
    i += 16;
  }
  if (i < len) {
    const size_t rem = len - i;
    uint64_t m;
    if (len >= 16) {
      // The last 16 bytes of the input are in bounds. Bit k of that block's
      // mask belongs to input position len - 16 + k, so shift it down to align
      // bit 0 with position i.
      m = escape_mask(escape_load16(src + len - 16)) >>
          (escape_mask_bits * (16 - rem));
    } else if (len >= 8) {
      // Here i == 0 and rem == len. Two overlapping 8-byte loads cover
      // [0, 8) and [len - 8, len), which is the whole input since len < 16.
      uint64_t mm = escape_mask(escape_load8x2(src, src + len - 8));
      constexpr uint64_t low8 = (uint64_t(1) << (escape_mask_bits * 8)) - 1;
      m = (mm & low8) |
          ((mm >> (escape_mask_bits * 8)) << (escape_mask_bits * (len - 8)));
    } else if (len >= 4) {
      // Same idea with two overlapping 4-byte loads.
      uint64_t mm = escape_mask(escape_load4x2(src, src + len - 4));
      constexpr uint64_t low4 = (uint64_t(1) << (escape_mask_bits * 4)) - 1;
      m = (mm & low4) | (((mm >> (escape_mask_bits * 4)) & low4)
                         << (escape_mask_bits * (len - 4)));
    } else {
      // Fewer than 4 bytes: at most three table lookups, no need for SIMD.
      for (size_t k = 0; k < len; k++) {
        uint8_t c = src[k];
        if (json_quotable_character[c]) {
          escape_json_char(char(c), out);
        } else {
          *out++ = char(c);
        }
      }
      return size_t(out - initout);
    }
    if (m == 0) {
      copy_lt16(out, src + i, rem);
      out += rem;
    } else {
      out = escape_block(src, out, i, len, m);
    }
  }
  return size_t(out - initout);
}

#else // SIMDJSON_BUILDER_HAS_BLOCK_ESCAPE

// Writes the escaped version of input to out, returning the number of bytes
// written. Uses SIMD position finding to locate quotable characters efficiently.
inline size_t write_string_escaped(const std::string_view input, char *out) {
  size_t mysize = input.size();

  // Use SIMD position finder directly - it returns mysize if no escape needed
  size_t location = find_next_json_quotable_character(input, 0);
  if (location == mysize) {
    // Fast path: no escaping needed
    memcpy(out, input.data(), input.size());
    return input.size();
  }

  const char *const initout = out;
  memcpy(out, input.data(), location);
  out += location;
  escape_json_char(input[location], out);
  location += 1;
  while (location < mysize) {
    size_t newlocation = find_next_json_quotable_character(input, location);
    memcpy(out, input.data() + location, newlocation - location);
    out += newlocation - location;
    location = newlocation;
    if (location == mysize) {
      break;
    }
    escape_json_char(input[location], out);
    location += 1;
  }
  return size_t(out - initout);
}

#endif // SIMDJSON_BUILDER_HAS_BLOCK_ESCAPE
#undef SIMDJSON_BUILDER_HAS_BLOCK_ESCAPE


simdjson_inline string_builder::string_builder(size_t initial_capacity)
    : buffer(new(std::nothrow) char[initial_capacity]), position(0),
      capacity(buffer.get() != nullptr ? initial_capacity : 0),
      is_valid(buffer.get() != nullptr) {}

simdjson_inline bool string_builder::capacity_check(size_t upcoming_bytes) {
  // We use the convention that when is_valid is false, then the capacity and
  // the position are 0.
  // Most of the time, this function will return true.
  if (simdjson_likely(upcoming_bytes <= capacity - position)) {
    return true;
  }
  // check for overflow, most of the time there is no overflow
  if (simdjson_unlikely(position + upcoming_bytes < position)) {
    return false;
  }
  // We will rarely get here.
  grow_buffer((std::max)(capacity * 2, position + upcoming_bytes));
  // If the buffer allocation failed, we set is_valid to false.
  return is_valid;
}

inline void string_builder::grow_buffer(size_t desired_capacity) {
  if (!is_valid) {
    return;
  }
  std::unique_ptr<char[]> new_buffer(new (std::nothrow) char[desired_capacity]);
  if (new_buffer.get() == nullptr) {
    set_valid(false);
    return;
  }
  std::memcpy(new_buffer.get(), buffer.get(), position);
  buffer.swap(new_buffer);
  capacity = desired_capacity;
}

simdjson_inline void string_builder::set_valid(bool valid) noexcept {
  if (!valid) {
    is_valid = false;
    capacity = 0;
    position = 0;
    buffer.reset();
  } else {
    is_valid = true;
  }
}

simdjson_inline size_t string_builder::size() const noexcept {
  return position;
}

simdjson_inline void string_builder::append(char c) noexcept {
  if (capacity_check(1)) {
    buffer.get()[position++] = c;
  }
}

simdjson_inline void string_builder::append_null() noexcept {
  constexpr char null_literal[] = "null";
  constexpr size_t null_len = sizeof(null_literal) - 1;
  if (capacity_check(null_len)) {
    std::memcpy(buffer.get() + position, null_literal, null_len);
    position += null_len;
  }
}

simdjson_inline void string_builder::clear() noexcept {
  position = 0;
  // if it was invalid, we should try to repair it
  if (!is_valid) {
    capacity = 0;
    buffer.reset();
    is_valid = true;
  }
}

namespace internal {

// Integer to decimal: James Edward Anhalt III's algorithm
static const char jeaiii_dd[201] =
    "00010203040506070809101112131415161718192021222324252627282930313233343536373839"
    "40414243444546474849505152535455565758596061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";
static const char jeaiii_fd[201] =
    "0\0" "1\0" "2\0" "3\0" "4\0" "5\0" "6\0" "7\0" "8\0" "9\0"
    "10111213141516171819202122232425262728293031323334353637383940414243444546474849"
    "50515253545556575859606162636465666768697071727374757677787980818283848586878889"
    "90919293949596979899";

simdjson_really_inline void jeaiii_write_dd(char *p, uint64_t k) noexcept {
  std::memcpy(p, &jeaiii_dd[2 * k], 2);
}
simdjson_really_inline void jeaiii_write_fd(char *p, uint64_t k) noexcept {
  std::memcpy(p, &jeaiii_fd[2 * k], 2);
}

// Caller guarantees n < 10^8. Writes 1 to 8 digits.
simdjson_really_inline char *jeaiii_lt1e8(char *b, uint32_t n) noexcept {
  constexpr uint64_t mask24 = (uint64_t(1) << 24) - 1;
  constexpr uint64_t mask32 = (uint64_t(1) << 32) - 1;
  if (n < 100) {
    jeaiii_write_fd(b, n);
    return n < 10 ? b + 1 : b + 2;
  }
  if (n < 1000000) {
    if (n < 10000) {
      const uint32_t f0 = uint32_t(10 * (1 << 24) / 1e3 + 1) * n;
      jeaiii_write_fd(b, f0 >> 24);
      b -= n < 1000;
      const uint32_t f2 = uint32_t(f0 & mask24) * 100;
      jeaiii_write_dd(b + 2, f2 >> 24);
      return b + 4;
    }
    const uint64_t f0 = uint64_t(10 * (1ull << 32) / 1e5 + 1) * n;
    jeaiii_write_fd(b, f0 >> 32);
    b -= n < 100000;
    const uint64_t f2 = (f0 & mask32) * 100;
    jeaiii_write_dd(b + 2, f2 >> 32);
    const uint64_t f4 = (f2 & mask32) * 100;
    jeaiii_write_dd(b + 4, f4 >> 32);
    return b + 6;
  }
  const uint64_t f0 = uint64_t(10 * (1ull << 48) / 1e7 + 1) * n >> 16;
  jeaiii_write_fd(b, f0 >> 32);
  b -= n < 10000000;
  const uint64_t f2 = (f0 & mask32) * 100;
  jeaiii_write_dd(b + 2, f2 >> 32);
  const uint64_t f4 = (f2 & mask32) * 100;
  jeaiii_write_dd(b + 4, f4 >> 32);
  const uint64_t f6 = (f4 & mask32) * 100;
  jeaiii_write_dd(b + 6, f6 >> 32);
  return b + 8;
}

// Caller guarantees z < 10^8. Always writes exactly 8 digits.
simdjson_really_inline char *jeaiii_8_digits(char *b, uint32_t z) noexcept {
  constexpr uint64_t mask32 = (uint64_t(1) << 32) - 1;
  const uint64_t f0 = (uint64_t((1ull << 48) / 1e6 + 1) * z >> 16) + 1;
  jeaiii_write_dd(b, f0 >> 32);
  const uint64_t f2 = (f0 & mask32) * 100;
  jeaiii_write_dd(b + 2, f2 >> 32);
  const uint64_t f4 = (f2 & mask32) * 100;
  jeaiii_write_dd(b + 4, f4 >> 32);
  const uint64_t f6 = (f4 & mask32) * 100;
  jeaiii_write_dd(b + 6, f6 >> 32);
  return b + 8;
}

// Caller guarantees 10^8 <= n < 2^32. Writes 9 or 10 digits.
simdjson_really_inline char *jeaiii_9_or_10(char *b, uint64_t n) noexcept {
  constexpr uint64_t mask57 = (uint64_t(1) << 57) - 1;
  const uint64_t f0 = uint64_t(10 * (1ull << 57) / 1e9 + 1) * n;
  jeaiii_write_fd(b, f0 >> 57);
  b -= n < 1000000000;
  const uint64_t f2 = (f0 & mask57) * 100;
  jeaiii_write_dd(b + 2, f2 >> 57);
  const uint64_t f4 = (f2 & mask57) * 100;
  jeaiii_write_dd(b + 4, f4 >> 57);
  const uint64_t f6 = (f4 & mask57) * 100;
  jeaiii_write_dd(b + 6, f6 >> 57);
  const uint64_t f8 = (f6 & mask57) * 100;
  jeaiii_write_dd(b + 8, f8 >> 57);
  return b + 10;
}

simdjson_really_inline char *write_uint_jeaiii(char *b, uint64_t n) noexcept {
  if (n < 100000000) {
    return jeaiii_lt1e8(b, uint32_t(n));
  }
  if (n < (uint64_t(1) << 32)) {
    return jeaiii_9_or_10(b, n);
  }
  // At least 10 digits: the low 8 digits, and 2 to 12 digits above them.
  const uint32_t z = uint32_t(n % 100000000);
  uint64_t u = n / 100000000;
  if (u < 100000000) {
    // u has 2 to 8 digits (if u < 10, n would be below 2^32).
    b = jeaiii_lt1e8(b, uint32_t(u));
  } else if (u < (uint64_t(1) << 32)) {
    b = jeaiii_9_or_10(b, u);
  } else {
    // u has 11 or 12 digits: split off 8 more.
    const uint32_t y = uint32_t(u % 100000000);
    u /= 100000000;
    b = jeaiii_lt1e8(b, uint32_t(u)); // 3 or 4 digits
    b = jeaiii_8_digits(b, y);
  }
  return jeaiii_8_digits(b, z);
}

// Writes v at p, which must have to_chars_buffer_size bytes available, and
// returns the end of what was written.
simdjson_inline char *write_double(char *p, double v) noexcept {
#if SIMDJSON_ENABLE_NAN_INF
  if (simdjson_unlikely(!std::isfinite(v))) {
    if (std::isnan(v)) {
      std::memcpy(p, "NaN", 3);
      return p + 3;
    }
    if (v < 0) {
      *p++ = '-';
    }
    std::memcpy(p, "Infinity", 8);
    return p + 8;
  }
#endif
  return simdjson::internal::to_chars(p, nullptr, v);
}
} // namespace internal

template <typename number_type, typename>
simdjson_inline void string_builder::append(number_type v) noexcept {
  static_assert(std::is_same<number_type, bool>::value ||
                    std::is_integral<number_type>::value ||
                    std::is_floating_point<number_type>::value,
                "Unsupported number type");
  // If C++17 is available, we can 'if constexpr' here.
  SIMDJSON_IF_CONSTEXPR(std::is_same<number_type, bool>::value) {
    if (v) {
      constexpr char true_literal[] = "true";
      constexpr size_t true_len = sizeof(true_literal) - 1;
      if (capacity_check(true_len)) {
        std::memcpy(buffer.get() + position, true_literal, true_len);
        position += true_len;
      }
    } else {
      constexpr char false_literal[] = "false";
      constexpr size_t false_len = sizeof(false_literal) - 1;
      if (capacity_check(false_len)) {
        std::memcpy(buffer.get() + position, false_literal, false_len);
        position += false_len;
      }
    }
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_unsigned<number_type>::value) {
    constexpr size_t max_number_size = 20;
    if (capacity_check(max_number_size)) {
      using unsigned_type = typename std::make_unsigned<number_type>::type;
      char* end = internal::write_uint_jeaiii(
          buffer.get() + position,
          static_cast<uint64_t>(static_cast<unsigned_type>(v)));
      position = end - buffer.get();
    }
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_integral<number_type>::value) {
    // 19 digits (max abs value of int64_t) + optional minus sign.
    constexpr size_t max_number_size = 20;
    if (capacity_check(max_number_size)) {
      using unsigned_type = typename std::make_unsigned<number_type>::type;
      bool negative = v < 0;
      // 0 - pv (rather than -pv) avoids an MSVC unary-minus warning.
      unsigned_type pv = negative
          ? unsigned_type(0) - static_cast<unsigned_type>(v)
          : static_cast<unsigned_type>(v);
      // Branchless: always write '-', advance only if negative.
      buffer.get()[position] = '-';
      position += negative;
      char* end = internal::write_uint_jeaiii(
          buffer.get() + position, static_cast<uint64_t>(pv));
      position = end - buffer.get();
    }
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_floating_point<number_type>::value) {
    // Must reserve to_chars_buffer_size (40): only ~24 chars are emitted,
    // but to_chars over-writes with fixed-size 16/17-byte copies so the
    // compiler can inline mem* (see simdjson::internal::to_chars_buffer_size).
    constexpr size_t max_number_size = simdjson::internal::to_chars_buffer_size;
    if (capacity_check(max_number_size)) {
      char *end = internal::write_double(buffer.get() + position, double(v));
      position = end - buffer.get();
    }
  }
}

simdjson_inline void
string_builder::escape_and_append(std::string_view input) noexcept {
  // escaping might turn a control character into \x00xx so 6 characters.
  // Guard against size_t overflow in the multiplication below.
  if (input.size() > (std::numeric_limits<size_t>::max)() / 6) {
    set_valid(false);
    return;
  }
  if (capacity_check(6 * input.size())) {
    position += write_string_escaped(input, buffer.get() + position);
  }
}

simdjson_inline void
string_builder::escape_and_append_with_quotes(std::string_view input) noexcept {
  // escaping might turn a control character into \x00xx so 6 characters.
  // Guard against size_t overflow in the arithmetic below.
  if (input.size() > ((std::numeric_limits<size_t>::max)() - 2) / 6) {
    set_valid(false);
    return;
  }
  if (capacity_check(2 + 6 * input.size())) {
    buffer.get()[position++] = '"';
    position += write_string_escaped(input, buffer.get() + position);
    buffer.get()[position++] = '"';
  }
}

simdjson_inline void
string_builder::escape_and_append_with_quotes(char input) noexcept {
  // escaping might turn a control character into \x00xx so 6 characters.
  if (capacity_check(2 + 6 * 1)) {
    buffer.get()[position++] = '"';
    std::string_view cinput(&input, 1);
    position += write_string_escaped(cinput, buffer.get() + position);
    buffer.get()[position++] = '"';
  }
}

simdjson_inline void
string_builder::escape_and_append_with_quotes(const char *input) noexcept {
  std::string_view cinput(input);
  escape_and_append_with_quotes(cinput);
}
#if SIMDJSON_SUPPORTS_CONCEPTS
template <constevalutil::fixed_string key>
simdjson_inline void string_builder::escape_and_append_with_quotes() noexcept {
  escape_and_append_with_quotes(constevalutil::string_constant<key>::value);
}
#endif

simdjson_inline void string_builder::append_raw(const char *c) noexcept {
  // char_traits::length is constexpr; lets the compiler fold the length
  // when called with a pointer to a compile-time-constant string.
  size_t len = std::char_traits<char>::length(c);
  append_raw(c, len);
}

simdjson_inline void
string_builder::append_raw(std::string_view input) noexcept {
  if (capacity_check(input.size())) {
    std::memcpy(buffer.get() + position, input.data(), input.size());
    position += input.size();
  }
}

simdjson_inline void string_builder::append_raw(const char *str,
                                                size_t len) noexcept {
  if (capacity_check(len)) {
    std::memcpy(buffer.get() + position, str, len);
    position += len;
  }
}

template <size_t N>
simdjson_inline void string_builder::append_raw_n(const char *str) noexcept {
  if (capacity_check(N)) {
    std::memcpy(buffer.get() + position, str, N);
    position += N;
  }
}
#if SIMDJSON_SUPPORTS_CONCEPTS
// Support for optional types (std::optional, etc.)
template <concepts::optional_type T>
  requires(!require_custom_serialization<T>)
simdjson_inline void string_builder::append(const T &opt) {
  if (opt) {
    append(*opt);
  } else {
    append_null();
  }
}

template <typename T>
  requires(require_custom_serialization<T>)
simdjson_inline void string_builder::append(T &&val) {
  serialize(*this, std::forward<T>(val));
}

template <typename T>
  requires(std::is_convertible<T, std::string_view>::value ||
           std::is_same<T, const char *>::value)
simdjson_inline void string_builder::append(const T &value) {
  escape_and_append_with_quotes(value);
}
#endif

#if SIMDJSON_SUPPORTS_RANGES && SIMDJSON_SUPPORTS_CONCEPTS
// Support for range-based appending (std::ranges::view, etc.)
template <std::ranges::range R>
  requires(!std::is_convertible<R, std::string_view>::value && !concepts::optional_type<R> && !require_custom_serialization<R>)
simdjson_inline void string_builder::append(const R &range) noexcept {
  auto it = std::ranges::begin(range);
  auto end = std::ranges::end(range);
  if constexpr (concepts::is_pair<std::ranges::range_value_t<R>>) {
    start_object();

    if (it == end) {
      end_object();
      return; // Handle empty range
    }
    // Append first item without leading comma
    append_key_value(it->first, it->second);
    ++it;

    // Append remaining items with preceding commas
    for (; it != end; ++it) {
      append_comma();
      append_key_value(it->first, it->second);
    }
    end_object();
  } else {
    start_array();
    if (it == end) {
      end_array();
      return; // Handle empty range
    }

    // Append first item without leading comma
    append(*it);
    ++it;

    // Append remaining items with preceding commas
    for (; it != end; ++it) {
      append_comma();
      append(*it);
    }
    end_array();
  }
}

#endif

#if SIMDJSON_EXCEPTIONS
simdjson_inline string_builder::operator std::string() const noexcept(false) {
  return std::string(operator std::string_view());
}

simdjson_inline string_builder::operator std::string_view() const
    noexcept(false) simdjson_lifetime_bound {
  return view();
}
#endif

simdjson_inline simdjson_result<std::string_view>
string_builder::view() const noexcept {
  if (!is_valid) {
    return simdjson::OUT_OF_CAPACITY;
  }
  return std::string_view(buffer.get(), position);
}

simdjson_inline simdjson_result<const char *> string_builder::c_str() noexcept {
  if (capacity_check(1)) {
    buffer.get()[position] = '\0';
    return buffer.get();
  }
  return simdjson::OUT_OF_CAPACITY;
}

simdjson_inline bool string_builder::validate_unicode() const noexcept {
  return simdjson::validate_utf8(buffer.get(), position);
}

simdjson_inline void string_builder::start_object() noexcept {
  if (capacity_check(1)) {
    buffer.get()[position++] = '{';
  }
}

simdjson_inline void string_builder::end_object() noexcept {
  if (capacity_check(1)) {
    buffer.get()[position++] = '}';
  }
}

simdjson_inline void string_builder::start_array() noexcept {
  if (capacity_check(1)) {
    buffer.get()[position++] = '[';
  }
}

simdjson_inline void string_builder::end_array() noexcept {
  if (capacity_check(1)) {
    buffer.get()[position++] = ']';
  }
}

simdjson_inline void string_builder::append_comma() noexcept {
  if (capacity_check(1)) {
    buffer.get()[position++] = ',';
  }
}

simdjson_inline void string_builder::append_colon() noexcept {
  if (capacity_check(1)) {
    buffer.get()[position++] = ':';
  }
}

template <typename key_type, typename value_type>
simdjson_inline void
string_builder::append_key_value(key_type key, value_type value) noexcept {
  static_assert(std::is_same<key_type, const char *>::value ||
                    std::is_convertible<key_type, std::string_view>::value,
                "Unsupported key type");
  escape_and_append_with_quotes(key);
  append_colon();
  SIMDJSON_IF_CONSTEXPR(std::is_same<value_type, std::nullptr_t>::value) {
    append_null();
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_same<value_type, char>::value) {
    escape_and_append_with_quotes(value);
  }
  else SIMDJSON_IF_CONSTEXPR(
      std::is_convertible<value_type, std::string_view>::value) {
    escape_and_append_with_quotes(value);
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_same<value_type, const char *>::value) {
    escape_and_append_with_quotes(value);
  }
  else {
    append(value);
  }
}

#if SIMDJSON_SUPPORTS_CONCEPTS
template <constevalutil::fixed_string key, typename value_type>
simdjson_inline void
string_builder::append_key_value(value_type value) noexcept {
  escape_and_append_with_quotes<key>();
  append_colon();
  SIMDJSON_IF_CONSTEXPR(std::is_same<value_type, std::nullptr_t>::value) {
    append_null();
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_same<value_type, char>::value) {
    escape_and_append_with_quotes(value);
  }
  else SIMDJSON_IF_CONSTEXPR(
      std::is_convertible<value_type, std::string_view>::value) {
    escape_and_append_with_quotes(value);
  }
  else SIMDJSON_IF_CONSTEXPR(std::is_same<value_type, const char *>::value) {
    escape_and_append_with_quotes(value);
  }
  else {
    append(value);
  }
}
#endif

} // namespace builder
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_GENERIC_STRING_BUILDER_INL_H
