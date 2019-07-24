#ifndef SIMDJSON_STAGE1_FIND_MARKS_H
#define SIMDJSON_STAGE1_FIND_MARKS_H

#include <cassert>
#include "simdjson/common_defs.h"
#include "simdjson/parsedjson.h"
#include "simdjson/portability.h"

#if defined (__AVX2__)
#elif defined (__SSE4_2__) || (defined(_MSC_VER) && defined(_M_AMD64))
#elif defined(__ARM_NEON) || (defined(_MSC_VER) && defined(_M_ARM64))
#include <arm_neon.h>
#else
#warning It appears that neither ARM NEON nor AVX2 nor SSE are detected.
#endif // (__AVX2__)

#ifndef SIMDJSON_SKIPUTF8VALIDATION
#define SIMDJSON_UTF8VALIDATE
#endif

// It seems that many parsers do UTF-8 validation.
// RapidJSON does not do it by default, but a flag
// allows it.
#ifdef SIMDJSON_UTF8VALIDATE
#if defined (__AVX2__)
#include "simdjson/simdutf8check.h"
#elif defined (__SSE4_2__) || (defined(_MSC_VER) && defined(_M_AMD64))
#include "simdjson/simdutf8check.h"
#elif defined(__ARM_NEON) || (defined(_MSC_VER) && defined(_M_ARM64))
#include "simdjson/simdutf8check_neon.h"
#endif // (__AVX2__)
#endif  // SIMDJSON_UTF8VALIDATE

//#define TRANSPOSE

namespace simdjson {
/**
 * 64 bytes (512 bits) of input in SIMD register(s).
 */
template<instruction_set>
struct simd_input;
/**
 * 256 bits of bitmask, in SIMD register(s).
 */
template<instruction_set>
struct simd_bitmask;

#ifdef __AVX2__
template<>
struct simd_input<instruction_set::avx2>
{
  __m256i lo;
  __m256i hi;
};
template<>
struct simd_bitmask<instruction_set::avx2>
{
  __m256i mask;
};
#endif

#if defined(__SSE4_2__) || (defined(_MSC_VER) && defined(_M_AMD64))
template<>
struct simd_input<instruction_set::sse4_2>
{
  __m128i v0;
  __m128i v1;
  __m128i v2;
  __m128i v3;
};
template<>
struct simd_bitmask<instruction_set::sse4_2>
{
	__m128i lo;
	__m128i hi;
};
#endif

#if defined(__ARM_NEON)  || (defined(_MSC_VER) && defined(_M_ARM64))
template<> struct simd_input<instruction_set::neon>
{
#ifndef TRANSPOSE
  uint8x16_t i0;
  uint8x16_t i1;
  uint8x16_t i2;
  uint8x16_t i3;
#else
  uint8x16x4_t i;
#endif
};
#endif

#if defined(__ARM_NEON)  || (defined(_MSC_VER) && defined(_M_ARM64))
really_inline
uint16_t neonmovemask(uint8x16_t input) {
  const uint8x16_t bitmask = { 0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                               0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  uint8x16_t minput = vandq_u8(input, bitmask);
  uint8x16_t tmp = vpaddq_u8(minput, minput);
  tmp = vpaddq_u8(tmp, tmp);
  tmp = vpaddq_u8(tmp, tmp);
  return vgetq_lane_u16(vreinterpretq_u16_u8(tmp), 0);
}

really_inline
uint64_t neonmovemask_bulk(uint8x16_t p0, uint8x16_t p1, uint8x16_t p2, uint8x16_t p3) {
#ifndef TRANSPOSE
  const uint8x16_t bitmask = { 0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                               0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  uint8x16_t t0 = vandq_u8(p0, bitmask);
  uint8x16_t t1 = vandq_u8(p1, bitmask);
  uint8x16_t t2 = vandq_u8(p2, bitmask);
  uint8x16_t t3 = vandq_u8(p3, bitmask);
  uint8x16_t sum0 = vpaddq_u8(t0, t1);
  uint8x16_t sum1 = vpaddq_u8(t2, t3);
  sum0 = vpaddq_u8(sum0, sum1);
  sum0 = vpaddq_u8(sum0, sum0);
  return vgetq_lane_u64(vreinterpretq_u64_u8(sum0), 0);
#else
  const uint8x16_t bitmask1 = { 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10,
                                0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10};
  const uint8x16_t bitmask2 = { 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20,
                                0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20};
  const uint8x16_t bitmask3 = { 0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0x04, 0x40,
                                0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0x04, 0x40};
  const uint8x16_t bitmask4 = { 0x08, 0x80, 0x08, 0x80, 0x08, 0x80, 0x08, 0x80,
                                0x08, 0x80, 0x08, 0x80, 0x08, 0x80, 0x08, 0x80};
  uint8x16_t t0 = vandq_u8(p0, bitmask1);
  uint8x16_t t1 = vbslq_u8(bitmask2, p1, t0);
  uint8x16_t t2 = vbslq_u8(bitmask3, p2, t1);
  uint8x16_t tmp = vbslq_u8(bitmask4, p3, t2);
  uint8x16_t sum = vpaddq_u8(tmp, tmp);
  return vgetq_lane_u64(vreinterpretq_u64_u8(sum), 0);
#endif
}
#endif

template<instruction_set T>
simd_input<T> fill_input(const uint8_t * ptr);

#ifdef __AVX2__
template<> really_inline
simd_input<instruction_set::avx2> fill_input<instruction_set::avx2>(const uint8_t * ptr) {
	struct simd_input<instruction_set::avx2> in;
	in.lo = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr + 0));
	in.hi = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(ptr + 32));
	return in;
}
#endif

#if defined(__SSE4_2__) || (defined(_MSC_VER) && defined(_M_AMD64))
template<> really_inline
simd_input<instruction_set::sse4_2> fill_input<instruction_set::sse4_2>(const uint8_t * ptr) {
	struct simd_input<instruction_set::sse4_2> in;
	in.v0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 0));
	in.v1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 16));
	in.v2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 32));
	in.v3 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 48));
	return in;
}
#endif

#if defined(__ARM_NEON)  || (defined(_MSC_VER) && defined(_M_ARM64))
template<> really_inline
simd_input<instruction_set::neon> fill_input<instruction_set::neon>(const uint8_t * ptr) {
	struct simd_input<instruction_set::neon> in;
#ifndef TRANSPOSE
	in.i0 = vld1q_u8(ptr + 0);
	in.i1 = vld1q_u8(ptr + 16);
	in.i2 = vld1q_u8(ptr + 32);
	in.i3 = vld1q_u8(ptr + 48);
#else
	in.i = vld4q_u8(ptr);
#endif
	return in;
}
#endif

template<instruction_set T>
uint64_t compute_quote_mask(uint64_t quote_bits);

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
}

// In practice, if you have NEON or __PCLMUL__, you would
// always want to use them, but it might be useful, for research
// purposes, to disable it willingly, that's what SIMDJSON_AVOID_CLMUL
// does.
// Also: we don't know of an instance where AVX2 is supported but 
// where clmul is not supported, so check for both, to be sure.
#ifdef SIMDJSON_AVOID_CLMUL
template<instruction_set T> really_inline
uint64_t compute_quote_mask(uint64_t quote_bits) {
  return portable_compute_quote_mask(quote_bits);
}
#else
template<instruction_set>
uint64_t compute_quote_mask(uint64_t quote_bits);

