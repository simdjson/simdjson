#ifndef SIMDJSON_INTERNAL_JSON_ESCAPE_H
#define SIMDJSON_INTERNAL_JSON_ESCAPE_H

// Escaping of JSON strings, shared by the builder and the DOM serializer.

#include "simdjson/base.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>
#ifdef _MSC_VER
#include <intrin.h>
#endif

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
namespace internal {

// The index of the lowest set bit of a non-zero value. The escape code runs
// outside any implementation, so it cannot use their trailing_zeroes.
simdjson_inline int trailing_zeroes(uint64_t input_num) noexcept {
#ifdef _MSC_VER
  unsigned long ret;
#if defined(_M_X64) || defined(_M_ARM64)
  _BitScanForward64(&ret, input_num);
#else
  if (!_BitScanForward(&ret, (unsigned long)input_num)) {
    _BitScanForward(&ret, (unsigned long)(input_num >> 32));
    ret += 32;
  }
#endif
  return int(ret);
#else
  return __builtin_ctzll(input_num);
#endif
}

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

#if SIMDJSON_EXPERIMENTAL_HAS_SSE2 || SIMDJSON_EXPERIMENTAL_HAS_NEON
#define SIMDJSON_BUILDER_HAS_BLOCK_ESCAPE 1
#endif

// With block escaping (SSE2/NEON), write_string_escaped scans the input itself.
#if !SIMDJSON_BUILDER_HAS_BLOCK_ESCAPE
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
#if SIMDJSON_EXPERIMENTAL_HAS_LASX
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
#endif // !SIMDJSON_BUILDER_HAS_BLOCK_ESCAPE

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

} // namespace internal
} // namespace simdjson

#endif // SIMDJSON_INTERNAL_JSON_ESCAPE_H