#ifdef __AVX2__ 
template<> really_inline
uint64_t compute_quote_mask<instruction_set::avx2>(uint64_t quote_bits) {
  // There should be no such thing with a processing supporting avx2
  // but not clmul.
  uint64_t quote_mask = _mm_cvtsi128_si64(_mm_clmulepi64_si128(
      _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));
  return quote_mask;
}
#endif

#if defined(__SSE4_2__) || (defined(_MSC_VER) && defined(_M_AMD64))
template<> really_inline
uint64_t compute_quote_mask<instruction_set::sse4_2>(uint64_t quote_bits) {
  // CLMUL is supported on some SSE42 hardware such as Sandy Bridge,
  // but not on others.
#ifdef __PCLMUL__
  return _mm_cvtsi128_si64(_mm_clmulepi64_si128(
      _mm_set_epi64x(0ULL, quote_bits), _mm_set1_epi8(0xFF), 0));
#else
  return portable_compute_quote_mask(quote_bits);
#endif
}
#endif

#if defined(__ARM_NEON)  || (defined(_MSC_VER) && defined(_M_ARM64))
template<> really_inline
uint64_t compute_quote_mask<instruction_set::neon>(uint64_t quote_bits) {
#ifdef __ARM_FEATURE_CRYPTO // some ARM processors lack this extension
  return vmull_p64( -1ULL, quote_bits);
#else
  return portable_compute_quote_mask(quote_bits);
#endif 
}
#endif
#endif // SIMDJSON_AVOID_CLMUL

#ifdef SIMDJSON_UTF8VALIDATE
// Holds the state required to perform check_utf8().
template<instruction_set>
struct utf8_checking_state;

#ifdef __AVX2__
template<>
struct utf8_checking_state<instruction_set::avx2>
{
  __m256i has_error = _mm256_setzero_si256();
  avx_processed_utf_bytes previous {
    _mm256_setzero_si256(), // rawbytes
    _mm256_setzero_si256(), // high_nibbles
    _mm256_setzero_si256()  // carried_continuations
  };
};
#endif

#if defined(__SSE4_2__) || (defined(_MSC_VER) && defined(_M_AMD64))
template<>
struct utf8_checking_state<instruction_set::sse4_2>
{
  __m128i has_error = _mm_setzero_si128();
  processed_utf_bytes previous {
    _mm_setzero_si128(), // rawbytes
    _mm_setzero_si128(), // high_nibbles
    _mm_setzero_si128()  // carried_continuations
  };
};
#endif

#if defined(__ARM_NEON)  || (defined(_MSC_VER) && defined(_M_ARM64))
template<>
struct utf8_checking_state<instruction_set::neon>
{
  int8x16_t has_error {};
  processed_utf_bytes previous {};
};
#endif

#if defined(__ARM_NEON)  || (defined(_MSC_VER) && defined(_M_ARM64))
// Checks that all bytes are ascii
really_inline
bool check_ascii_neon(simd_input<instruction_set::neon> in) {
  // checking if the most significant bit is always equal to 0.
  uint8x16_t highbit = vdupq_n_u8(0x80);
  uint8x16_t t0 = vorrq_u8(in.i0, in.i1);
  uint8x16_t t1 = vorrq_u8(in.i2, in.i3);
  uint8x16_t t3 = vorrq_u8(t0, t1);
  uint8x16_t t4 = vandq_u8(t3, highbit);
  uint64x2_t v64 = vreinterpretq_u64_u8(t4);
  uint32x2_t v32 = vqmovn_u64(v64);
  uint64x1_t result = vreinterpret_u64_u32(v32);
  return vget_lane_u64(result, 0) == 0;
}
#endif

template<instruction_set T>
void check_utf8(simd_input<T> in, utf8_checking_state<T>& state);

template<instruction_set T>
void check_utf8_256(const uint8_t *buf, utf8_checking_state<T>& state) {
  for (int lane = 0; lane < 4; lane++) {
    simd_input<T> in = fill_input<T>(buf + lane * 64);
#ifdef SIMDJSON_UTF8VALIDATE
    check_utf8<T>(in, state);
#endif
  }
}

#ifdef __AVX2__
template<> really_inline
void check_utf8<instruction_set::avx2>(simd_input<instruction_set::avx2> in,
                utf8_checking_state<instruction_set::avx2>& state) {
  __m256i highbit = _mm256_set1_epi8(0x80);
  if ((_mm256_testz_si256(_mm256_or_si256(in.lo, in.hi), highbit)) == 1) {
    // it is ascii, we just check continuation
    state.has_error = _mm256_or_si256(
        _mm256_cmpgt_epi8(
            state.previous.carried_continuations,
            _mm256_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                             9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 1)),
        state.has_error);
  } else {
    // it is not ascii so we have to do heavy work
    state.previous = avxcheckUTF8Bytes(in.lo, &(state.previous), &(state.has_error));
    state.previous = avxcheckUTF8Bytes(in.hi, &(state.previous), &(state.has_error));
  }
}
#endif //__AVX2__

#if defined(__SSE4_2__) || (defined(_MSC_VER) && defined(_M_AMD64))
template<> really_inline
void check_utf8<instruction_set::sse4_2>(simd_input<instruction_set::sse4_2> in,
                utf8_checking_state<instruction_set::sse4_2>& state) {
  __m128i highbit = _mm_set1_epi8(0x80);
  if ((_mm_testz_si128(_mm_or_si128(in.v0, in.v1), highbit)) == 1) {
    // it is ascii, we just check continuation
    state.has_error = _mm_or_si128(
        _mm_cmpgt_epi8(
            state.previous.carried_continuations,
            _mm_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 1)),
        state.has_error);
  } else {
    // it is not ascii so we have to do heavy work
    state.previous = checkUTF8Bytes(in.v0, &(state.previous), &(state.has_error));
    state.previous = checkUTF8Bytes(in.v1, &(state.previous), &(state.has_error));
  }

  if ((_mm_testz_si128(_mm_or_si128(in.v2, in.v3), highbit)) == 1) {
    // it is ascii, we just check continuation
    state.has_error = _mm_or_si128(
            _mm_cmpgt_epi8(
                    state.previous.carried_continuations,
                    _mm_setr_epi8(9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 1)),
            state.has_error);
  } else {
    // it is not ascii so we have to do heavy work
    state.previous = checkUTF8Bytes(in.v2, &(state.previous), &(state.has_error));
    state.previous = checkUTF8Bytes(in.v3, &(state.previous), &(state.has_error));
  }
}
#endif // __SSE4_2

#if defined(__ARM_NEON)  || (defined(_MSC_VER) && defined(_M_ARM64))
template<> really_inline
void check_utf8<instruction_set::neon>(simd_input<instruction_set::neon> in,
                utf8_checking_state<instruction_set::neon>& state) {
  if (check_ascii_neon(in)) {
    // All bytes are ascii. Therefore the byte that was just before must be ascii too.
    // We only check the byte that was just before simd_input. Nines are arbitrary values.
    const int8x16_t verror = (int8x16_t){9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 1};
    state.has_error =
        vorrq_s8(vreinterpretq_s8_u8(vcgtq_s8(state.previous.carried_continuations,
                                    verror)),
                     state.has_error);
  } else {
    // it is not ascii so we have to do heavy work
    state.previous = checkUTF8Bytes(vreinterpretq_s8_u8(in.i0), &(state.previous), &(state.has_error));
    state.previous = checkUTF8Bytes(vreinterpretq_s8_u8(in.i1), &(state.previous), &(state.has_error));
    state.previous = checkUTF8Bytes(vreinterpretq_s8_u8(in.i2), &(state.previous), &(state.has_error));
    state.previous = checkUTF8Bytes(vreinterpretq_s8_u8(in.i3), &(state.previous), &(state.has_error));
  }
}
#endif // __ARM_NEON

// Checks if the utf8 validation has found any error.
template<instruction_set T>
errorValues check_utf8_errors(utf8_checking_state<T>& state);

#ifdef __AVX2__
template<> really_inline
errorValues check_utf8_errors<instruction_set::avx2>(utf8_checking_state<instruction_set::avx2>& state) {
  return _mm256_testz_si256(state.has_error, state.has_error) == 0 ? simdjson::UTF8_ERROR : simdjson::SUCCESS;
}
#endif

#if defined(__SSE4_2__) || (defined(_MSC_VER) && defined(_M_AMD64))
template<> really_inline
errorValues check_utf8_errors<instruction_set::sse4_2>(utf8_checking_state<instruction_set::sse4_2>& state) {
  return _mm_testz_si128(state.has_error, state.has_error) == 0 ? simdjson::UTF8_ERROR : simdjson::SUCCESS;
}
#endif

#if defined(__ARM_NEON)  || (defined(_MSC_VER) && defined(_M_ARM64))
template<> really_inline
errorValues check_utf8_errors<instruction_set::neon>(utf8_checking_state<instruction_set::neon>& state) {
  uint64x2_t v64 = vreinterpretq_u64_s8(state.has_error);
  uint32x2_t v32 = vqmovn_u64(v64);
  uint64x1_t result = vreinterpret_u64_u32(v32);
  return vget_lane_u64(result, 0) != 0 ? simdjson::UTF8_ERROR : simdjson::SUCCESS;
}
#endif
#endif // SIMDJSON_UTF8VALIDATE

template<instruction_set T>
void split_bitmask(uint64_t (&dest)[4], const simd_bitmask<T> bitmask);
template<instruction_set T>
simd_bitmask<T> join_bitmask(const uint64_t bitmask[4]);
template<instruction_set T>
simd_bitmask<T> set_bitmask(const uint64_t hi, const uint64_t mid_hi, const uint64_t mid_lo, const uint64_t lo);
template<instruction_set T>
simd_bitmask<T> splat_bitmask(const uint64_t bitmask);
template<instruction_set T>
really_inline simd_bitmask<T> add_overflow(simd_bitmask<T> a, simd_bitmask<T> b, uint64_t& overflow) {
	uint64_t split_a[4]; split_bitmask<T>(split_a, a);
	uint64_t split_b[4]; split_bitmask<T>(split_b, b);
	uint64_t result[4];
	for (int lane = 3; lane >= 0; lane--) {
		// TODO can we do this with less than 2 add_overflows?
		overflow = add_overflow(split_a[lane], overflow, &result[lane]) ? 1 : 0;
		overflow = add_overflow(result[lane], split_b[lane], &result[lane]) ? 1 : overflow;
	}
	return join_bitmask<T>(result);
}
template<instruction_set T>
really_inline simd_bitmask<T> operator+(simd_bitmask<T> a, simd_bitmask<T> b) {
	uint64_t overflow;
	return add_overflow(a, b, overflow);
}

#ifdef __AVX2__
template<>
really_inline void split_bitmask(uint64_t(&dest)[4], const simd_bitmask<instruction_set::avx2> bitmask) {
	_mm256_storeu_si256(reinterpret_cast<__m256i*>(&dest[0]), bitmask.mask);
}
template<>
really_inline simd_bitmask<instruction_set::avx2> join_bitmask(const uint64_t bitmasks[4]) {
  simd_bitmask<instruction_set::avx2> result;
  result.mask = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&bitmasks[0]));
  return result;
}
template<>
really_inline simd_bitmask<instruction_set::avx2> set_bitmask(const uint64_t hi, const uint64_t mid_hi, const uint64_t mid_lo, const uint64_t lo) {
	simd_bitmask<instruction_set::avx2> result;
	result.mask = _mm256_set_epi64x(hi, mid_hi, mid_lo, lo);
	return result;
}
template<>
really_inline simd_bitmask<instruction_set::avx2> splat_bitmask(const uint64_t bitmask) {
	simd_bitmask<instruction_set::avx2> result;
	result.mask = _mm256_set1_epi64x(bitmask);
	return result;
}
really_inline simd_bitmask<instruction_set::avx2> operator&(const simd_bitmask<instruction_set::avx2> a, const simd_bitmask<instruction_set::avx2> b) {
	simd_bitmask<instruction_set::avx2> result;
	result.mask = _mm256_and_si256(a.mask, b.mask);
	return result;
}
really_inline simd_bitmask<instruction_set::avx2> operator|(const simd_bitmask<instruction_set::avx2> a, const simd_bitmask<instruction_set::avx2> b) {
	simd_bitmask<instruction_set::avx2> result;
	result.mask = _mm256_or_si256(a.mask, b.mask);
	return result;
}
really_inline simd_bitmask<instruction_set::avx2> operator^(const simd_bitmask<instruction_set::avx2> a, const simd_bitmask<instruction_set::avx2> b) {
	simd_bitmask<instruction_set::avx2> result;
	result.mask = _mm256_xor_si256(a.mask, b.mask);
	return result;
}
really_inline simd_bitmask<instruction_set::avx2> operator~(const simd_bitmask<instruction_set::avx2> bitmask) {
	return bitmask ^ splat_bitmask<instruction_set::avx2>(-1);
}
really_inline simd_bitmask<instruction_set::avx2> shl_overflow(const simd_bitmask<instruction_set::avx2> a, int n, uint64_t &overflow) {
	simd_bitmask<instruction_set::avx2> result;
	// Shift the bits inside each 64-bit lane left, and bring zeroes in.
	__m256i shifted64 = _mm256_slli_epi64(a.mask, n);
	// Move the carry bit over to the right side of each 64-bit lane, shifting zeroes in on the left.
	__m256i carry64 = _mm256_srli_epi64(a.mask, 64 - n);
	// Pull the overflow from the high bit (and save the current overflow, since we still need to use it)
	uint64_t prev_overflow = overflow;
	overflow = _mm256_extract_epi64(carry64, 3);
	// Move each 64-bit lane up 1, effectively moving that carry bit into the right place.
	carry64 = _mm256_permute4x64_epi64(carry64, _MM_SHUFFLE(2, 1, 0, 3));
	// Replace the low bit with the overflow from last time
	carry64 = _mm256_or_si256(carry64, _mm256_set_epi64x(0, 0, 0, prev_overflow));
	// Put the answer together!
	result.mask = _mm256_or_si256(shifted64, carry64);
	return result;
}
really_inline simd_bitmask<instruction_set::avx2> add_overflow(const simd_bitmask<instruction_set::avx2> a, int n) {
	simd_bitmask<instruction_set::avx2> result;
	// Shift the bits inside each 64-bit lane left, and bring zeroes in.
	__m256i shifted64 = _mm256_slli_epi64(a.mask, n);
	// Move the carry bit over to the right side of each 64-bit lane, shifting zeroes in on the left.
	__m256i carry64 = _mm256_srli_epi64(a.mask, 64 - n);
	// Move each 64-bit lane up 1, effectively moving that carry bit into the right place.
	carry64 = _mm256_permute4x64_epi64(carry64, _MM_SHUFFLE(2, 1, 0, 3));
	// Mask out the low bit, which now has the high bit from the far left.
	carry64 = _mm256_andnot_si256(carry64, _mm256_set_epi64x(0, 0, 0, 1));
	// Put the answer together!
	result.mask = _mm256_or_si256(shifted64, carry64);
	return result;
}
#endif

#if defined(__SSE4_2__) || (defined(_MSC_VER) && defined(_M_AMD64))
template<>
really_inline void split_bitmask(uint64_t(&dest)[4], const simd_bitmask<instruction_set::sse4_2> bitmask) {
	_mm_storeu_si128(reinterpret_cast<__m128i*>(&dest[0]), bitmask.lo);
	_mm_storeu_si128(reinterpret_cast<__m128i*>(&dest[2]), bitmask.hi);
}
template<>
really_inline simd_bitmask<instruction_set::sse4_2> join_bitmask(const uint64_t bitmasks[4]) {
	simd_bitmask<instruction_set::sse4_2> bitmask;
	bitmask.lo = _mm_load_si128(reinterpret_cast<const __m128i*>(&bitmasks[0]));
	bitmask.hi = _mm_load_si128(reinterpret_cast<const __m128i*>(&bitmasks[2]));
	return bitmask;
}
template<>
really_inline simd_bitmask<instruction_set::sse4_2> set_bitmask(const uint64_t hi, const uint64_t mid_hi, const uint64_t mid_lo, const uint64_t lo) {
	simd_bitmask<instruction_set::sse4_2> result;
	result.hi = _mm_set_epi64x(hi, mid_hi);
	result.lo = _mm_set_epi64x(mid_lo, lo);
	return result;
}
template<>
really_inline simd_bitmask<instruction_set::sse4_2> splat_bitmask(const uint64_t bitmask) {
	simd_bitmask<instruction_set::sse4_2> result;
	result.hi = _mm_set1_epi64x(bitmask);
	result.lo = _mm_set1_epi64x(bitmask);
	return result;
}
really_inline simd_bitmask<instruction_set::sse4_2> operator&(const simd_bitmask<instruction_set::sse4_2> a, const simd_bitmask<instruction_set::sse4_2> b) {
	simd_bitmask<instruction_set::sse4_2> result;
	result.hi = _mm_and_si128(a.hi, b.hi);
	result.lo = _mm_and_si128(a.lo, b.lo);
	return result;
}
really_inline simd_bitmask<instruction_set::sse4_2> operator|(const simd_bitmask<instruction_set::sse4_2> a, const simd_bitmask<instruction_set::sse4_2> b) {
	simd_bitmask<instruction_set::sse4_2> result;
	result.hi = _mm_or_si128(a.hi, b.hi);
	result.lo = _mm_or_si128(a.lo, b.lo);
	return result;
}
really_inline simd_bitmask<instruction_set::sse4_2> operator^(const simd_bitmask<instruction_set::sse4_2> a, const simd_bitmask<instruction_set::sse4_2> b) {
	simd_bitmask<instruction_set::sse4_2> result;
	result.hi = _mm_xor_si128(a.hi, b.hi);
	result.lo = _mm_xor_si128(a.lo, b.lo);
	return result;
}
really_inline simd_bitmask<instruction_set::sse4_2> operator~(const simd_bitmask<instruction_set::sse4_2> bitmask) {
	return bitmask ^ splat_bitmask<instruction_set::sse4_2>(-1);
}
really_inline __m128i shl128_overflow(__m128i a, int n, uint64_t &overflow) {
	// Shift the bits inside each 64-bit lane left, and bring zeroes in.
	__m128i shifted64 = _mm_slli_epi64(a, n);
	// Move the carry bit over to the right side of each 64-bit lane, shifting zeroes in on the left.
	__m128i carry64 = _mm_srli_epi64(a, 64 - n);
	// Pull the overflow from the high bit (and save last time's overflow, since we still need to use it)
	uint64_t prev_overflow = overflow;
	overflow = _mm_extract_epi64(carry64, 1);
	// Move the overflow from the low 64 bits into the high 64 bits, zeroing everything else out
	carry64 = _mm_shuffle_epi32(carry64, 0b11001111);
	// Bring in the overflow from last time)
	carry64 = _mm_or_si128(carry64, _mm_set_epi64x(0, prev_overflow));
	// Put the answer together!
	return _mm_or_si128(shifted64, carry64);
}
really_inline simd_bitmask<instruction_set::sse4_2> shl_overflow(const simd_bitmask<instruction_set::sse4_2> a, int n, uint64_t &overflow) {
	simd_bitmask<instruction_set::sse4_2> result;
	result.hi = shl128_overflow(a.hi, n, overflow);
	result.lo = shl128_overflow(a.lo, n, overflow);
	return result;
}
#endif

// a straightforward comparison of a mask against input. 5 uops; would be
// cheaper in AVX512.
template<instruction_set T>
uint64_t cmp_mask_against_input(simd_input<T> in, uint8_t m);
template<instruction_set T>
simd_bitmask<T> cmp_mask_against_input_256(const uint8_t* chunk, uint8_t m);

#ifdef __AVX2__
template<> really_inline
uint64_t cmp_mask_against_input<instruction_set::avx2>(simd_input<instruction_set::avx2> in, uint8_t m) {
  const __m256i mask = _mm256_set1_epi8(m);
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(in.lo, mask);
  uint64_t res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp_res_0));
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(in.hi, mask);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
}
#endif

template<instruction_set T, typename Func>
really_inline void each_simd_input(const uint8_t* chunk, Func&&f)
{
	//some things
	f(0, fill_input<T>(&chunk[0*64]));
	f(1, fill_input<T>(&chunk[1*64]));
	f(2, fill_input<T>(&chunk[2*64]));
	f(3, fill_input<T>(&chunk[3*64]));
}
template<instruction_set T, typename Func>
really_inline simd_bitmask<T> mask_each_simd_input(const uint8_t* chunk, Func&&f)
{
	uint64_t result[4];
	each_simd_input<T>(chunk, [&](int lane, simd_input<T> in) { result[lane] = f(in); });
	return join_bitmask<T>(result);
}
template<instruction_set T>
really_inline simd_bitmask<T> cmp_mask_against_each_input(const uint8_t* chunk, uint8_t m)
{
	return mask_each_simd_input<T>(chunk, [&](simd_input<T> in) -> uint64_t { return cmp_mask_against_input<T>(in, m); });
}

#if defined(__SSE4_2__) || (defined(_MSC_VER) && defined(_M_AMD64))
template<> really_inline
uint64_t cmp_mask_against_input<instruction_set::sse4_2>(simd_input<instruction_set::sse4_2> in, uint8_t m) {
  const __m128i mask = _mm_set1_epi8(m);
  __m128i cmp_res_0 = _mm_cmpeq_epi8(in.v0, mask);
  uint64_t res_0 = _mm_movemask_epi8(cmp_res_0);
  __m128i cmp_res_1 = _mm_cmpeq_epi8(in.v1, mask);
  uint64_t res_1 = _mm_movemask_epi8(cmp_res_1);
  __m128i cmp_res_2 = _mm_cmpeq_epi8(in.v2, mask);
  uint64_t res_2 = _mm_movemask_epi8(cmp_res_2);
  __m128i cmp_res_3 = _mm_cmpeq_epi8(in.v3, mask);
  uint64_t res_3 = _mm_movemask_epi8(cmp_res_3);
  return res_0 | (res_1 << 16) | (res_2 << 32) | (res_3 << 48);
}
#endif

#if defined(__ARM_NEON)  || (defined(_MSC_VER) && defined(_M_ARM64))
template<> really_inline
uint64_t cmp_mask_against_input<instruction_set::neon>(simd_input<instruction_set::neon> in, uint8_t m) {
  const uint8x16_t mask = vmovq_n_u8(m); 
  uint8x16_t cmp_res_0 = vceqq_u8(in.i0, mask); 
  uint8x16_t cmp_res_1 = vceqq_u8(in.i1, mask); 
  uint8x16_t cmp_res_2 = vceqq_u8(in.i2, mask); 
  uint8x16_t cmp_res_3 = vceqq_u8(in.i3, mask); 
  return neonmovemask_bulk(cmp_res_0, cmp_res_1, cmp_res_2, cmp_res_3);
}
#endif

// find all values less than or equal than the content of maxval (using unsigned arithmetic) 
template<instruction_set T>
uint64_t unsigned_lteq_against_input(simd_input<T> in, uint8_t m);

#ifdef __AVX2__
template<> really_inline
uint64_t unsigned_lteq_against_input<instruction_set::avx2>(simd_input<instruction_set::avx2> in, uint8_t m) {
  const __m256i maxval = _mm256_set1_epi8(m);
  __m256i cmp_res_0 = _mm256_cmpeq_epi8(_mm256_max_epu8(maxval,in.lo),maxval);
  uint64_t res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(cmp_res_0));
  __m256i cmp_res_1 = _mm256_cmpeq_epi8(_mm256_max_epu8(maxval,in.hi),maxval);
  uint64_t res_1 = _mm256_movemask_epi8(cmp_res_1);
  return res_0 | (res_1 << 32);
}
#endif

#if defined(__SSE4_2__) || (defined(_MSC_VER) && defined(_M_AMD64))
template<> really_inline
uint64_t unsigned_lteq_against_input<instruction_set::sse4_2>(simd_input<instruction_set::sse4_2> in, uint8_t m) {
  const __m128i maxval = _mm_set1_epi8(m);
  __m128i cmp_res_0 = _mm_cmpeq_epi8(_mm_max_epu8(maxval,in.v0),maxval);
  uint64_t res_0 = _mm_movemask_epi8(cmp_res_0);
  __m128i cmp_res_1 = _mm_cmpeq_epi8(_mm_max_epu8(maxval,in.v1),maxval);
  uint64_t res_1 = _mm_movemask_epi8(cmp_res_1);
  __m128i cmp_res_2 = _mm_cmpeq_epi8(_mm_max_epu8(maxval,in.v2),maxval);
  uint64_t res_2 = _mm_movemask_epi8(cmp_res_2);
  __m128i cmp_res_3 = _mm_cmpeq_epi8(_mm_max_epu8(maxval,in.v3),maxval);
  uint64_t res_3 = _mm_movemask_epi8(cmp_res_3);
  return res_0 | (res_1 << 16) | (res_2 << 32) | (res_3 << 48);
}
#endif

#if defined(__ARM_NEON)  || (defined(_MSC_VER) && defined(_M_ARM64))
template<> really_inline
uint64_t unsigned_lteq_against_input<instruction_set::neon>(simd_input<instruction_set::neon> in, uint8_t m) {
  const uint8x16_t mask = vmovq_n_u8(m); 
  uint8x16_t cmp_res_0 = vcleq_u8(in.i0, mask); 
  uint8x16_t cmp_res_1 = vcleq_u8(in.i1, mask); 
  uint8x16_t cmp_res_2 = vcleq_u8(in.i2, mask); 
  uint8x16_t cmp_res_3 = vcleq_u8(in.i3, mask); 
  return neonmovemask_bulk(cmp_res_0, cmp_res_1, cmp_res_2, cmp_res_3);
}
#endif

// return a bitvector indicating where we have characters that end an odd-length
// sequence of backslashes (and thus change the behavior of the next character
// to follow). A even-length sequence of backslashes, and, for that matter, the
// largest even-length prefix of our odd-length sequence of backslashes, simply
// modify the behavior of the backslashes themselves.
// We also update the prev_iter_ends_odd_backslash reference parameter to
// indicate whether we end an iteration on an odd-length sequence of
// backslashes, which modifies our subsequent search for odd-length
// sequences of backslashes in an obvious way.
template<instruction_set T> really_inline
simd_bitmask<T> find_odd_backslash_sequences_256(const uint8_t* chunk, uint64_t &prev_iter_ends_odd_backslash) {
	simd_bitmask<T> even_bits = splat_bitmask<T>(0x5555555555555555ULL);
	simd_bitmask<T> odd_bits = ~even_bits;

	// detect odd sequences of backslashes
	simd_bitmask<T> bs_bits = cmp_mask_against_each_input<T>(chunk, '\\');
	// If we have a significant (odd-series) backslash at the end of the previous run, pull that backslash in
	uint64_t bs_overflow = prev_iter_ends_odd_backslash;
	simd_bitmask<T> prev_bs_bits = shl_overflow(bs_bits, 1, prev_iter_ends_odd_backslash);
	simd_bitmask<T> start_edges = bs_bits & ~prev_bs_bits;
	simd_bitmask<T> even_starts = start_edges & even_bits;
	simd_bitmask<T> odd_starts = start_edges & odd_bits;
	simd_bitmask<T> even_carries = bs_bits + even_starts;
	// must record the carry-out of our odd-carries out of bit 63; this
	// indicates whether the sense of any edge going to the next iteration
	// should be flipped
	simd_bitmask<T> odd_carries = add_overflow(bs_bits, odd_starts, prev_iter_ends_odd_backslash);
	simd_bitmask<T> even_carry_ends = even_carries & ~bs_bits;
	simd_bitmask<T> odd_carry_ends = odd_carries & ~bs_bits;
	simd_bitmask<T> even_start_odd_end = even_carry_ends & odd_bits;
	simd_bitmask<T> odd_start_even_end = odd_carry_ends & even_bits;
	simd_bitmask<T> odd_ends = even_start_odd_end | odd_start_even_end;
	return odd_ends;
}

// return both the quote mask (which is a half-open mask that covers the first
// quote
// in an unescaped quote pair and everything in the quote pair) and the quote
// bits, which are the simple
// unescaped quoted bits. We also update the prev_iter_inside_quote value to
// tell the next iteration
// whether we finished the final iteration inside a quote pair; if so, this
// inverts our behavior of
// whether we're inside quotes for the next iteration.
// Note that we don't do any error checking to see if we have backslash
// sequences outside quotes; these
// backslash sequences (of any length) will be detected elsewhere.
template<instruction_set T> really_inline
uint64_t find_quote_mask_and_bits(simd_input<T> in, uint64_t odd_ends,
    uint64_t &prev_iter_inside_quote, uint64_t &quote_bits, uint64_t &error_mask) {
  quote_bits = cmp_mask_against_input<T>(in, '"');
  quote_bits = quote_bits & ~odd_ends;
  uint64_t quote_mask = compute_quote_mask<T>(quote_bits);
  quote_mask ^= prev_iter_inside_quote;
  // All Unicode characters may be placed within the
  // quotation marks, except for the characters that MUST be escaped:
  // quotation mark, reverse solidus, and the control characters (U+0000
  //through U+001F).
  // https://tools.ietf.org/html/rfc8259
  uint64_t unescaped = unsigned_lteq_against_input<T>(in, 0x1F);
  error_mask |= quote_mask & unescaped;
  // right shift of a signed value expected to be well-defined and standard
  // compliant as of C++20,
  // John Regher from Utah U. says this is fine code
  prev_iter_inside_quote =
      static_cast<uint64_t>(static_cast<int64_t>(quote_mask) >> 63);
  return quote_mask;
}

// do a 'shufti' to detect structural JSON characters
// they are { 0x7b } 0x7d : 0x3a [ 0x5b ] 0x5d , 0x2c
// these go into the first 3 buckets of the comparison (1/2/4)

// we are also interested in the four whitespace characters
// space 0x20, linefeed 0x0a, horizontal tab 0x09 and carriage return 0x0d
// these go into the next 2 buckets of the comparison (8/16)
template<instruction_set T>
void find_whitespace_and_structurals(simd_input<T> in,
                                     uint64_t &whitespace,
                                     uint64_t &structurals);

#ifdef __AVX2__
template<> really_inline
void find_whitespace_and_structurals<instruction_set::avx2>(simd_input<instruction_set::avx2> in,
                                                     uint64_t &whitespace,
                                                     uint64_t &structurals) {
#ifdef SIMDJSON_NAIVE_STRUCTURAL
  // You should never need this naive approach, but it can be useful
  // for research purposes
  const __m256i mask_open_brace = _mm256_set1_epi8(0x7b);
  __m256i struct_lo = _mm256_cmpeq_epi8(in.lo, mask_open_brace);
  __m256i struct_hi = _mm256_cmpeq_epi8(in.hi, mask_open_brace);
  const __m256i mask_close_brace = _mm256_set1_epi8(0x7d);
  struct_lo = _mm256_or_si256(struct_lo,_mm256_cmpeq_epi8(in.lo, mask_close_brace));
  struct_hi = _mm256_or_si256(struct_hi,_mm256_cmpeq_epi8(in.hi, mask_close_brace));
  const __m256i mask_open_bracket = _mm256_set1_epi8(0x5b);
  struct_lo = _mm256_or_si256(struct_lo,_mm256_cmpeq_epi8(in.lo, mask_open_bracket));
  struct_hi = _mm256_or_si256(struct_hi,_mm256_cmpeq_epi8(in.hi, mask_open_bracket));
  const __m256i mask_close_bracket = _mm256_set1_epi8(0x5d);
  struct_lo = _mm256_or_si256(struct_lo,_mm256_cmpeq_epi8(in.lo, mask_close_bracket));
  struct_hi = _mm256_or_si256(struct_hi,_mm256_cmpeq_epi8(in.hi, mask_close_bracket));
  const __m256i mask_column = _mm256_set1_epi8(0x3a);
  struct_lo = _mm256_or_si256(struct_lo,_mm256_cmpeq_epi8(in.lo, mask_column));
  struct_hi = _mm256_or_si256(struct_hi,_mm256_cmpeq_epi8(in.hi, mask_column));
  const __m256i mask_comma = _mm256_set1_epi8(0x2c);
  struct_lo = _mm256_or_si256(struct_lo,_mm256_cmpeq_epi8(in.lo, mask_comma));
  struct_hi = _mm256_or_si256(struct_hi,_mm256_cmpeq_epi8(in.hi, mask_comma));
  uint64_t structural_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(struct_lo));
  uint64_t structural_res_1 = _mm256_movemask_epi8(struct_hi);
  structurals = (structural_res_0 | (structural_res_1 << 32));

  const __m256i mask_space = _mm256_set1_epi8(0x20);
  __m256i space_lo = _mm256_cmpeq_epi8(in.lo, mask_space);
  __m256i space_hi = _mm256_cmpeq_epi8(in.hi, mask_space);
  const __m256i mask_linefeed = _mm256_set1_epi8(0x0a);
  space_lo = _mm256_or_si256(space_lo,_mm256_cmpeq_epi8(in.lo, mask_linefeed));
  space_hi = _mm256_or_si256(space_hi,_mm256_cmpeq_epi8(in.hi, mask_linefeed));
  const __m256i mask_tab = _mm256_set1_epi8(0x09);
  space_lo = _mm256_or_si256(space_lo,_mm256_cmpeq_epi8(in.lo, mask_tab));
  space_hi = _mm256_or_si256(space_hi,_mm256_cmpeq_epi8(in.hi, mask_tab));
  const __m256i mask_carriage = _mm256_set1_epi8(0x0d);
  space_lo = _mm256_or_si256(space_lo,_mm256_cmpeq_epi8(in.lo, mask_carriage));
  space_hi = _mm256_or_si256(space_hi,_mm256_cmpeq_epi8(in.hi, mask_carriage));

  uint64_t ws_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(space_lo));
  uint64_t ws_res_1 = _mm256_movemask_epi8(space_hi);
  whitespace = (ws_res_0 | (ws_res_1 << 32));
  // end of naive approach

#else // SIMDJSON_NAIVE_STRUCTURAL
  const __m256i structural_table = _mm256_setr_epi8(
      44, 125, 0, 0, 0xc0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123, 
      44, 125, 0, 0, 0xc0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123);
  const __m256i white_table = _mm256_setr_epi8(
      32,  100,  100,  100,  17,  100,  113,  2,  100,  9,  10,  112,  100,  13,  100,  100, 
      32,  100,  100,  100,  17,  100,  113,  2,  100,  9,  10,  112,  100,  13,  100,  100);
  const __m256i struct_offset = _mm256_set1_epi8(0xd4);
  const __m256i struct_mask = _mm256_set1_epi8(32);

  __m256i lo_white = _mm256_cmpeq_epi8(in.lo, 
           _mm256_shuffle_epi8(white_table, in.lo));
  __m256i hi_white = _mm256_cmpeq_epi8(in.hi, 
           _mm256_shuffle_epi8(white_table, in.hi));
  uint64_t ws_res_0 = static_cast<uint32_t>(_mm256_movemask_epi8(lo_white));
  uint64_t ws_res_1 = _mm256_movemask_epi8(hi_white);
  whitespace = (ws_res_0 | (ws_res_1 << 32));
  __m256i lo_struct_r1 = _mm256_add_epi8(struct_offset, in.lo);
  __m256i hi_struct_r1 = _mm256_add_epi8(struct_offset, in.hi);
  __m256i lo_struct_r2 = _mm256_or_si256(in.lo, struct_mask);
  __m256i hi_struct_r2 = _mm256_or_si256(in.hi, struct_mask);
  __m256i lo_struct_r3 = _mm256_shuffle_epi8(structural_table, lo_struct_r1);
  __m256i hi_struct_r3 = _mm256_shuffle_epi8(structural_table, hi_struct_r1);
  __m256i lo_struct = _mm256_cmpeq_epi8(lo_struct_r2, lo_struct_r3);
  __m256i hi_struct = _mm256_cmpeq_epi8(hi_struct_r2, hi_struct_r3);
  
  uint64_t structural_res_0 =
      static_cast<uint32_t>(_mm256_movemask_epi8(lo_struct));
  uint64_t structural_res_1 = _mm256_movemask_epi8(hi_struct);
  structurals = (structural_res_0 | (structural_res_1 << 32));
#endif // SIMDJSON_NAIVE_STRUCTURAL
}
#endif // __AVX2__

#if defined(__SSE4_2__) || (defined(_MSC_VER) && defined(_M_AMD64))
template<> really_inline
void find_whitespace_and_structurals<instruction_set::sse4_2>(simd_input<instruction_set::sse4_2> in,
                                                     uint64_t &whitespace, uint64_t &structurals) {
  const __m128i structural_table = _mm_setr_epi8(44, 125, 0, 0, 0xc0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 58, 123);
  const __m128i white_table = _mm_setr_epi8(
      32,  100,  100,  100,  17,  100,  113,  2,  100,  9,  10,  112,  100,  13,  100,  100);
  const __m128i struct_offset = _mm_set1_epi8(0xd4);
  const __m128i struct_mask = _mm_set1_epi8(32);

  __m128i white0 = _mm_cmpeq_epi8(in.v0,
           _mm_shuffle_epi8(white_table, in.v0));
  __m128i white1 = _mm_cmpeq_epi8(in.v1,
           _mm_shuffle_epi8(white_table, in.v1));
  __m128i white2 = _mm_cmpeq_epi8(in.v2,
           _mm_shuffle_epi8(white_table, in.v2));
  __m128i white3 = _mm_cmpeq_epi8(in.v3,
           _mm_shuffle_epi8(white_table, in.v3));
  uint64_t ws_res_0 = _mm_movemask_epi8(white0);
  uint64_t ws_res_1 = _mm_movemask_epi8(white1);
  uint64_t ws_res_2 = _mm_movemask_epi8(white2);
  uint64_t ws_res_3 = _mm_movemask_epi8(white3);

  whitespace = (ws_res_0 | (ws_res_1 << 16) | (ws_res_2 << 32) | (ws_res_3 << 48));

  __m128i struct1_r1 = _mm_add_epi8(struct_offset, in.v0);
  __m128i struct2_r1 = _mm_add_epi8(struct_offset, in.v1);
  __m128i struct3_r1 = _mm_add_epi8(struct_offset, in.v2);
  __m128i struct4_r1 = _mm_add_epi8(struct_offset, in.v3);

  __m128i struct1_r2 = _mm_or_si128(in.v0, struct_mask);
  __m128i struct2_r2 = _mm_or_si128(in.v1, struct_mask);
  __m128i struct3_r2 = _mm_or_si128(in.v2, struct_mask);
  __m128i struct4_r2 = _mm_or_si128(in.v3, struct_mask);

  __m128i struct1_r3 = _mm_shuffle_epi8(structural_table, struct1_r1);
  __m128i struct2_r3 = _mm_shuffle_epi8(structural_table, struct2_r1);
  __m128i struct3_r3 = _mm_shuffle_epi8(structural_table, struct3_r1);
  __m128i struct4_r3 = _mm_shuffle_epi8(structural_table, struct4_r1);

  __m128i struct1 = _mm_cmpeq_epi8(struct1_r2, struct1_r3);
  __m128i struct2 = _mm_cmpeq_epi8(struct2_r2, struct2_r3);
  __m128i struct3 = _mm_cmpeq_epi8(struct3_r2, struct3_r3);
  __m128i struct4 = _mm_cmpeq_epi8(struct4_r2, struct4_r3);

  uint64_t structural_res_0 = _mm_movemask_epi8(struct1);
  uint64_t structural_res_1 = _mm_movemask_epi8(struct2);
  uint64_t structural_res_2 = _mm_movemask_epi8(struct3);
  uint64_t structural_res_3 = _mm_movemask_epi8(struct4);

  structurals = (structural_res_0 | (structural_res_1 << 16) | (structural_res_2 << 32) | (structural_res_3 << 48));
}
#endif // __SSE4_2__

#if defined(__ARM_NEON)  || (defined(_MSC_VER) && defined(_M_ARM64))
template<> really_inline
void find_whitespace_and_structurals<instruction_set::neon>(
                                                  simd_input<instruction_set::neon> in,
                                                  uint64_t &whitespace,
                                                  uint64_t &structurals) {
  const uint8x16_t low_nibble_mask = (uint8x16_t){ 
      16, 0, 0, 0, 0, 0, 0, 0, 0, 8, 12, 1, 2, 9, 0, 0};
  const uint8x16_t high_nibble_mask = (uint8x16_t){ 
      8, 0, 18, 4, 0, 1, 0, 1, 0, 0, 0, 3, 2, 1, 0, 0};
  const uint8x16_t structural_shufti_mask = vmovq_n_u8(0x7); 
  const uint8x16_t whitespace_shufti_mask = vmovq_n_u8(0x18); 
  const uint8x16_t low_nib_and_mask = vmovq_n_u8(0xf); 

  uint8x16_t nib_0_lo = vandq_u8(in.i0, low_nib_and_mask);
  uint8x16_t nib_0_hi = vshrq_n_u8(in.i0, 4);
  uint8x16_t shuf_0_lo = vqtbl1q_u8(low_nibble_mask, nib_0_lo);
  uint8x16_t shuf_0_hi = vqtbl1q_u8(high_nibble_mask, nib_0_hi);
  uint8x16_t v_0 = vandq_u8(shuf_0_lo, shuf_0_hi);

  uint8x16_t nib_1_lo = vandq_u8(in.i1, low_nib_and_mask);
  uint8x16_t nib_1_hi = vshrq_n_u8(in.i1, 4);
  uint8x16_t shuf_1_lo = vqtbl1q_u8(low_nibble_mask, nib_1_lo);
  uint8x16_t shuf_1_hi = vqtbl1q_u8(high_nibble_mask, nib_1_hi);
  uint8x16_t v_1 = vandq_u8(shuf_1_lo, shuf_1_hi);

  uint8x16_t nib_2_lo = vandq_u8(in.i2, low_nib_and_mask);
  uint8x16_t nib_2_hi = vshrq_n_u8(in.i2, 4);
  uint8x16_t shuf_2_lo = vqtbl1q_u8(low_nibble_mask, nib_2_lo);
  uint8x16_t shuf_2_hi = vqtbl1q_u8(high_nibble_mask, nib_2_hi);
  uint8x16_t v_2 = vandq_u8(shuf_2_lo, shuf_2_hi);

  uint8x16_t nib_3_lo = vandq_u8(in.i3, low_nib_and_mask);
  uint8x16_t nib_3_hi = vshrq_n_u8(in.i3, 4);
  uint8x16_t shuf_3_lo = vqtbl1q_u8(low_nibble_mask, nib_3_lo);
  uint8x16_t shuf_3_hi = vqtbl1q_u8(high_nibble_mask, nib_3_hi);
  uint8x16_t v_3 = vandq_u8(shuf_3_lo, shuf_3_hi);

  uint8x16_t tmp_0 = vtstq_u8(v_0, structural_shufti_mask);
  uint8x16_t tmp_1 = vtstq_u8(v_1, structural_shufti_mask);
  uint8x16_t tmp_2 = vtstq_u8(v_2, structural_shufti_mask);
  uint8x16_t tmp_3 = vtstq_u8(v_3, structural_shufti_mask);
  structurals = neonmovemask_bulk(tmp_0, tmp_1, tmp_2, tmp_3);

  uint8x16_t tmp_ws_0 = vtstq_u8(v_0, whitespace_shufti_mask);
  uint8x16_t tmp_ws_1 = vtstq_u8(v_1, whitespace_shufti_mask);
  uint8x16_t tmp_ws_2 = vtstq_u8(v_2, whitespace_shufti_mask);
  uint8x16_t tmp_ws_3 = vtstq_u8(v_3, whitespace_shufti_mask);
  whitespace = neonmovemask_bulk(tmp_ws_0, tmp_ws_1, tmp_ws_2, tmp_ws_3);
}
#endif // __ARM_NEON


#ifdef SIMDJSON_NAIVE_FLATTEN // useful for benchmarking
//
// This is just a naive implementation. It should be normally
// disable, but can be used for research purposes to compare
// again our optimized version.
really_inline void flatten_bits(uint32_t *base_ptr, uint32_t &base,
                                uint32_t idx, uint64_t bits) {
  uint32_t * out_ptr = base_ptr + base;
  idx -= 64;
  while(bits != 0) {
      out_ptr[0] = idx + trailingzeroes(bits);
      bits = bits & (bits - 1);
      out_ptr++;
  }
  base = (out_ptr - base_ptr);
}

#else 
// flatten out values in 'bits' assuming that they are are to have values of idx
// plus their position in the bitvector, and store these indexes at
// base_ptr[base] incrementing base as we go
// will potentially store extra values beyond end of valid bits, so base_ptr
// needs to be large enough to handle this
really_inline void flatten_bits(uint32_t *base_ptr, uint32_t &base,
                                uint32_t idx, uint64_t bits) {
  // In some instances, the next branch is expensive because it is mispredicted. 
  // Unfortunately, in other cases,
  // it helps tremendously.
  if(bits == 0) return; 
  uint32_t cnt = hamming(bits);
  uint32_t next_base = base + cnt;
  idx -= 64;
  base_ptr += base;
  { 
    base_ptr[0] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[1] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[2] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[3] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[4] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[5] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[6] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[7] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr += 8;
  }
  // We hope that the next branch is easily predicted.
  if (cnt > 8) {
    base_ptr[0] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[1] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[2] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[3] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[4] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[5] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[6] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr[7] = idx + trailingzeroes(bits);
    bits = bits & (bits - 1);
    base_ptr += 8;
  }
  if (cnt > 16) { // unluckly: we rarely get here
    // since it means having one structural or pseudo-structral element 
    // every 4 characters (possible with inputs like "","","",...).
    do {
      base_ptr[0] = idx + trailingzeroes(bits);
      bits = bits & (bits - 1);
      base_ptr++;
    } while(bits != 0);
  }
  base = next_base;
}
#endif // SIMDJSON_NAIVE_FLATTEN

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
  // add the real quote bits back into our bitmask as well, so we can
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

// Find structural bits, for a guaranteed-256-byte-wide chunk
template<instruction_set T = instruction_set::native>
really_inline void find_structural_bits_256(const uint8_t *chunk,
                                           const size_t chunk_idx,
                                           ParsedJson &pj,
                                           uint32_t *base_ptr,
                                           uint32_t &base,
                                           uint64_t &prev_iter_ends_odd_backslash,
                                           uint64_t &prev_iter_inside_quote,
                                           uint64_t &prev_iter_ends_pseudo_pred,
                                           uint64_t &structurals,
                                           uint64_t &error_mask) {
	// detect odd sequences of backslashes
	uint64_t odd_ends[4];
	split_bitmask(odd_ends, find_odd_backslash_sequences_256<T>(chunk, prev_iter_ends_odd_backslash));
	each_simd_input<T>(chunk, [&](int lane, simd_input<T> in) {
		// detect insides of quote pairs ("quote_mask") and also our quote_bits
		// themselves
		uint64_t quote_bits;
		uint64_t quote_mask = find_quote_mask_and_bits<T>(
			in, odd_ends[lane], prev_iter_inside_quote, quote_bits, error_mask);

		// take the previous iterations structural bits, not our current iteration,
		// and flatten
		flatten_bits(base_ptr, base, chunk_idx + lane * 64, structurals);

		uint64_t whitespace;
		find_whitespace_and_structurals<T>(in, whitespace, structurals);

		// fixup structurals to reflect quotes and add pseudo-structural characters
		structurals = finalize_structurals(structurals, whitespace, quote_mask,
			quote_bits, prev_iter_ends_pseudo_pred);
	});
}

template<instruction_set T = instruction_set::native>
WARN_UNUSED
/*never_inline*/ int find_structural_bits(const uint8_t *buf, size_t len,
                                           ParsedJson &pj) {
  if (len > pj.bytecapacity) {
    std::cerr << "Your ParsedJson object only supports documents up to "
         << pj.bytecapacity << " bytes but you are trying to process " << len
         << " bytes" << std::endl;
    return simdjson::CAPACITY;
  }
  uint32_t *base_ptr = pj.structural_indexes;
  uint32_t base = 0;
#ifdef SIMDJSON_UTF8VALIDATE
  utf8_checking_state<T> utf8_state;
#endif

  // we have padded the input out to 64 byte multiple with the remainder being
  // zeros

  // persistent state across loop
  // does the last iteration end with an odd-length sequence of backslashes? 
  // either 0 or 1, but a 64-bit value
  uint64_t prev_iter_ends_odd_backslash = 0ULL;
  // does the previous iteration end inside a double-quote pair?
  uint64_t prev_iter_inside_quote = 0ULL;  // either all zeros or all ones
  // does the previous iteration end on something that is a predecessor of a
  // pseudo-structural character - i.e. whitespace or a structural character
  // effectively the very first char is considered to follow "whitespace" for
  // the
  // purposes of pseudo-structural character detection so we initialize to 1
  uint64_t prev_iter_ends_pseudo_pred = 1ULL;

  // structurals are persistent state across loop as we flatten them on the
  // subsequent iteration into our array pointed to be base_ptr.
  // This is harmless on the first iteration as structurals==0
  // and is done for performance reasons; we can hide some of the latency of the
  // expensive carryless multiply in the previous step with this work
  uint64_t structurals = 0;

  uint64_t error_mask = 0; // for unescaped characters within strings (ASCII code points < 0x20)

  // Iterate over 256-byte-wide chunks so we can use wider bitmasks
  size_t lenminus256 = len < 256 ? 0 : len - 256;
  size_t idx = 0;
  for (; idx < lenminus256; idx += 256) {
#ifndef _MSC_VER
    __builtin_prefetch(buf + idx + 512);
#endif
#ifdef SIMDJSON_UTF8VALIDATE
    check_utf8_256(buf + idx, utf8_state);
#endif
    find_structural_bits_256(buf + idx, idx, pj, base_ptr, base, prev_iter_ends_odd_backslash, prev_iter_inside_quote, prev_iter_ends_pseudo_pred, structurals, error_mask);
  }

  // Handle the final, partial buffer
  if (idx < len) {
    uint8_t tmpbuf[256];
    // Fill with spaces
    memset(tmpbuf, 0x20, 256);
    memcpy(tmpbuf, buf + idx, len - idx);
#ifdef SIMDJSON_UTF8VALIDATE
    check_utf8_256(tmpbuf, utf8_state);
#endif
    find_structural_bits_256(tmpbuf, idx, pj, base_ptr, base, prev_iter_ends_odd_backslash, prev_iter_inside_quote, prev_iter_ends_pseudo_pred, structurals, error_mask);

    idx += 256;
  }

  // If the last string quote was stil open, it's invalid JSON.
  if (prev_iter_inside_quote) {
    return simdjson::UNCLOSED_STRING;
  }

  // finally, flatten out the remaining structurals from the last iteration
  flatten_bits(base_ptr, base, idx, structurals);

  pj.n_structural_indexes = base;
  // a valid JSON file cannot have zero structural indexes - we should have
  // found something
  if (pj.n_structural_indexes == 0u) {
    return simdjson::EMPTY;
  }
  if (base_ptr[pj.n_structural_indexes - 1] > len) {
    return simdjson::UNEXPECTED_ERROR;
  }
  if (len != base_ptr[pj.n_structural_indexes - 1]) {
    // the string might not be NULL terminated, but we add a virtual NULL ending
    // character.
    base_ptr[pj.n_structural_indexes++] = len;
  }
  // make it safe to dereference one beyond this array
  base_ptr[pj.n_structural_indexes] = 0;  
  if (error_mask) {
    return simdjson::UNESCAPED_CHARS;
  }
#ifdef SIMDJSON_UTF8VALIDATE
  return check_utf8_errors<T>(utf8_state);
#else
  return simdjson::SUCCESS;
#endif
}

template<instruction_set T = instruction_set::native>
WARN_UNUSED
int find_structural_bits(const char *buf, size_t len, ParsedJson &pj) {
  return find_structural_bits<T>(reinterpret_cast<const uint8_t *>(buf), len, pj);
}
}
#endif
